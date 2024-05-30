#include <flow/store.hpp>
#include <flow/subscriber.hpp>
#include <cassert>
#include <chrono>

namespace flow::tests
{
	enum class int_action
	{
		increment,
		decrement,
		increment_async
	};

	struct int_reducer
	{
		auto operator()(int value, int_action action) const -> int
		{
			using namespace std::chrono_literals;

			switch(action)
			{
			case int_action::increment:
				return ++value;
			case int_action::increment_async:
				std::this_thread::sleep_for(3s);
				return ++value;
			case int_action::decrement:
				return --value;
			default:
				return value;
			}
		}
	};

	struct async_store_tester final: public subscriber<int>
	{
		int old_value{0};
		int_action dispatched_action;
		std::thread::id main_thread_id{std::this_thread::get_id()};

		void handle_change(int value)
		{
			if(dispatched_action == int_action::increment_async)
			{
				assert(std::this_thread::get_id() != main_thread_id); // calling thread needs to be different than main
				assert(value == old_value + 1);
			}
		}
	};

	class store_test
	{
	public:
		store_test()
		{
			store_->subscribe(&async_tester_);
		}

		void test_increment() 
		{ 
			auto old_value = store_->state();

			store_->dispatch(int_action::increment);
			
			// use catch / gtest here??
			assert(store_->state() == old_value + 1);
		}

		void test_decrement()
		{
			auto old_value = store_->state();

			store_->dispatch(int_action::decrement);

			// use catch / gtest here??
			assert(store_->state() == old_value - 1);
		}

		void test_async()
		{
			async_tester_.dispatched_action = int_action::increment_async;
			async_tester_.old_value = store_->state();

			store_->dispatch(int_action::increment_async, async_dispatch);
		}
	private:
		async_store_tester async_tester_;
		std::shared_ptr<store<int, int_action, int_reducer>> store_{make_store<int, int_action, int_reducer>(0)};
	};
}

int main()
{
	flow::tests::store_test t;

	t.test_increment();
	t.test_decrement();
	t.test_async();

	return 0;
}