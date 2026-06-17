#include <future>
#include <algorithm>
#include <numeric>
#include <vector>

template <typename Iterator, typename T>
T parallel_accmulate(Iterator first, Iterator last, T init)
{
	const unsigned long length = std::distance(first, last);
	const unsigned long max_chunk_size = 25;

	if (length <= max_chunk_size)
	{
		return std::accumulate(first, last, init);
	}

	Iterator mid_point = first;
	std::advance(mid_point, length / 2);
	std::future<T> first_half_result = std::async(parallel_accmulate<Iterator, T>, first, mid_point, init);
	T second_half_result = parallel_accmulate(mid_point, last, T());

	return first_half_result.get() + second_half_result;
}

int main()
{
	std::vector<int> vec(1000);
	std::iota(vec.begin(), vec.end(), 5);
	int i = 0;
	i = parallel_accmulate(vec.begin(), vec.end(), i);

	return EXIT_SUCCESS;
}