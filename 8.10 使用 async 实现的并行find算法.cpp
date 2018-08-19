#include <future>
#include <atomic>
#include <vector>
#include <numeric>

template <typename Iterator, typename MatchType>
Iterator parallel_find_impl(Iterator first, Iterator last, MatchType match, std::atomic<bool> &done)
{
	try
	{
		const unsigned long length = std::distance(first, last);
		const unsigned long min_per_thread = 25;
		if (length < (2 * min_per_thread))
		{
			for (; (first != last) && (!done.load()); ++first)
			{
				if (*first == match)
				{
					if (!done.load())
					{
						done.store(true);
						return first;
					}
				}
			}

			return last;
		}
		else
		{
			auto mid_point = first;
			std::advance(mid_point, length / 2);
			std::future<Iterator> async_result = std::async(&parallel_find_impl<Iterator, MatchType>, mid_point, last, match, std::ref(done));
			auto direct_result = parallel_find_impl(first, mid_point, match, done);

			return (direct_result == mid_point) ? async_result.get() : direct_result;
		}
	}
	catch (...)
	{
		done.store(true);
		throw;
	}
}

template <typename Iterator, typename MatchType>
Iterator parallel_find(Iterator first, Iterator last, MatchType match)
{
	std::atomic<bool> done(false);
	return parallel_find_impl(first, last, match, done);
}

int main()
{
	std::vector<int> vn(12312, 0);
	std::iota(vn.begin(), vn.end(), 1);
	auto it = parallel_find(vn.cbegin(), vn.cend(), 234);

	return EXIT_SUCCESS;
}