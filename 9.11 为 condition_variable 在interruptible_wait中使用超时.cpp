#include <mutex>
#include <thread>
#include <future>
#include <condition_variable>
#include <atomic>

class interrupt_flag;
extern thread_local interrupt_flag this_thread_interrupt_flag;

class interrupt_flag
{
public:
	interrupt_flag() = default;
	void set()
	{
		m_flag.store(true, std::memory_order_relaxed);
		std::lock_guard<std::mutex> lk(m_set_clear_mutex);
		if (m_thread_cond != nullptr)
		{
			m_thread_cond->notify_all();
		}
	}
	
	bool is_set()const
	{
		return m_flag.load(std::memory_order_relaxed);
	}

	void set_condition_variable(std::condition_variable &cv)
	{
		std::lock_guard<std::mutex> lk(m_set_clear_mutex);
		m_thread_cond = &cv;
	}

	void clear_condition_variable()
	{
		std::lock_guard<std::mutex> lk(m_set_clear_mutex);
		m_thread_cond = nullptr;
	}

	struct clear_cv_on_destruct
	{
		~clear_cv_on_destruct()
		{
			this_thread_interrupt_flag.clear_condition_variable();
		}
	};

private:
	std::atomic<bool> m_flag;
	std::condition_variable *m_thread_cond = nullptr;
	std::mutex m_set_clear_mutex;
};

thread_local interrupt_flag this_thread_interrupt_flag;

class interruptible_thread
{
public:
	template<typename FunctionType>
	interruptible_thread(FunctionType f)
	{
		std::promise<interrupt_flag*> p;
		m_internal_thread = std::thread([f, &p] {
			p.set_value(&this_thread_interrupt_flag);
			f();
		});
		m_flag = p.get_future().get();
	}

	void join()
	{
		if (m_internal_thread.joinable())
		{
			m_internal_thread.join();
		}
	}

	void detach()
	{
		if (m_internal_thread.joinable())
		{
			m_internal_thread.detach();
		}
	}

	bool joinable()const
	{
		return m_internal_thread.joinable();
	}

	void interrupt()
	{
		if (m_flag != nullptr)
		{
			m_flag->set();
		}
	}
private:
	std::thread m_internal_thread;
	interrupt_flag *m_flag = nullptr;
};

void interruption_point()
{
	if (this_thread_interrupt_flag.is_set())
	{
		throw;
	}
}

void interruptible_wait(std::condition_variable &cv, std::unique_lock<std::mutex> &lk)
{
	interruption_point();
	this_thread_interrupt_flag.set_condition_variable(cv);
	interrupt_flag::clear_cv_on_destruct gu;
	interruption_point();
	cv.wait_for(lk, std::chrono::milliseconds(1));
	interruption_point();
}

template <typename Predicate>
void interruptible_wait(std::condition_variable &cv, std::unique_lock<std::mutex> &lk, Predicate pred)
{
	interruption_point();
	this_thread_interrupt_flag.set_condition_variable(cv);
	interrupt_flag::clear_cv_on_destruct gu;
	while (!this_thread_interrupt_flag.is_set() && !pred())
	{
		cv.wait_for(lk, std::chrono::milliseconds(1));
	}
	interruption_point();
}

int main()
{
	return 0;
}