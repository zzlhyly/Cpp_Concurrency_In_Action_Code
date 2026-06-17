#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>

template <typename T>
class threadsafe_queue
{
public:
	threadsafe_queue() = default;
	void push(T new_value)
	{
		std::lock_guard<std::mutex> lk(m_mx);
		m_data_queue.push(std::move(new_value));
		m_data_cond.notify_one();
	}

	void wait_for_pop(T &value)
	{
		std::unique_lock<std::mutex> ul(m_mx);
		m_data_cond.wait(ul, [this] { return !m_data_queue.empty(); });
		value = std::move(m_data_queue.front());
		m_data_queue.pop();
	}

	std::shared_ptr<T> wait_for_pop()
	{
		std::unique_lock<std::mutex> ul(m_mx);
		m_data_cond.wait(ul, [this] { return !m_data_queue.empty(); });
		std::shared_ptr<T> res(std::make_shared<T>(std::move(m_data_queue.front())));
		m_data_queue.pop();
		return res;
	}

	bool try_pop(T &value)
	{
		std::lock_guard<std::mutex> lk(m_mx);
		if (m_data_queue.empty())
		{
			return false;
		}

		value = std::move(m_data_queue.front());
		m_data_queue.pop();
		return true;
	}

	std::shared_ptr<T> try_pop()
	{
		std::lock_guard<std::mutex> lk(m_mx);
		if (m_data_queue.empty())
		{
			return nullptr;
		}

		std::shared_ptr<T> res(std::make_shared<T>(std::move(m_data_queue.front())));
		return res;
	}

	bool empty() const
	{
		std::lock_guard<std::mutex> lk(m_mx);
		return m_data_queue.empty();
	}
private:
	mutable std::mutex m_mx;
	std::queue<T> m_data_queue;
	std::condition_variable m_data_cond;
};

int main()
{
	threadsafe_queue<int> tq;
	tq.empty();
	tq.push(333);
	int i = 0;
	tq.try_pop(i);
	tq.wait_for_pop(i);
	return EXIT_SUCCESS;
}