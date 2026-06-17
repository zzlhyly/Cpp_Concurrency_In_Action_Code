#include <atomic>
#include <memory>
#include <thread>
#include <exception>
#include <stdexcept>

const unsigned max_hazard_pointers = 100;
struct hazard_pointer
{
	std::atomic<std::thread::id> m_id;
	std::atomic<void*> m_pointer;
};
hazard_pointer g_hazard_pointers[max_hazard_pointers];

class hp_owner
{
public:
	hp_owner()
		: m_hp(nullptr)
	{
		for (unsigned index = 0; index < max_hazard_pointers; ++index)
		{
			std::thread::id old_id;
			if (g_hazard_pointers[index].m_id.compare_exchange_strong(old_id, std::this_thread::get_id()))
			{
				m_hp = &g_hazard_pointers[index];
				break;
			}

			if (m_hp == nullptr)
			{
				throw std::runtime_error("No hazard pointers available");
			}
		}
	}
	hp_owner(const hp_owner&) = delete;
	hp_owner &operator=(const hp_owner&) = delete;
	~hp_owner()
	{
		m_hp->m_pointer.store(nullptr);
		m_hp->m_id.store(std::thread::id());
	}

	std::atomic<void*> &get_pointer()
	{
		return m_hp->m_pointer;
	}

private:
	hazard_pointer * m_hp;
};

std::atomic<void*> &get_hazard_pointer_for_current_thread()
{
	thread_local static hp_owner hazard;
	return hazard.get_pointer();
}

bool outstanding_hazard_pointers_for(void *p)
{
	for (unsigned index = 0; index < max_hazard_pointers; ++index)
	{
		if (g_hazard_pointers[index].m_pointer.load() == p)
		{
			return true;
		}
	}

	return false;
}

template <typename T>
void do_delete(void *p)
{
	delete static_cast<T*>(p);
}

template <typename T>
class lock_free_stack
{
public:
	void push(const T &new_value)
	{
		stack_node *new_node = new stack_node(new_value);
		new_node->m_next = m_head.load();
		while (!m_head.compare_exchange_weak(new_node->m_next, new_node))
		{

		}
	}

	std::shared_ptr<T> pop()
	{
		std::atomic<void*>& hp = get_hazard_pointer_for_current_thread();
		stack_node *old_head = m_head.load();
		do 
		{
			stack_node *temp = nullptr;
			do 
			{
				temp = old_head;
				hp.store(old_head);
				old_head = m_head.load();
			} while (old_head != temp);
		} while (old_head != nullptr
			&& !m_head.compare_exchange_strong(old_head, old_head->m_next));

		hp.store(nullptr);
		std::shared_ptr<T> res = nullptr;
		if (old_head != nullptr)
		{
			res.swap(old_head->m_spData);
			if (outstanding_hazard_pointers_for(old_head))
			{
				recliam_later(old_head);
			}
			else
			{
				delete old_head;
			}
			delete_node_with_no_hazards();
		}

		return res;
	}

private:
	struct stack_node
	{
		explicit stack_node(const T &data)
			: m_spData(std::make_shared<T>(data))
		{

		}
		std::shared_ptr<T> m_spData;
		stack_node *m_next = nullptr;
	};

	std::atomic<stack_node*> m_head = nullptr;

	struct data_to_reclaim
	{
		template <typename T>
		data_to_reclaim(T *p)
			: m_data(p), m_deleter(&do_delete<T>), m_next(nullptr)
		{

		}

		~data_to_reclaim()
		{
			m_deleter(m_data);
		}

		void *m_data = nullptr;
		std::function<void(void*)> m_deleter;
		data_to_reclaim *m_next = nullptr;
	};

	std::atomic<data_to_reclaim*> m_nodes_to_reclaim;

	void add_to_reclaim_list(data_to_reclaim *node)
	{
		node->m_next = m_nodes_to_reclaim.load();
		while (!m_nodes_to_reclaim.compare_exchange_weak(node->m_next, node))
		{

		}
	}

	template <typename T>
	void recliam_later(T *data)
	{
		add_to_reclaim_list(new data_to_reclaim(data));
	}

	void delete_node_with_no_hazards()
	{
		data_to_reclaim *current = m_nodes_to_reclaim.exchange(nullptr);
		while (current != nullptr)
		{
			data_to_reclaim *const next = current->m_next;
			if (!outstanding_hazard_pointers_for(current->m_data))
			{
				delete current;
			}
			else
			{
				add_to_reclaim_list(current);
			}

			current = next;
		}
	}
};

int main()
{
	lock_free_stack<int> lfs;
	lfs.push(33);
	lfs.push(44);
	lfs.push(33);
	lfs.pop();

	return EXIT_SUCCESS;
}