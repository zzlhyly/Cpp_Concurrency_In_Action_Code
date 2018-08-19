#include <memory>
#include <queue>
#include <condition_variable>
#include <mutex>

template <typename T>
class threadsafe_queue
{
public:
	threadsafe_queue() = default;
	void wait_for_pop(T &value)
	{
		std::unique_lock<std::mutex> ul(m_mx);
		m_data_cond.wait(ul, [this] { return !m_data_queue.empty(); });
		value = std::move(*m_data_queue.front());
		m_data_queue.pop();
	}

	bool try_pop(T &value)
	{
		std::lock_guard<std::mutex> lk(m_mx);
		if (m_data_queue.empty())
		{
			return false;
		}

		value = std::move(*m_data_queue.front());
		m_data_queue.pop();
		return true;
	}

	std::shared_ptr<T> wait_for_pop()
	{
		std::unique_lock<std::mutex> ul(m_mx);
		m_data_cond.wait(ul, [this] { return !m_data_queue.empty(); });
		std::shared_ptr<T> res = m_data_queue.front();
		m_data_queue.pop();
		return res;
	}

	std::shared_ptr<T> try_pop()
	{
		std::lock_guard<std::mutex> lk(m_mx);
		if (m_data_queue.empty())
		{
			return nullptr;
		}

		std::shared_ptr<T> res = m_data_queue.front();
		m_data_queue.pop();
		return res;
	}

	void push(T new_value)
	{
		std::shared_ptr<T> spData(std::make_shared<T>(std::move(new_value)));
		std::lock_guard<std::mutex> lk(m_mx);
		m_data_queue.push(spData);
		m_data_cond.notify_one();
	}


	bool empty() const
	{
		std::lock_guard<std::mutex> lk(m_mx);
		return m_data_queue.empty();
	}
private:
	mutable std::mutex m_mx;
	std::queue<std::shared_ptr<T>> m_data_queue;
	std::condition_variable m_data_cond;
};

int main()
{
	threadsafe_queue<int> tq;
	tq.empty();
	tq.push(2);
	int i = 0;
	tq.try_pop();
	tq.try_pop(i);
	tq.wait_for_pop();
	tq.wait_for_pop(i);

 	return EXIT_SUCCESS;
}