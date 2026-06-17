#include <memory>
#include <mutex>
#include <condition_variable>
#include <future>
#include <cassert>

template <typename T>
class threadsafe_queue
{
public:
	threadsafe_queue()
		: m_upHead(std::make_unique<queue_node>()), m_pTail(m_upHead.get())
	{

	}

	threadsafe_queue(const threadsafe_queue&) = delete;
	threadsafe_queue &operator=(const threadsafe_queue&) = delete;

	void push(T new_value)
	{
		std::shared_ptr<T> new_data(std::make_shared<T>(std::move(new_value)));
		std::unique_ptr<queue_node> p(std::make_unique<queue_node>());
		{
			std::lock_guard<std::mutex> lk(m_tail_mutex);
			m_pTail->m_data = new_data;
			queue_node *const new_tail = p.get();
			m_pTail->m_next = std::move(p);
			m_pTail = new_tail;
		}

		m_data_cond.notify_one();
	}

	std::shared_ptr<T> wait_and_pop()
	{
		std::unique_ptr<queue_node> const old_head = wait_pop_head();
		return old_head->m_data;
	}

	void wait_and_pop(T &value)
	{
		std::unique_ptr<queue_node> const old_head = wait_pop_head(value);
	}

	std::shared_ptr<T> try_pop()
	{
		std::unique_ptr<queue_node> old_head = try_pop_head();
		return old_head != nullptr ? old_head->m_data : nullptr;
	}

	bool try_pop(T &value)
	{
		std::unique_ptr<queue_node> old_head = try_pop_head(value);
		return old_head != nullptr;
	}

	bool empty()
	{
		std::lock_guard<std::mutex> lk(m_head_mutex);
		return (m_upHead.get() == get_tail());
	}

private:
	struct queue_node
	{
		std::shared_ptr<T> m_data;
		std::unique_ptr<queue_node> m_next;
	};

	std::mutex m_head_mutex;
	std::unique_ptr<queue_node> m_upHead;
	std::mutex m_tail_mutex;
	queue_node *m_pTail;
	std::condition_variable m_data_cond;

private:
	queue_node * get_tail()
	{
		std::lock_guard<std::mutex> lk(m_tail_mutex);
		return m_pTail;
	}

	std::unique_ptr<queue_node> pop_head()
	{
		std::unique_ptr<queue_node> old_head = std::move(m_upHead);
		m_upHead = std::move(old_head->m_next);
		return old_head;
	}

	std::unique_lock<std::mutex> wait_for_data()
	{
		std::unique_lock<std::mutex> head_lock(m_head_mutex);
		m_data_cond.wait(head_lock, [&] {return m_upHead.get() != get_tail(); });
		return std::move(head_lock);
	}

	std::unique_ptr<queue_node> wait_pop_head()
	{
		std::unique_lock<std::mutex> head_lock(wait_for_data());
		return pop_head();
	}

	std::unique_ptr<queue_node> wait_pop_head(T &value)
	{
		std::unique_lock<std::mutex> head_lock(wait_for_data());
		value = std::move(*m_upHead->m_data);
		return pop_head();
	}

	std::unique_ptr<queue_node> try_pop_head()
	{
		std::lock_guard<std::mutex> lk(m_head_mutex);
		if (m_upHead.get() == get_tail())
		{
			return nullptr;
		}

		return pop_head();
	}

	std::unique_ptr<queue_node> try_pop_head(T &value)
	{
		std::lock_guard<std::mutex> lk(m_head_mutex);
		if (m_upHead.get() == get_tail())
		{
			return nullptr;
		}

		value = std::move(*m_upHead->m_data);
		return pop_head();
	}
};

void test_concurrent_push_and_pop_on_empty_queue()
{
	threadsafe_queue<int> tq;
	std::promise<void> push_ready;
	std::promise<void> pop_ready;
	std::promise<void> go;
	std::shared_future<void> ready(go.get_future());
	std::future<void> push_done;
	std::future<int> pop_done;

	try
	{
		push_done = std::async(std::launch::async, 
		[&tq, ready, &push_ready]() 
		{
			push_ready.set_value();
			ready.wait();
			tq.push(43);
		});

		pop_done = std::async(std::launch::async,
		[&tq, ready, &pop_ready]()
		{
			int value = 0;
			pop_ready.set_value();
			ready.wait();
			tq.try_pop(value);
			return value;
		});

		push_ready.get_future().wait();
		pop_ready.get_future().wait();
		go.set_value();
		push_done.get();
		assert(pop_done.get() == 43);
		assert(tq.empty());
	}
	catch (const std::exception&)
	{
		go.set_value();
		throw;
	}

}

int main()
{
	test_concurrent_push_and_pop_on_empty_queue();

	return EXIT_SUCCESS;
}