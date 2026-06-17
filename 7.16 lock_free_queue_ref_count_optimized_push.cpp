#define _ENABLE_ATOMIC_ALIGNMENT_FIX
#include <memory>
#include <atomic>

template <typename T>
class lock_free_queue
{
public:
	lock_free_queue()
	{
		queue_counted_node_ptr new_node;
		new_node.m_external_count = 1;
		new_node.m_ptr = new queue_node();

		m_head.store(new_node);
		m_tail.store(new_node);
	}

	void push(T new_value)
	{
		std::unique_ptr<T> new_data(std::make_unique<T>(std::move(new_value)));

		queue_counted_node_ptr new_next;
		new_next.m_ptr = new queue_node();
		new_next.m_external_count = 1;

		queue_counted_node_ptr old_tail = m_tail.load();

		while (true)
		{
			increase_external_count(m_tail, old_tail);

			T *old_data = nullptr;
			if (old_tail.m_ptr->m_data.compare_exchange_strong(old_data, new_data.get()))
			{
				queue_counted_node_ptr old_next;
				if (!old_tail.m_ptr->m_next.compare_exchange_strong(old_next, new_next))
				{
					delete new_next.m_ptr;
					new_next = old_next;
				}
				set_new_tail(old_tail, new_next);
				new_data.release();
				break;
			}
			else
			{
				queue_counted_node_ptr old_next;
				if (old_tail.m_ptr->m_next.compare_exchange_strong(old_next, new_next))
				{
					old_next = new_next;
					new_next.m_ptr = new queue_node();
				}

				set_new_tail(old_tail, old_next);
			}
		}
	}

	std::unique_ptr<T> pop()
	{
		queue_counted_node_ptr old_head = m_head.load(std::memory_order_relaxed);

		while (true)
		{
			increase_external_count(m_head, old_head);
			queue_node *const ptr = old_head.m_ptr;
			if (ptr == m_tail.load().m_ptr)
			{
				return nullptr;
			}

			queue_counted_node_ptr next = ptr->m_next.load();
			if (m_head.compare_exchange_strong(old_head, next))
			{
				T *const res = ptr->m_data.exchange(nullptr);
				free_external_counter(old_head);
				return std::unique_ptr<T>(res);
			}

			ptr->release_ref();
		}
	}

private:
	struct queue_node;

	struct queue_counted_node_ptr
	{
		int m_external_count = 0;
		queue_node *m_ptr = nullptr;
	};

	std::atomic<queue_counted_node_ptr> m_head;
	std::atomic<queue_counted_node_ptr> m_tail;

	struct queue_node_counter
	{
		unsigned m_internal_count : 30;
		unsigned m_external_counters : 2;
	};

	struct queue_node
	{
		queue_node()
		{
			queue_node_counter new_count;
			new_count.m_internal_count = 0;
			new_count.m_external_counters = 2;
			m_count.store(new_count);
			m_data = nullptr;
		}

		void release_ref()
		{
			queue_node_counter old_counter = m_count.load(std::memory_order_relaxed);
			queue_node_counter new_counter;

			do
			{
				new_counter = old_counter;
				--new_counter.m_internal_count;
			} while (!m_count.compare_exchange_strong(old_counter, new_counter, std::memory_order_acquire, std::memory_order_relaxed));

			if (new_counter.m_internal_count == 0
				&& new_counter.m_external_counters == 0)
			{
				delete this;
			}
		}

		std::atomic<T*> m_data;
		std::atomic<queue_node_counter> m_count;
		std::atomic<queue_counted_node_ptr> m_next;
	};

	static void	increase_external_count(std::atomic<queue_counted_node_ptr> &conuter, queue_counted_node_ptr &old_counter)
	{
		queue_counted_node_ptr new_counter;
		do
		{
			new_counter = old_counter;
			++new_counter.m_external_count;
		} while (!conuter.compare_exchange_strong(old_counter, new_counter, std::memory_order_acquire, std::memory_order_relaxed));

		old_counter.m_external_count = new_counter.m_external_count;
	}

	static void free_external_counter(queue_counted_node_ptr &old_node_ptr)
	{
		queue_node *const ptr = old_node_ptr.m_ptr;
		int const count_increase = old_node_ptr.m_external_count - 2;

		queue_node_counter old_counter = ptr->m_count.load(std::memory_order_relaxed);
		queue_node_counter new_counter;

		do
		{
			new_counter = old_counter;
			--new_counter.m_external_counters;
			new_counter.m_internal_count += count_increase;
		} while (!ptr->m_count.compare_exchange_strong(old_counter, new_counter, std::memory_order_acquire, std::memory_order_relaxed));

		if (new_counter.m_internal_count == 0
			&& new_counter.m_external_counters == 0)
		{
			delete ptr;
		}
	}

	void set_new_tail(queue_counted_node_ptr &old_tail, queue_counted_node_ptr &new_tail)
	{
		queue_node *const current_tail_ptr = old_tail.m_ptr;
		while (!m_tail.compare_exchange_weak(old_tail, new_tail)
			&& old_tail.m_ptr == current_tail_ptr)
		{
			if (old_tail.m_ptr == current_tail_ptr)
			{
				free_external_counter(old_tail);
			}
			else
			{
				current_tail_ptr->release_ref();
			}
		}
	}
};

int main()
{
	lock_free_queue<int> lfq;
	lfq.push(213213);
	lfq.pop();

	return EXIT_SUCCESS;
}