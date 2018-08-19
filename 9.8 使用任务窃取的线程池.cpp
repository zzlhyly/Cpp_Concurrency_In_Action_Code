#include <atomic>
#include <memory>
#include <thread>
#include <queue>
#include <mutex>
#include <future>
#include <iostream>
#include <list>
#include <deque>
#include <vector>
#include <random>

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

class work_stealing_queue
{
public:
	work_stealing_queue() = default;
	work_stealing_queue(const work_stealing_queue&) = delete;
	work_stealing_queue &operator=(const work_stealing_queue&) = delete;

	void push(function_wrapper data)
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		m_deuqe.push_front(std::move(data));
	}

	bool empty() const
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		return m_deuqe.empty();
	}

	bool try_pop(function_wrapper &value)
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		if (m_deuqe.empty())
		{
			return false;
		}

		value = std::move(m_deuqe.front());
		m_deuqe.pop_front();
		return true;
	}

	bool try_steal(function_wrapper &value)
	{
		std::lock_guard<std::mutex> lk(m_mutex);
		if (m_deuqe.empty())
		{
			return false;
		}

		value = std::move(m_deuqe.back());
		m_deuqe.pop_back();
		return true;
	}

private:
	std::deque<function_wrapper> m_deuqe;
	mutable std::mutex m_mutex;
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
				m_queues.push_back(std::make_unique<work_stealing_queue>());
			}
			for (unsigned index = 0; index < thread_count; ++index)
			{
				m_threads.push_back(std::thread(&thread_pool::work_thread, this, index));
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

		if (pop_task_from_local_queue(task)
			|| pop_task_from_pool_queue(task)
			|| pop_task_from_other_thread_queue(task))
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
	std::vector<std::unique_ptr<work_stealing_queue>> m_queues;
	std::vector<std::thread> m_threads;
	join_threads m_joiner;
	static thread_local work_stealing_queue* sm_local_work_queue;
	static thread_local unsigned sm_my_index;

	void work_thread(unsigned my_index)
	{
		sm_my_index = my_index;
		sm_local_work_queue = m_queues[my_index].get();
		while (!m_done)
		{
			run_pending_task();
		}
	}

	bool pop_task_from_local_queue(function_wrapper &value)
	{
		return (sm_local_work_queue != nullptr) && sm_local_work_queue->try_pop(value);
	}

	bool pop_task_from_pool_queue(function_wrapper &value)
	{
		return m_pool_work_queue.try_pop(value);
	}

	bool pop_task_from_other_thread_queue(function_wrapper &value)
	{
		for (unsigned index = 0; index < m_queues.size(); ++index)
		{
			const unsigned tmp = (sm_my_index + index + 1) % m_queues.size();
			if (m_queues[tmp]->try_pop(value))
			{
				return true;
			}
		}

		return false;
	}
};

thread_local work_stealing_queue* thread_pool::sm_local_work_queue = nullptr;
thread_local unsigned thread_pool::sm_my_index = 0;

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
	std::list<int> ln;
	std::default_random_engine dre(time(nullptr));
	std::uniform_int_distribution<> dis(1, 3000);
	for (int index = 0; index < 300; ++index)
	{
		ln.push_back(dis(dre));
	}
	
	auto tmp = parallel_quick_sort(ln);
	for (auto data : tmp)
	{
		std::cout << data << " ";
	}

	return 0;
}