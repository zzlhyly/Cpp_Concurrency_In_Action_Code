#include <memory>
template <typename T>
class zzlQueue
{
public:
	zzlQueue() = default;
	zzlQueue(const zzlQueue&) = delete;
	zzlQueue &operator=(const zzlQueue&) = delete;

	std::shared_ptr<T> try_pop()
	{
		if (m_head == nullptr)
		{
			return nullptr;
		}

		std::shared_ptr<T> const res(std::make_shared<T>(std::move(m_head->m_data)));
		m_head = std::move(m_head->m_next);
		return res;
	}

	void push(T new_value)
	{
		std::unique_ptr<QueueNode> p(std::make_unique<QueueNode>(std::move(new_value)));
		QueueNode* const new_tail = p.get();

		if (m_tail != nullptr)
		{
			m_tail->m_next = std::move(p);
		}
		else
		{
			m_head = std::move(p);
		}
		m_tail = new_tail;
	}

private:
	struct QueueNode
	{
		QueueNode(T data)
			: m_data(std::move(data))
		{

		}

		T m_data;
		std::unique_ptr<QueueNode> m_next;
	};

	std::unique_ptr<QueueNode> m_head;
	QueueNode *m_tail;
};

int main()
{
	zzlQueue<int> zq;
	zq.push(333);
	zq.try_pop();

	return EXIT_SUCCESS;
}