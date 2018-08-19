#include <memory>
#include <mutex>
template <typename T>
class threadsafe_queue
{
public:
	threadsafe_queue()
		: m_head(std::make_unique<queue_node>()), m_tail(m_head.get())
	{

	}
	threadsafe_queue(const threadsafe_queue&) = delete;
	threadsafe_queue &operator=(const threadsafe_queue&) = delete;
	~threadsafe_queue() = default;

	std::shared_ptr<T> try_pop()
	{
		std::unique_ptr<queue_node> old_head = pop_head();
		return old_head != nullptr ? old_head->m_data : std::make_shared<T>();
	}

	void push(T new_value)
	{
		std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
		std::unique_ptr<queue_node> p(std::make_unique<queue_node>());
		queue_node *const new_tail = p.get();
		std::lock_guard<std::mutex> lk(m_tail_mutex);
		m_tail->m_data = new_data;
		m_tail->m_next = std::move(p);
		m_tail = new_tail;
	}

private:
	struct queue_node
	{
		std::shared_ptr<T> m_data;
		std::unique_ptr<queue_node> m_next;
	};
	std::mutex m_head_mutex;
	std::unique_ptr<queue_node> m_head;
	std::mutex m_tail_mutex;
	queue_node *m_tail;

private:
	queue_node * get_tail()
	{
		std::lock_guard<std::mutex> lk(m_tail_mutex);
		return m_tail;
	}

	std::unique_ptr<queue_node> pop_head()
	{
		std::lock_guard<std::mutex> lk(m_head_mutex);
		if (m_head.get() == get_tail())
		{
			return nullptr;
		}

		std::unique_ptr<queue_node> old_head = std::move(m_head);
		m_head = std::move(old_head->m_next);
		return old_head;
	}
};

int main()
{
	threadsafe_queue<int> tq;
	tq.push(333);
	tq.try_pop();

	return EXIT_SUCCESS;
}