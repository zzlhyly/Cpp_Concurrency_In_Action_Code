#include <exception>
#include <stack>
#include <mutex>
#include <memory>

struct empty_stack : public std::exception
{
	const char* what() const throw();
};

template <typename T>
class threadsafe_stack
{
public:
	threadsafe_stack() = default;
	threadsafe_stack(const threadsafe_stack &other)
	{
		std::lock_guard<std::mutex> lk(other.m_mx);
		m_data = other.m_data;
	}
	threadsafe_stack &operator=(const threadsafe_stack&) = delete;

	void push(T new_value)
	{
		std::lock_guard<std::mutex> lk(m_mx);
		m_data.push(std::move(new_value));
	}

	std::shared_ptr<T> pop()
	{
		std::lock_guard<std::mutex> lk(m_mx);
		if (m_data.empty())
		{
			throw empty_stack();
		}

		const std::shared_ptr<T> res(std::make_shared<T>(std::move(m_data.top())));
		m_data.pop();
		return res;
	}

	void pop(T &value)
	{
		std::lock_guard<std::mutex> lk(m_mx);
		if (m_data.empty())
		{
			throw empty_stack();
		}
		value = std::move(m_data.top());
		m_data.pop();
	}

	bool empty() const
	{
		std::lock_guard<std::mutex> lk(m_mx);
		return m_data.empty();
	}

private:
	std::stack<T> m_data;
	mutable std::mutex m_mx;
};

int main()
{
	return EXIT_SUCCESS;
}