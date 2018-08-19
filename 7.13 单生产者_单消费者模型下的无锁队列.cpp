#include <memory>
#include <atomic>

template <typename T>
class	lock_free_queue
{
public:
	lock_free_queue()
		: m_head(new queue_node()), m_tail(m_head.load())
	{

	}

	lock_free_queue(const lock_free_queue&) = delete;
	lock_free_queue &operator=(const lock_free_queue&) = delete;

	~lock_free_queue()
	{
		while (queue_node *const old_head = m_head.load())
		{
			m_head.store(old_head->m_next);
			delete old_head;
		}
	}

	void push(T new_value)
	{
		std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));

		queue_node *p = new queue_node();
		queue_node *const old_tail = m_tail.load();
		old_tail->m_data.swap(new_data);
		old_tail->m_next = p;
		m_tail.store(p);
	}

	std::shared_ptr<T> pop()
	{
		queue_node *old_head = pop_head();
		if (old_head == nullptr)
		{
			return nullptr;
		}

		std::shared_ptr<T> const res(old_head->m_data);
		delete old_head;
		return res;
	}

private:
	struct queue_node
	{
		std::shared_ptr<T> m_data = nullptr;
		queue_node *m_next = nullptr;
	};

	std::atomic<queue_node*> m_head;
	std::atomic<queue_node*> m_tail;

	queue_node *pop_head()
	{
		queue_node *const old_head = m_head.load();
		if (old_head == m_tail.load())
		{
			return nullptr;
		}

		m_head.store(old_head->m_next);
		return old_head;
	}
};

int main()
{
	lock_free_queue<int> lfq;
	lfq.push(13123);
	lfq.pop();

	return EXIT_SUCCESS;
}