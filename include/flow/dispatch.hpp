#pragma once

namespace flow
{
	struct async_dispatch_tag { };
	constexpr auto async_dispatch = async_dispatch_tag {};

	/**
	 * @interface dispatch_context
	 * @brief Interface for things you can dispatch an action to
	 * @tparam TAction
	 * @details This exists mainly to hide the actual state from views dispatching to a store
	 */
	template<class TAction>
	class dispatch_context
	{
	public:
		using action_type = TAction;

		virtual ~dispatch_context() = 0;
		/**
		 * @brief Dispatch an action
		 * @param[in] action: The action to be dispatched
		 */
		virtual void dispatch(TAction &&action) = 0;
		/**
		 * @brief Dispatch an action asynchronously
		 * @param[in] action: The action to be dispatched
		 */
		virtual void dispatch(TAction &&action, async_dispatch_tag) = 0;
	};

	template<class TAction>
	dispatch_context<TAction>::~dispatch_context() {}
}