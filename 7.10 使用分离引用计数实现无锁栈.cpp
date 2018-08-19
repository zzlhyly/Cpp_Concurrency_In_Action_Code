#define _ENABLE_ATOMIC_ALIGNMENT_FIX
#include <memory>
#include <atomic>
#include <thread>

template <typename T>
class lock_free_stack
{
public:
	lock_free_stack() = default;
	~lock_free_stack()
	{

	}

	void push(const T & data)
	{
		counted_stack_node_ptr new_node;
		new_node.m_ptr = new stack_node(data);
		new_node.m_external_count = 1;
		new_node.m_ptr->m_next = m_head.load();
		while (!m_head.compare_exchange_weak(new_node.m_ptr->m_next, new_node))
		{

		}
	}

	std::shared_ptr<T> pop()
	{
		counted_stack_node_ptr old_head = m_head.load();
		while (true)
		{
			increase_head_count(old_head);
			stack_node *const ptr = old_head.m_ptr;

			if (ptr == nullptr)
			{
				return nullptr;
			}

			if (m_head.compare_exchange_strong(old_head, old_head.m_ptr->m_next))
			{
				std::shared_ptr<T> res = nullptr;
				res.swap(ptr->m_data);
				const int count_increase = old_head.m_external_count - 2;
				if (ptr->m_internal_count.fetch_add(count_increase) == -count_increase)
				{
					delete ptr;
				}

				return res;
			}
			else if (ptr->m_internal_count.fetch_add(1) == 1)
			{
				delete ptr;
			}
		}
	}

private:
	struct stack_node;
	
	struct counted_stack_node_ptr
	{
		int m_external_count = 0;
		stack_node *m_ptr = nullptr;
	};

	struct stack_node
	{
	public:
		stack_node(const T &data)
			: m_data(std::make_shared<T>(data)), m_internal_count(0)
		{

		}
		std::shared_ptr<T> m_data = nullptr;
		std::atomic<int> m_internal_count = 0;
		counted_stack_node_ptr m_next;
	};

	std::atomic<counted_stack_node_ptr> m_head;

	void increase_head_count(counted_stack_node_ptr &old_counter)
	{
		counted_stack_node_ptr new_counter;
		do 
		{
			new_counter = old_counter;
			++new_counter.m_external_count;
		} while (!m_head.compare_exchange_weak(old_counter, new_counter));

		old_counter.m_external_count = new_counter.m_external_count;
	}
};

lock_free_stack<int> g_lfs;

void Test1()
{
	g_lfs.push(23323);
	g_lfs.push(23323);
	g_lfs.push(23323);
	g_lfs.push(23323);
	g_lfs.push(23323);
	g_lfs.push(23323);
	g_lfs.pop();
	g_lfs.pop();
	g_lfs.pop();
	g_lfs.pop();
	g_lfs.pop();
}

void Test2()
{
	g_lfs.pop();
	g_lfs.pop();
	g_lfs.pop();
	g_lfs.pop();
	g_lfs.pop();
}

int main()
{
	std::thread t1(Test1);
	std::thread t2(Test2);
	std::thread t3(Test1);
	std::thread t4(Test2);
	std::thread t5(Test1);
	std::thread t6(Test2);

	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	t6.join();

	return EXIT_SUCCESS;
}