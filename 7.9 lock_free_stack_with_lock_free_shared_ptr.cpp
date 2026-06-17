#include <memory>
#include <atomic>

template <typename T>
class lock_free_stack
{
public:
	void push(const T &data)
	{
		std::shared_ptr<stack_node> const new_node = std::make_shared<stack_node>(data);
		new_node->m_next = std::atomic_load(&m_head);
		while (!std::atomic_compare_exchange_weak(&m_head, &new_node->m_next, new_node))
		{

		}
	}

	std::shared_ptr<T> pop()
	{
		std::shared_ptr<stack_node> old_head = std::atomic_load(&m_head);
		while (old_head != nullptr
			&& std::atomic_compare_exchange_weak(&m_head, &old_head, old_head->m_next))
		{

		}

		return old_head != nullptr ? old_head->m_data : nullptr;
	}

private:
	struct stack_node
	{
	public:
		stack_node(const T &data)
			: m_data(std::make_shared<T>(data))
		{

		}
		std::shared_ptr<T> m_data = nullptr;
		std::shared_ptr<stack_node> m_next = nullptr;
	};

	std::shared_ptr<stack_node> m_head = nullptr;
};

int main()
{
	lock_free_stack<int> lfs;
	lfs.push(323);
	lfs.pop();

	return EXIT_SUCCESS;
}