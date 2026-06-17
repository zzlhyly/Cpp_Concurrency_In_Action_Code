#include <stack>
#include <mutex>
#include <memory>
#include <future>
#include <list>
#include <vector>
#include <thread>
#include <atomic>
#include <iostream>

template <typename T>
class threadsafe_stack
{
public:
	threadsafe_stack() = default;
	threadsafe_stack(const threadsafe_stack &other)
	{
		std::lock_guard<std::mutex> lk(other.m_mx);
		m_data = other.m_data;
	}
	threadsafe_stack &operator=(const threadsafe_stack&) = delete;

	void push(T new_value)
	{
		std::lock_guard<std::mutex> lk(m_mx);
		m_data.push(std::move(new_value));
	}

	std::shared_ptr<T> pop()
	{
		std::lock_guard<std::mutex> lk(m_mx);
		if (m_data.empty())
		{
			return nullptr;
		}

		const std::shared_ptr<T> res(std::make_shared<T>(std::move(m_data.top())));
		m_data.pop();
		return res;
	}

	void pop(T &value)
	{
		std::lock_guard<std::mutex> lk(m_mx);
		if (m_data.empty())
		{
			return;
		}
		value = std::move(m_data.top());
		m_data.pop();
	}

	bool empty() const
	{
		std::lock_guard<std::mutex> lk(m_mx);
		return m_data.empty();
	}

private:
	std::stack<T> m_data;
	mutable std::mutex m_mx;
};

template <typename T>
struct sorter
{
public:

	struct chunk_to_sort
	{
	public:
		std::list<T> m_data;
		std::promise<std::list<T>> m_promise;
	};

	threadsafe_stack<chunk_to_sort> m_chunks;
	std::vector<std::thread> m_threads;
	const unsigned m_max_thread_count;
	std::atomic<bool> m_end_of_data;

	sorter()
		: m_max_thread_count(std::thread::hardware_concurrency() - 1), m_end_of_data(false)
	{

	}

	~sorter()
	{
		m_end_of_data = true;

		for (size_t index = 0; index < m_threads.size(); ++index)
		{
			m_threads[index].join();
		}
	}

	void try_sort_chunk()
	{
		std::shared_ptr<chunk_to_sort> chunk = m_chunks.pop();
		if (chunk != nullptr)
		{
			sort_chunk(chunk);
		}
	}

	std::list<T> do_sort(std::list<T> &chunk_data)
	{
		if (chunk_data.size() <= 1)
		{
			return chunk_data;
		}

		std::list<T> result;
		result.splice(result.begin(), chunk_data, chunk_data.begin());

		const T &partition_value = *result.begin();

		typename std::list<T>::iterator divide_it = std::partition(chunk_data.begin(), chunk_data.end(), 
			[&](const T &val) { return val < partition_value; });

		chunk_to_sort new_lower_chunk;
		new_lower_chunk.m_data.splice(new_lower_chunk.m_data.end(), chunk_data, chunk_data.begin(), divide_it);

		std::future<std::list<T>> new_lower = new_lower_chunk.m_promise.get_future();
		m_chunks.push(std::move(new_lower_chunk));
		if (m_threads.size() < m_max_thread_count)
		{
			std::cout << "new_thread" << std::endl;
			m_threads.push_back(std::thread(&sorter<T>::sort_thread, this));
		}

		std::list<T> new_higher(do_sort(chunk_data));
		result.splice(result.end(), new_higher);
		while (new_lower.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		{
			try_sort_chunk();
		}

		result.splice(result.begin(), new_lower.get());

		return result;
	}

	void sort_chunk(std::shared_ptr<chunk_to_sort> const & chunk)
	{
		chunk->m_promise.set_value(do_sort(chunk->m_data));
	}

	void sort_thread()
	{
		while (!m_end_of_data)
		{
			try_sort_chunk();
			std::this_thread::yield();
		}
	}
};

template <typename T>
std::list<T> parallel_quick_sort(std::list<T> input)
{
	if (input.size() <= 1)
	{
		return input;
	}

	sorter<T> s;
	return s.do_sort(input);
}

int main()
{
	std::list<int> ln{ 12, 3, 123, 21, 3, 21, 23, 4, 32, 4, 23, 4, 35, 4, 6, 65, 7, 6, 6, 5, 2, 42, 3, 21, 3, 14, 2, 43, 46, 54, 6, 45 };
	ln.swap(parallel_quick_sort(ln));

	for(auto data : ln)
	{
		std::cout << data << " ,";
	}
	std::cout << std::endl;

	return EXIT_SUCCESS;
}