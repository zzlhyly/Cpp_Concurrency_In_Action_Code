#include <future>
#include <thread>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>

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
protected:
private:
	std::vector<std::thread> &m_threads;
};

template <typename Iterator>
void parallel_partial_sum(Iterator first, Iterator last)
{
	using value_type = typename Iterator::value_type;

	struct process_chunk
	{
		void operator()(Iterator begin,
			Iterator last,
			std::future<value_type> *previous_end_value,
			std::promise<value_type> *end_value)
		{
			try
			{
				Iterator end = last;
				++end;
				std::partial_sum(begin, end, begin);
				if (previous_end_value != nullptr)
				{
					value_type add_end = previous_end_value->get();
					*last += add_end;
					if (end_value != nullptr)
					{
						end_value->set_value(*last);
					}

					std::for_each(begin, last, 
					[add_end](value_type &item)
					{
						item += add_end;
					});
				}
				else if (end_value != nullptr)
				{
					end_value->set_value(*last);
				}
			}
			catch (...)
			{
				if (end_value != nullptr)
				{
					end_value->set_exception(std::current_exception());
				}
				else
				{
					throw;
				}
			}
		}
	};

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
	std::vector<std::thread> threads(num_threads - 1);
	std::vector<std::promise<value_type>> end_values(num_threads - 1);
	std::vector<std::future<value_type>> previous_end_values;
	previous_end_values.reserve(num_threads - 1);
	join_threads jt(threads);

	Iterator block_start = first;
	for (unsigned long index = 0; index < (num_threads - 1); ++index)
	{
		auto block_last = block_start;
		std::advance(block_last, block_size - 1);
		threads[index] = std::thread(process_chunk(),
			block_start, block_last,
			(index != 0) ? &previous_end_values[index - 1] : nullptr, &end_values[index]);

		block_start = block_last;
		++block_start;
		previous_end_values.push_back(end_values[index].get_future());
	}

	auto final_element = block_start;
	std::advance(final_element, std::distance(block_start, last) - 1);
	process_chunk()(block_start, final_element,
		(num_threads > 1) ? &previous_end_values.back() : nullptr, nullptr);
}

int main()
{
	std::vector<int> vn(30000000);
	std::iota(vn.begin(), vn.end(), 1);

	auto time_begin = std::chrono::steady_clock::now();
	parallel_partial_sum(vn.begin(), vn.end());
	std::cout << (std::chrono::steady_clock::now() - time_begin).count() << std::endl;


	vn.swap(std::vector<int>(30000000));
	std::iota(vn.begin(), vn.end(), 1);

	time_begin = std::chrono::steady_clock::now();
	std::partial_sum(vn.begin(), vn.end(), vn.begin());
	std::cout << (std::chrono::steady_clock::now() - time_begin).count() << std::endl;

	return EXIT_SUCCESS;
}