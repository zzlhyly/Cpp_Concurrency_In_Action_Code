#include <atomic>
#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <future>
#include <vector>
#include <iostream>
#include <list>

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

	bool empty()
	{
		std::lock_guard<std::mutex> lk(m_mx);
		return m_queue.empty();
	}

protected:
private:
	std::queue<T> m_queue;
	mutable std::mutex m_mx;
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
		if (sm_local_work_queue != nullptr)
		{
			sm_local_work_queue->push(std::move(task));
		}
		else
		{
			m_pool_work_queue.push(std::move(task));
		}
		
		return res;
	}

	void run_pending_task()
	{
		function_wrapper task;

		if ((sm_local_work_queue != nullptr)
			&& !sm_local_work_queue->empty())
		{
			task = std::move(sm_local_work_queue->front());
			sm_local_work_queue->pop();
			task();
		}
		else if (m_pool_work_queue.try_pop(task))
		{
			task();
		}
		else
		{
			std::this_thread::yield();
		}
	}

private:
	std::atomic<bool> m_done;
	thread_safe_queue<function_wrapper> m_pool_work_queue;
	static thread_local std::unique_ptr<std::queue<function_wrapper>> sm_local_work_queue;
	std::vector<std::thread> m_threads;
	join_threads m_joiner;

	void work_thread()
	{
		sm_local_work_queue.reset(new std::queue<function_wrapper>);
		while (!m_done)
		{
			run_pending_task();
		}
	}
};

thread_local std::unique_ptr<std::queue<function_wrapper>> thread_pool::sm_local_work_queue = nullptr;

template <typename T>
struct sorter
{
	thread_pool m_tp;

	std::list<T> do_sort(std::list<T> &chunk_data)
	{
		if (chunk_data.empty())
		{
			return chunk_data;
		}

		std::list<T> result;
		result.splice(result.begin(), chunk_data, chunk_data.begin());
		const T &partition_val = *result.begin();

		typename std::list<T>::iterator divide_it = std::partition(chunk_data.begin(), chunk_data.end(),
			[&](const T &val) { return val < partition_val; });
		
		std::list<T> new_lower_chunk;
		new_lower_chunk.splice(new_lower_chunk.end(), chunk_data, chunk_data.begin(), divide_it);
		std::future<std::list<T>> new_lower = m_tp.submit(std::bind(&sorter::do_sort, this, std::move(new_lower_chunk)));
		std::list<T> new_higher(do_sort(chunk_data));
		result.splice(result.end(), new_higher);

		while (!(new_lower.wait_for(std::chrono::seconds(0)) == std::future_status::ready))
		{
			m_tp.run_pending_task();
		}

		result.splice(result.begin(), new_lower.get());
		return result;
	}
};

template <typename T>
std::list<T> parallel_quick_sort(std::list<T> input)
{
	if (input.empty())
	{
		return input;
	}

	sorter<T> s;
	return s.do_sort(input);
}

int main()
{
	std::list<int> ln{ 24, 34, 324, 23, 4, 24, 2, 4, 2, 4, 25, 3, 5, 546, 8, 67, 8, 78, 78, 980, 4, 345, 7, 79, 765, 62, 34, 2346, 0, 98, 77, 32, 34, 89, 67, 85, 45, 234, 4, 6, 8, 6, 54, 3, 7, 87, 9, 65, 45, 532, 4, 3, 4 };
	
	auto tmp = parallel_quick_sort(ln);
	for (auto data : tmp)
	{
		std::cout << data << " ";
	}

	return 0;
}