
#include <future>
#include <exception>
#include <atomic>
#include <vector>
#include <numeric>

class join_threads
{
public:
	explicit join_threads(std::vector<std::thread> & threads)
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

template<typename Iterator, typename MatchType>
Iterator parallel_find(Iterator first, Iterator last, MatchType match)
{
	struct find_element
	{
		void operator()(Iterator begin, Iterator end, MatchType match, std::promise<Iterator> * result, std::atomic<bool> * done_flag)
		{
			try
			{
				for (; (begin != end) && (!done_flag->load()); ++begin)
				{
					if (*begin == match)
					{
						if (!done_flag->load())
						{
							done_flag->store(true);
							result->set_value(begin);
						}
						return;
					}
				}
			}
			catch (...)
			{
				try
				{
					done_flag->store(true);
					result->set_exception(std::current_exception());
				}
				catch (...)
				{

				}
			}
		}

	};

	const unsigned long length = std::distance(first, last);
	if (length == 0)
	{
		return last;
	}

	const unsigned long min_per_thread = 25;
	const unsigned long max_threads = (length + min_per_thread - 1) / min_per_thread;
	const unsigned long hardware_threads = std::thread::hardware_concurrency();
	const unsigned long num_threads = std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);
	const unsigned long block_size = length / num_threads;
	std::promise<Iterator> result;
	std::atomic<bool> done_flag(false);
	std::vector<std::thread> threads(num_threads - 1);

	{
		join_threads jt(threads);
		Iterator block_start = first;
		for (unsigned long index = 0; index < (num_threads - 1); ++index)
		{
			Iterator block_end = block_start;
			std::advance(block_end, block_size);
			threads[index] = std::thread(find_element(), block_start, block_end, match, &result, &done_flag);
			block_start = block_end;
		}
		find_element()(block_start, last, match, &result, &done_flag);
	}

	if (!done_flag.load())
	{
		return last;
	}

	return result.get_future().get();
}

int main()
{
	std::vector<int> vn(23423, 0);
	std::iota(vn.begin(), vn.end(), 1);
	auto it = parallel_find(vn.cbegin(), vn.cend(), 3453);

	return EXIT_SUCCESS;
}