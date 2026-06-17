#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>
#include <string>
#include <iostream>
#include <thread>

namespace messaging
{
	struct message_base
	{
	public:
		virtual ~message_base() = default;
	};

	template <typename Msg>
	struct wrapped_message : public message_base
	{
	public:
		explicit wrapped_message(const Msg& contents)
			: m_contents(contents)
		{

		}

		Msg m_contents;
	};

	class message_queue
	{
	public:
		template <typename T>
		void push(const T &msg)
		{
			std::lock_guard<std::mutex> lk(m_mx);
			m_queue.push(std::make_shared<wrapped_message<T>>(msg));
			m_cond.notify_all();
		}

		std::shared_ptr<message_base> wait_and_pop()
		{
			std::unique_lock<std::mutex> uk(m_mx);
			m_cond.wait(uk, [&] { return !m_queue.empty(); });
			auto res = m_queue.front();
			m_queue.pop();
			return res;
		}

	private:
		std::mutex m_mx;
		std::condition_variable m_cond;
		std::queue<std::shared_ptr<message_base>> m_queue;
	};

	class sender
	{
	public:
		sender()
			: m_p_mq(nullptr)
		{

		}

		explicit sender(message_queue *p_mq)
			: m_p_mq(p_mq)
		{

		}

		template <typename Message>
		void send_message(const Message &msg)
		{
			if (m_p_mq != nullptr)
			{
				m_p_mq->push(msg);
			}
		}

	private:
		message_queue * m_p_mq;;
	};

	class close_queue
	{

	};

	template <typename PreviousDispatcher, typename Msg, typename Func>
	class template_dispatcher
	{
	public:
		template_dispatcher(message_queue *p_mq, PreviousDispatcher *p_prev_dp, Func && f)
			: m_p_mq(p_mq), m_p_prev_dp(p_prev_dp), m_f(std::forward<Func>(f)), m_chained(false)
		{
			p_prev_dp->m_chained = true;
		}

		template_dispatcher(template_dispatcher &&other)
			: m_p_mq(other.m_p_mq), m_p_prev_dp(other.m_p_prev_dp), m_f(std::move(other.m_f)), m_chained(other.m_chained)
		{
			other.m_chained = true;
		}
		template_dispatcher(const template_dispatcher &) = delete;
		template_dispatcher &operator=(const template_dispatcher&) = delete;
		~template_dispatcher()noexcept(false)
		{
			if (!m_chained)
			{
				wait_and_dispatch();
			}
		}

		template <typename OtherMsg, typename OtherFunc>
		template_dispatcher<template_dispatcher, OtherMsg, OtherFunc> handle(OtherFunc && of)
		{
			return template_dispatcher<template_dispatcher, OtherMsg, OtherFunc>(m_p_mq, this, std::forward<OtherFunc>(of));
		}

	protected:
	private:
		message_queue * m_p_mq;
		PreviousDispatcher *m_p_prev_dp;
		Func m_f;
		bool m_chained;

		template <typename Dispatcher, typename OtherMsg, typename OtherFunc>
		friend class template_dispatcher;

		void wait_and_dispatch()
		{
			while (true)
			{
				auto msg = m_p_mq->wait_and_pop();
				if (dispatch(msg))
				{
					break;
				}
			}
		}

		bool dispatch(const std::shared_ptr<message_base> &msg)
		{
			wrapped_message<Msg> *wrapper = nullptr;
			if ((wrapper = dynamic_cast<wrapped_message<Msg>*>(msg.get())) != nullptr)
			{
				m_f(wrapper->m_contents);
				return true;
			}
			else
			{
				return m_p_prev_dp->dispatch(msg);
			}
		}
	};

	class dispatcher
	{
	public:
		dispatcher(dispatcher && other)
			: m_p_mq(other.m_p_mq), m_chained(other.m_chained)
		{
			other.m_chained = true;
		}

		explicit dispatcher(message_queue *p_mq)
			: m_p_mq(p_mq), m_chained(false)
		{

		}

		dispatcher(const dispatcher&) = delete;
		dispatcher &operator=(const dispatcher&) = delete;
		~dispatcher() noexcept(false)
		{
			if (!m_chained)
			{
				wait_and_dispatch();
			}
		}

		template <typename Message, typename Func>
		template_dispatcher<dispatcher, Message, Func> handle(Func &&f)
		{
			return template_dispatcher<dispatcher, Message, Func>(m_p_mq, this, std::forward<Func>(f));
		}

		
	private:
		message_queue * m_p_mq;
		bool m_chained;

		template <typename Dispatcher, typename Msg, typename Func>
		friend class template_dispatcher;

		void wait_and_dispatch()
		{
			while (true)
			{
				auto msg = m_p_mq->wait_and_pop();
				dispatch(msg);
			}
		}

		bool dispatch(const std::shared_ptr<message_base> & msg)
		{
			if (dynamic_cast<wrapped_message<close_queue>*>(msg.get()) != nullptr)
			{
				throw close_queue();
			}

			return false;
		}
	};

	class receiver
	{
	public:
		operator sender()
		{
			return sender(&m_mq);
		}

		dispatcher wait()
		{
			return dispatcher(&m_mq);
		}

	private:
		message_queue m_mq;
	};
}


struct withdraw
{
public:
	withdraw(const std::string &account, unsigned amount, messaging::sender atm_queue)
		: m_account(account), m_amount(amount), m_atm_queue(atm_queue)
	{

	}

	std::string m_account;
	unsigned m_amount;
	mutable messaging::sender m_atm_queue;
};

struct withdraw_ok
{

};

struct withdraw_denied
{

};

struct cancel_withdrawal
{
public:
	cancel_withdrawal(const std::string &account, unsigned amount)
		: m_account(account), m_amount(amount)
	{

	}

	std::string m_account;
	unsigned m_amount;
};

struct withdrawal_processed
{
public:
	withdrawal_processed(const std::string &account, unsigned amount)
		: m_account(account), m_amount(amount)
	{

	}

	std::string m_account;
	unsigned m_amount;
};

struct card_inserted
{
public:
	explicit card_inserted(const std::string &account)
		: m_account(account)
	{

	}

	std::string m_account;
};

struct digit_pressed
{
public:
	explicit digit_pressed(char digit)
		: m_digit(digit)
	{

	}

	char m_digit;
};

struct clear_last_pressed
{

};

struct eject_card
{

};

struct withdraw_pressed
{
public:
	explicit withdraw_pressed(unsigned amount)
		: m_amount(amount)
	{

	}

	unsigned m_amount;
};

struct cancel_pressed
{

};

struct issue_money
{
public:
	explicit issue_money(unsigned amount)
		: m_amount(amount)
	{

	}

	unsigned m_amount;
};

struct verify_pin
{
public:
	verify_pin(const std::string &account, const std::string &pin, messaging::sender atm_queue)
		: m_account(account), m_pin(pin), m_atm_queue(atm_queue)
	{

	}

	std::string m_account;
	std::string m_pin;
	mutable messaging::sender m_atm_queue;
};

struct pin_verified
{

};

struct pin_incorrect
{

};

struct display_enter_pin
{

};

struct display_enter_card
{

};

struct display_insufficient_funds
{

};

struct display_withdrawal_cancelled
{

};

struct display_pin_incorrect_message
{

};

struct display_withdrawal_options
{

};

struct get_balance
{
public:
	get_balance(const std::string &account, messaging::sender atm_queue)
		: m_account(account), m_atm_queue(atm_queue)
	{

	}

	std::string m_account;
	mutable messaging::sender m_atm_queue;
};

struct balance
{
public:
	explicit balance(unsigned amount)
		: m_amount(amount)
	{

	}

	unsigned m_amount;
};

struct display_balance
{
public:
	explicit display_balance(unsigned amount)
		: m_amount(amount)
	{

	}

	unsigned m_amount;
};

struct balance_pressed
{

};

class atm
{
public:
	atm(messaging::sender bank, messaging::sender interface_hardware)
		: m_bank(bank), m_interface_hardware(interface_hardware)
	{

	}

	atm(const atm&) = delete;
	atm &operator=(const atm&) = delete;

	void done()
	{
		get_sender().send_message(messaging::close_queue());
	}

	void run()
	{
		m_state = &atm::waiting_for_card;
		try
		{
			while (true)
			{
				(this->*m_state)();
			}
		}
		catch (...)
		{

		}
	}

	messaging::sender get_sender()
	{
		return m_incoming;
	}

private:
	messaging::receiver m_incoming;
	messaging::sender m_bank;
	messaging::sender m_interface_hardware;
	void (atm::*m_state)();
	std::string m_account;
	unsigned m_withdrawal_amount;
	std::string m_pin;

	void done_processing()
	{
		m_interface_hardware.send_message(eject_card());
		m_state = &atm::waiting_for_card;
	}

	void process_withdrawal()
	{
		m_incoming.wait()
			.handle<withdraw_ok>
		([&](const withdraw_ok& msg)
		{
			m_interface_hardware.send_message(issue_money(m_withdrawal_amount));
			m_bank.send_message(withdrawal_processed(m_account, m_withdrawal_amount));
			m_state = &atm::done_processing;
		})
			.handle<withdraw_denied>
		([&](const withdraw_denied &msg)
		{
			m_interface_hardware.send_message(display_insufficient_funds());
			m_state = &atm::done_processing;
		})
			.handle<cancel_pressed>
		([&](const cancel_pressed &msg)
		{
			m_bank.send_message(cancel_withdrawal(m_account, m_withdrawal_amount));
			m_interface_hardware.send_message(display_withdrawal_cancelled());
			m_state = &atm::done_processing;
		});
	}

	void process_balance()
	{
		m_incoming.wait()
			.handle<balance>
		([&](const balance &msg)
		{
			m_interface_hardware.send_message(display_balance(msg.m_amount));
			m_state = &atm::wait_for_action;
		})
			.handle<cancel_pressed>
		([&](const cancel_pressed &msg)
		{
			m_state = &atm::done_processing;
		});
	}

	void wait_for_action()
	{
		m_interface_hardware.send_message(display_withdrawal_options());

		m_incoming.wait()
			.handle<withdraw_pressed>
		([&](const withdraw_pressed &msg) 
		{
			m_withdrawal_amount = msg.m_amount;
			m_bank.send_message(withdraw(m_account, msg.m_amount, m_incoming));
			m_state = &atm::process_withdrawal;
		})
			.handle<balance_pressed>
		([&](const balance_pressed &msg) 
		{
			m_bank.send_message(get_balance(m_account, m_incoming));
			m_state = &atm::process_balance;
		})
			.handle<cancel_pressed>
		([&](const cancel_pressed &msg)
		{
			m_state = &atm::done_processing; 
		});
	}

	void verifying_pin()
	{
		m_incoming.wait()
			.handle<pin_verified>
		([&](const pin_verified & msg) 
		{ 
			m_state = &atm::wait_for_action; 
		})
			.handle<pin_incorrect>
		([&](const pin_incorrect &msg)
		{
			m_interface_hardware.send_message(display_pin_incorrect_message());
			m_state = &atm::done_processing;
		})
			.handle<cancel_pressed>
		([&](const cancel_pressed & msg) 
		{ 
			m_state = &atm::done_processing; 
		});
	}

	void getting_pin()
	{
		m_incoming.wait()
			.handle<digit_pressed>
		([&](const digit_pressed & msg)
		{
			const unsigned pin_length = 4;
			m_pin += msg.m_digit;
			if (m_pin.size() == pin_length)
			{
				m_bank.send_message(verify_pin(m_account, m_pin, m_incoming));
				m_state = &atm::verifying_pin;
			}
		})
			.handle<clear_last_pressed>
		([&](const clear_last_pressed &msg)
		{
			if (!m_pin.empty())
			{
				m_pin.pop_back();
			}
		})
			.handle<cancel_pressed>
		([&](const cancel_pressed & msg)
		{
			m_state = &atm::done_processing;
		});
	}

	void waiting_for_card()
	{
		m_interface_hardware.send_message(display_enter_card());

		m_incoming.wait()
			.handle<card_inserted>
		([&](const card_inserted & msg) 
		{
			m_account = msg.m_account;
			m_pin = "";
			m_interface_hardware.send_message(display_enter_pin());
			m_state = &atm::getting_pin;
		});
	}
};

class bank_machine
{
public:
	bank_machine()
		: m_balance(199)
	{

	}

	void done()
	{
		get_sender().send_message(messaging::close_queue());
	}

	void run()
	{
		try
		{
			while (true)
			{
				m_incomig.wait()
					.handle<verify_pin>
				([&](const verify_pin &msg)
				{
					if (msg.m_pin == "4326")
					{
						msg.m_atm_queue.send_message(pin_verified());
					}
					else
					{
						msg.m_atm_queue.send_message(pin_incorrect());
					}
				})
					.handle<withdrawal_processed>([&](const withdrawal_processed &msg) {})
					.handle<cancel_pressed>([&](const cancel_pressed & msg) {});
			}
		}
		catch (...)
		{

		}
	}

	messaging::sender get_sender()
	{
		return m_incomig;
	}

private:
	messaging::receiver m_incomig;
	unsigned m_balance;
};

class interface_machine
{
public:
	void done()
	{
		get_sender().send_message(messaging::close_queue());
	}

	void run()
	{
		try
		{
			while (true)
			{
				m_incoming.wait()
					.handle<issue_money>
				([&](const issue_money &msg) 
				{
					{
						std::lock_guard<std::mutex> lk(m_iom);
						std::cout << "Issuing " << msg.m_amount << std::endl;
					}
				})
					.handle<display_insufficient_funds>
				([&](const display_insufficient_funds &msg)
				{
					{
						std::lock_guard<std::mutex> lk(m_iom);
						std::cout << "Insufficient" << std::endl;
					}
				})
					.handle<display_enter_pin>
				([&](const display_enter_pin &msg)
				{
					{
						std::lock_guard<std::mutex> lk(m_iom);
						std::cout << "Please enter your PIN(0-9)" << std::endl;
					}
				})
					.handle<display_enter_card>
				([&](const display_enter_card &msg) 
				{
					{
						std::lock_guard<std::mutex> lk(m_iom);
						std::cout << "Please enter your card(I)" << std::endl;
					}
				})
					.handle<display_balance>
				([&](const display_balance &msg)
				{
					{
						std::lock_guard<std::mutex> lk(m_iom);
						std::cout << "The balance of your account is" << msg.m_amount << std::endl;
					}
				})
					.handle<display_withdrawal_options>
				([&](const display_withdrawal_options &msg)
				{
					{
						std::lock_guard<std::mutex> lk(m_iom);
						std::cout << "Withdraw 50?(w)" << std::endl;
						std::cout << "Display Balance?(b)" << std::endl;
						std::cout << "Cancel?(c)" << std::endl;
					}
				})
					.handle<display_withdrawal_cancelled>
					([&](const display_withdrawal_cancelled &msg)
				{
					{
						std::lock_guard<std::mutex> lk(m_iom);
						std::cout << "Withdrawal cancelled" << std::endl;
					}
				})
					.handle<display_pin_incorrect_message>
				([&](const display_pin_incorrect_message &msg)
				{
					{
						std::lock_guard<std::mutex> lk(m_iom);
						std::cout << "PIN incorrect" << std::endl;
					}
				})
					.handle<eject_card>
				([&](const eject_card &msg)
				{
					{
						std::lock_guard<std::mutex> lk(m_iom);
						std::cout << "Eject card" << std::endl;
					}
				});
			}
		}
		catch (...)
		{

		}
	}

	messaging::sender get_sender()
	{
		return m_incoming;
	}
protected:
private:
	messaging::receiver m_incoming;
	std::mutex m_iom;
};

int main()
{
	bank_machine bm;
	interface_machine im;
	atm machine(bm.get_sender(), im.get_sender());
	
	std::thread bank_thread(&bank_machine::run, &bm);
	std::thread im_thread(&interface_machine::run, &im);
	std::thread atm_thread(&atm::run, &machine);
	
	messaging::sender atm_queue(machine.get_sender());
	bool quit_pressed = false;

	while (!quit_pressed)
	{
		char c = getchar();
		switch (c)
		{
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			atm_queue.send_message(digit_pressed(c));
			break;
		case 'b':
			atm_queue.send_message(balance_pressed());
			break;
		case 'w':
			atm_queue.send_message(withdraw_pressed(50));	
			break;
		case 'c':
			atm_queue.send_message(cancel_pressed());
			break;
		case 'q':
			quit_pressed = true;
			break;
		case 'i':
			atm_queue.send_message(card_inserted("zzl"));
		default:
			break;
		}
	}

	bm.done();
	machine.done();
	im.done();

	atm_thread.join();
	bank_thread.join();
	im_thread.join();

	return 0;
}