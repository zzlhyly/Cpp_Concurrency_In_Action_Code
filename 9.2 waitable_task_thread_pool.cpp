#include <atomic>
#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <future>
#include <vector>
#include <iostream>
#include <numeric>

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

	bool try_pop(T &value)
	{
		std::lock_guard<std::mutex> lk(m_mx);
		if (m_queue.empty())
		{
			return false;
		}
		value = std::move(m_queue.front());
		m_queue.pop();
		return true;
	}

protected:
private:
	std::queue<T> m_queue;
	std::mutex m_mx;
};

class function_wrapper
{
public:
	function_wrapper() = default;
	template <typename F>
	function_wrapper(F &&f)
		: m_impl(std::make_unique<impl_type<F>>(std::move(f)))
	{

	}

	function_wrapper(function_wrapper &&other)
		: m_impl(std::move(other.m_impl))
	{

	}

	function_wrapper &operator=(function_wrapper &&other)
	{
		m_impl = std::move(other.m_impl);
		return *this;
	}

	function_wrapper(const function_wrapper&) = delete;
	function_wrapper &operator=(const function_wrapper&) = delete;

	void operator()()
	{
		m_impl->call();
	}

private:
	struct impl_base
	{
	public:
		virtual ~impl_base()
		{

		}
		virtual void call() = 0;
	};

	std::unique_ptr<impl_base> m_impl;

	template <typename F>
	struct impl_type : public impl_base
	{
	public:
		impl_type(F &&f)
			: m_f(std::move(f))
		{

		}

		void call()
		{
			m_f();
		}
			
		F m_f;
	};
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
	std::future<typename std::result_of<FunctionType()>::type> submit(FunctionType f)
	{
		using result_type = typename std::result_of<FunctionType()>::type;
		std::packaged_task<result_type()> task(std::move(f));
		std::future<result_type> res(task.get_future());
		m_work_queue.push(std::move(task));
		return res;
	}

private:
	std::atomic<bool> m_done;
	thread_safe_queue<function_wrapper> m_work_queue;
	std::vector<std::thread> m_threads;
	join_threads m_joiner;

	void work_thread()
	{
		while (!m_done)
		{
			function_wrapper task;
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

template <typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init)
{
	const unsigned long length = std::distance(first, last);
	if (length == 0)
	{
		return init;
	}

	const unsigned long block_size = 25;
	const unsigned long num_blocks = (length + block_size - 1) / block_size;
	std::vector<std::future<T>> futures(num_blocks - 1);
	thread_pool tp;

	Iterator block_start = first;
	for (unsigned long index = 0; index < (num_blocks - 1); ++index)
	{
		auto block_end = block_start;
		std::advance(block_end, block_size);
		futures[index] = tp.submit(std::bind(std::accumulate<Iterator, T>, block_start, block_end, T()));
		block_start = block_end;
	}
	T last_result = std::accumulate(block_start, last, T());


	T result = init;
	for (unsigned long index = 0; index < (num_blocks - 1); ++index)
	{
		result += futures[index].get();
	}
	result += last_result;

	return result;
}

int main()
{
	std::vector<int> vn(800, 0);
	std::iota(vn.begin(), vn.end(), 0);
	std::cout << std::accumulate(vn.begin(), vn.end(), 0) << std::endl;
	std::cout << parallel_accumulate(vn.begin(), vn.end(), 0) << std::endl;

	return 0;
}