#include <future>
#include <algorithm>
#include <iostream>
#include <vector>
#include <numeric>

template <typename Iterator, typename Func>
void parallel_for_each(Iterator first, Iterator last, Func f)
{
	const unsigned long length = std::distance(first, last);
	if (length == 0)
	{
		return;
	}

	const unsigned long min_per_thread = 25;
	if (length < (2 * min_per_thread))
	{
		std::for_each(first, last, f);
	}
	else
	{
		auto mid_point = first;
		std::advance(mid_point, length / 2);
		std::future<void> first_half = std::async(&parallel_for_each<Iterator, Func>, first, mid_point, f);
		parallel_for_each(mid_point, last, f);
		first_half.get();
	}
}

void Test(int i)
{
	std::cout << i << " ";
}

int main()
{
	std::vector<int> vn(11312);
	std::iota(vn.begin(), vn.end(), 1);
	parallel_for_each(vn.cbegin(), vn.cend(), Test);

	return EXIT_SUCCESS;
}