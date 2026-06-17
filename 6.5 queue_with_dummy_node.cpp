#include <memory>

template <typename T>
class zzlQueue
{
public:
	zzlQueue()
		: m_head(new QueueNode()), m_tail(m_head.get())
	{

	}
	zzlQueue(const zzlQueue&) = delete;
	zzlQueue &operator=(const zzlQueue&) = delete;

	std::shared_ptr<T> try_pop()
	{
		if (m_head.get() == m_tail)
		{
			return nullptr;
		}

		std::shared_ptr<T> const res(m_head->m_data);
		std::unique_ptr<QueueNode> old_head = std::move(m_head);
		m_head = std::move(old_head->m_next);
		return res;
	}

	void push(T new_value)
	{
		std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
		std::unique_ptr<QueueNode> p(new QueueNode());
		m_tail->m_data = new_data;
		QueueNode* const new_tail = p.get();
		m_tail->m_next = std::move(p);
		m_tail = new_tail;
	}

private:
	struct QueueNode
	{
		std::shared_ptr<T> m_data;
		std::unique_ptr<QueueNode> m_next;
	};
	std::unique_ptr<QueueNode> m_head;
	QueueNode* m_tail;
};

int main()
{
	zzlQueue<int> zq;
	zq.push(4);
	zq.try_pop();

	return EXIT_SUCCESS;
}