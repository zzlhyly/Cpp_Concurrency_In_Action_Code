#include <future>
#include <thread>
#include <algorithm>
#include <vector>
#include <iostream>
#include <numeric>

class join_threads
{
public:
	explicit join_threads(std::vector<std::thread> &threads)
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

template <typename Iterator, typename Func>
void parallel_for_each(Iterator first, Iterator last, Func f)
{
	const unsigned long length = std::distance(first, last);
	if (length == 0)
	{
		return;
	}

	const unsigned long min_per_thread = 25;
	const unsigned long max_threads = (length + min_per_thread - 1) / min_per_thread;
	const unsigned long hardware_threads = std::thread::hardware_concurrency();
	const unsigned long num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
	const unsigned long block_size = length / num_threads;
	std::vector<std::future<void>> futures(num_threads - 1);
	std::vector<std::thread> threads(num_threads - 1);
	join_threads jt(threads);

	Iterator block_start = first;
	for (unsigned long index = 0; index < (num_threads - 1); ++index)
	{
		Iterator block_end = block_start;
		std::advance(block_end, block_size);
		std::packaged_task<void(void)> task([=]() { std::for_each(block_start, block_end, f); });
		futures[index] = task.get_future();
		threads[index] = std::thread(std::move(task));
		block_start = block_end;
	}
	std::for_each(block_start, last, f);

	for (unsigned long index = 0; index < (num_threads - 1); ++index)
	{
		futures[index].get();
	}
}

void Test(int i)
{
	std::cout << i << " ";
}

int main()
{
	std::vector<int> vn(2131);
	std::iota(vn.begin(), vn.end(), 1);
	parallel_for_each(vn.cbegin(), vn.cend(), Test);

	return EXIT_SUCCESS;
}