#include <iostream>
#include <format>
#include <flow/store.hpp>

struct counter
{
	int value = 0;

	auto operator!=(const counter &rhs) const noexcept -> bool
	{
		return value != rhs.value;
	}
};

enum counter_action
{
	increment,
	decrement,
	reset
};

void print_counter(const counter &counter)
{
	std::cout << std::format("counter {{value: {}}}\n", counter.value);
}

namespace flow
{
	template<>
	struct reducer<counter, counter_action>
	{
		auto operator()(counter state, counter_action action) const -> counter
		{
			switch(action)
			{
			case counter_action::increment:
				state.value++;
				std::cout << "counter_action::increment\n";
				break;
			case counter_action::decrement:
				state.value--;
				std::cout << "counter_action::decrement\n";
				break;
			case counter_action::reset:
				state.value = 0;
				std::cout << "counter_action::reset\n";
				break;
			default:
				break;
			}

			print_counter(state);
			std::cout << std::endl;
			return state;
		}
	};
}

int main()
{
	auto store = flow::make_store<counter, counter_action>();

	for(auto i = 0; i < 10; ++i)
	{
		store->dispatch(counter_action::increment);
	}

	for(auto i = 0; i < 10; ++i)
	{
		store->dispatch(counter_action::decrement);
	}

	store->dispatch(counter_action::reset);
	return 0;
}