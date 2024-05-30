#pragma once
#include <type_traits>

namespace flow
{
	/**
	 * @interface subscriber 
	 * @brief Interface for a class subscribing to changes of an observed type
	 * @tparam TObserved: The observed type
	 */
	template<class TObserved>
	class subscriber
	{
	public:
		// optimized input parameter type: pass by ref if the type is bigger than a pointer (therefore a reference) of itself
		using observed_input_type = std::conditional_t<
			sizeof(TObserved) <= sizeof(TObserved*),
			TObserved,
			const TObserved&
		>;
		using observed_type = TObserved;

		virtual ~subscriber() = 0;

		virtual void handle_change(observed_input_type value) = 0;
	};

	template<class TObserved>
	subscriber<TObserved>::~subscriber() {}
}