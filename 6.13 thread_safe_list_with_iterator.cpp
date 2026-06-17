#include <mutex>
#include <memory>
#include <algorithm>

template <typename T>
class threadsafe_list
{
public:
	threadsafe_list()
	{

	}

	~threadsafe_list()
	{
		remove_if([](const list_node&) { return true; });
	}

	threadsafe_list(const threadsafe_list&) = delete;
	threadsafe_list &operator=(const threadsafe_list&) = delete;
	void push_front(const T &value)
	{
		std::unique_ptr<list_node> new_node(std::make_unique<list_node>(value));
		std::lock_guard<std::mutex> lk(m_head.m_mx);
		new_node->m_next = std::move(m_head.m_next);
		m_head.m_next = std::move(new_node);
	}

	template <typename Function>
	void for_each(Function f)
	{
		list_node *current = &m_head;
		std::unique_lock<std::mutex> uk(m_head.m_mx);
		while (list_node *const next = current->m_next.get())
		{
			std::unique_lock<std::mutex> next_uk(next->m_mx);
			uk.unlock();
			f(*next->m_data);
			current = next;
			uk = std::move(next_uk);
		}
	}

	template <typename Predicate>
	std::shared_ptr<T> find_first_if(Predicate p)
	{
		list_node *current = &m_head;
		std::unique_lock<std::mutex> uk(m_head.m_mx);
		while (list_node *next = current->m_next.get())
		{
			std::unique_lock<std::mutex> next_uk(next->m_mx);
			uk.unlock();
			if (p(*next->m_data))
			{
				return next->m_data;
			}
			current = next;
			uk = std::move(next_uk);
		}

		return nullptr;
	}

	template <typename Predicate>
	void remove_if(Predicate p)
	{
		list_node *current = &m_head;
		std::unique_lock<std::mutex> uk(m_head.m_mx);
		while (list_node *next = current->m_next.get())
		{
			std::unique_lock<std::mutex> next_uk(next->m_mx);
			if (p(*next->m_data))
			{
				std::unique_ptr<list_node> old_next = std::move(current->m_next);
				current->m_next = std::move(next->m_next);
				next_uk.unlock();
			}
			else
			{
				uk.unlock();
				current = next;
				uk = std::move(next_uk);
			}
		}
	}
private:
	struct list_node
	{
		list_node()
			: m_next()
		{

		}

		list_node(const T & value)
			: m_data(std::make_shared<T>(value))
		{

		}
		std::mutex m_mx;
		std::shared_ptr<T> m_data;
		std::unique_ptr<list_node> m_next;

	};

	list_node m_head;
};

void Test(int i)
{

}

bool bTest(int i)
{
	return true;
}

int main()
{
	threadsafe_list<int> tl;
	tl.push_front(2);
	tl.push_front(3);
	tl.push_front(4);
	tl.push_front(5);

	tl.for_each(Test);
	tl.find_first_if(bTest);
	tl.remove_if(bTest);

	return EXIT_SUCCESS;
}