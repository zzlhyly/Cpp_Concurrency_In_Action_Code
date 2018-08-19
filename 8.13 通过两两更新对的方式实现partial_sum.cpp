#include <thread>
#include <atomic>
#include <vector>
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

class barrier
{
public:
	barrier(unsigned count)
		: m_count(count), m_spaces(count), m_generation(0)
	{

	}

	void wait()
	{
		const unsigned gen = m_generation.load();
		--m_spaces;
		if (m_spaces == 0)
		{
			m_spaces = m_count.load();
			++m_generation;
		}
		else
		{
			while (gen == m_generation.load())
			{
				std::this_thread::yield();
			}
		}
	}

	void done_waiting()
	{
		--m_count;
		--m_spaces;
		if (m_spaces == 0)
		{
			m_spaces = m_count.load();
			++m_generation;
		}
	}

private:
	std::atomic<unsigned> m_count;
	std::atomic<unsigned> m_spaces;
	std::atomic<unsigned> m_generation;
};

template <typename Iterator>
void parallel_partial_sum(Iterator first, Iterator last)
{
	using value_type = typename Iterator::value_type;

	struct process_element
	{
		void operator()(Iterator first, Iterator last,
			std::vector<value_type> &buffer, unsigned i, barrier &b)
		{
			value_type &ith_element = *(first + i);
			bool update_source = false;

			for (unsigned step = 0, stride = 1; stride <= i; ++step, stride *= 2)
			{
				const value_type &source = (step % 2) ? buffer[i] : ith_element;
				value_type &dest = (step % 2) ? ith_element : buffer[i];
				const value_type &addend = (step % 2) ? buffer[i - stride] : *(first + i - stride);

				dest = source + addend;
				update_source = !(step % 2);
				b.wait();
			}

			if (update_source)
			{
				ith_element = buffer[i];
			}

			b.done_waiting();
		}
	};

	const unsigned long length = std::distance(first, last);
	if (length <= 1)
	{
		return;
	}

	std::vector<value_type> buffer(length);
	barrier b(length);
	std::vector<std::thread> threads(length - 1);
	join_threads jt(threads);

	Iterator block_start = first;
	for (unsigned long index = 0; index < (length - 1); ++index)
	{
		threads[index] = std::thread(process_element(), first, last,
			std::ref(buffer), index, std::ref(b));
	}

	process_element()(first, last, buffer, length - 1, b);
}

int main()
{
	std::vector<int> vn(300);
	std::iota(vn.begin(), vn.end(), 0);
	parallel_partial_sum(vn.begin(), vn.end());

	return 0;
}