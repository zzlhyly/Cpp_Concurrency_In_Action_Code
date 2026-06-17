#define _ENABLE_ATOMIC_ALIGNMENT_FIX
#include <memory>
#include <atomic>

template <typename T>
class lock_free_stack
{
public:
	lock_free_stack() = default;
	~lock_free_stack()
	{

	}

	void push(const T &data)
	{
		stack_counted_node_ptr new_node;
		new_node.m_ptr = new stack_node(data);
		new_node.m_external_count = 1;
		new_node.m_ptr->m_next = m_head.load(std::memory_order_relaxed);
		while (!m_head.compare_exchange_weak(new_node.m_ptr->m_next, new_node, std::memory_order_release, std::memory_order_relaxed))
		{

		}
	}

	std::shared_ptr<T> pop()
	{
		stack_counted_node_ptr old_head = m_head.load(std::memory_order_relaxed);
		while (true)
		{
			increase_head_count(old_head);
			stack_node *const ptr = old_head.m_ptr;
			if (ptr == nullptr)
			{
				return nullptr;
			}

			if (m_head.compare_exchange_strong(old_head, old_head.m_ptr->m_next, std::memory_order_relaxed))
			{
				std::shared_ptr<T> res = nullptr;
				res.swap(ptr->m_data);
				const int count_increase = old_head.m_external_count - 2;
				if (ptr->m_internal_count.fetch_add(count_increase, std::memory_order_release) == -count_increase)
				{
					delete ptr;
				}

				return res;
			}
			else if (ptr->m_internal_count.fetch_add(-1, std::memory_order_relaxed) == 1)
			{
				ptr->m_internal_count.load(std::memory_order_acquire);
				delete ptr;
			}
		}
	}

private:
	struct stack_node;

	struct stack_counted_node_ptr
	{
		int m_external_count = 0;
		stack_node *m_ptr = nullptr;
	};

	struct stack_node
	{
	public:
		stack_node(const T &data)
			: m_data(std::make_shared<T>(data))
		{

		}

		std::shared_ptr<T> m_data = nullptr;
		std::atomic<int> m_internal_count = 0;
		stack_counted_node_ptr m_next;
	};

	std::atomic<stack_counted_node_ptr> m_head;

	void increase_head_count(stack_counted_node_ptr &old_counter)
	{
		stack_counted_node_ptr new_counter;
		do 
		{
			new_counter = old_counter;
			++new_counter.m_external_count;
		} while (!m_head.compare_exchange_strong(old_counter, new_counter, std::memory_order_acquire, std::memory_order_relaxed));
		old_counter.m_external_count = new_counter.m_external_count;
	}
};

int main()
{
	lock_free_stack<int> lfs;
	lfs.push(232);
	lfs.pop();

	return EXIT_SUCCESS;
}