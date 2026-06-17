#include <atomic>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <functional>
#include <iostream>

class join_threads
{
public:
	join_threads(std::vector<std::thread> &threads)
		: m_threads(threads)
	{

	}

	~join_threads()
	{
		for (auto &t : m_threads)
		{
			if (t.joinable())
			{
				t.join();
			}
		}
	}
protected:
private:
	std::vector<std::thread> &m_threads;
};

template <typename T>
class thread_safe_queue
{
public:
	thread_safe_queue() = default;
	~thread_safe_queue() = default;

	void push(T data)
	{
		std::lock_guard<std::mutex> lk(m_mx);
		m_queue.push(std::move(data));
	}

	bool try_pop(T &data)
	{
		std::lock_guard<std::mutex> lk(m_mx);
		if (m_queue.empty())
		{
			return false;
		}
		data = std::move(m_queue.front());
		m_queue.pop();
		return true;
	}

	bool empty()
	{
		std::lock_guard<std::mutex> lk(m_mx);
		return m_queue.empty();
	}

private:
	std::queue<T> m_queue;
	std::mutex m_mx;
};

class thread_pool
{
public:
	thread_pool()
		: m_done(false), m_joiner(m_threads)
	{
		const unsigned thread_count = std::thread::hardware_concurrency();
		try
		{
			for (unsigned index = 0; index < thread_count; ++index)
			{
				m_threads.push_back(std::thread(&thread_pool::work_thread, this));
			}
		}
		catch (...)
		{
			m_done = true;
			throw;
		}
	}

	~thread_pool()
	{
		m_done = true;
	}

	template <typename FunctionType>
	void submit(FunctionType f)
	{
		m_work_queue.push(std::function<void()>(f));
	}

private:
	std::atomic<bool> m_done;
	thread_safe_queue<std::function<void()>> m_work_queue;
	std::vector<std::thread> m_threads;
	join_threads m_joiner;

	void work_thread()
	{
		while (!m_done)
		{
			std::function<void()> task;
			if (m_work_queue.try_pop(task))
			{
				task();
			}
			else
			{
				std::this_thread::yield();
			}
		}
	}
};

void Test()
{
	std::cout << "test" << std::endl;
}

int main()
{
	thread_pool tp;
	tp.submit(Test);
	tp.submit(Test);
	tp.submit(Test);
	tp.submit(Test);
	tp.submit(Test);
	tp.submit(Test);
	tp.submit(Test);
	tp.submit(Test);
	tp.submit(Test);
	tp.submit(Test);
	tp.submit(Test);
	tp.submit(Test);
	tp.submit(Test);
	tp.submit(Test);
	tp.submit(Test);

	std::this_thread::sleep_for(std::chrono::seconds(20));
	return 0;
}