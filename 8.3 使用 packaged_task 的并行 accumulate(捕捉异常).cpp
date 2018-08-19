#include <future>
#include <thread>
#include <numeric>
#include <algorithm>
#include <vector>

template <typename Iterator, typename T>
struct accumulate_block
{
	T operator()(Iterator first, Iterator last)
	{
		return std::accumulate(first, last, T());
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

	const unsigned long min_per_thread = 25;
	const unsigned long max_threads = (length + min_per_thread - 1) / min_per_thread;
	const unsigned long	hardware_threads = std::thread::hardware_concurrency();
	const unsigned long num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
	const unsigned long block_size = length / num_threads;
	std::vector<std::future<T>> futures(num_threads - 1);
	std::vector<std::thread> threads(num_threads - 1);

	accumulate_block<Iterator, T> ab;
	Iterator block_start = first;
	for (unsigned long i = 0; i < (num_threads - 1); ++i)
	{
		Iterator block_end = block_start;
		std::advance(block_end, block_size);
		std::packaged_task<T(Iterator, Iterator)> task(ab);
		futures[i] = task.get_future();
		threads[i] = std::thread(std::move(task), block_start, block_end);
		block_start = block_end;
	}

	T result = init;
	try
	{
		for (unsigned long i = 0; i < (num_threads - 1); ++i)
		{
			result += futures[i].get();
		}
		T last_result = ab(block_start, last);
		result += last_result;
		std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
	}
	catch (...)
	{
		for (unsigned long i = 0; i < (num_threads - 1); ++i)
		{
			if (threads[i].joinable())
			{
				threads[i].join();
			}
		}

		throw;
	}

	return result;
}

int main()
{
	std::vector<int> vn(10000000);
	std::iota(vn.begin(), vn.end(), 0);
	int i = 0;
	i = parallel_accumulate(vn.begin(), vn.end(), i);

	return EXIT_SUCCESS;
}