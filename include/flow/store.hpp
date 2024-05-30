#pragma once
#include <mutex>
#include <thread>
#include <queue>
#include <unordered_set>
#include <functional>
#include <memory>

#include "dispatch.hpp"
#include "subscriber.hpp"

namespace flow
{
	template<class TState, class TAction>
	struct reducer {};

	template<class T, class TState, class TAction>
	concept reducer_concept = (std::invocable<T, TState, TAction> && std::convertible_to<std::invoke_result_t<T, TState, TAction>, TState> && std::default_initializable<T>);

	/**
	 * @brief A state manager *owning* a state and mutating it through actions
	 * @tparam TState: The type of state the store holds
	 * @tparam TAction: The type of action the store dispatches 
	 * @tparam TReducer: The reducer function to use
	 */
	template<class TState, class TAction, reducer_concept<TState, TAction> TReducer>
	class store: public dispatch_context<TAction>
	{
	public:
		using base_type = dispatch_context<TAction>;
		using action_type = base_type::action_type;
		using state_type = TState;
		using reducer_type = TReducer;
		using subscriber_type = subscriber<state_type>;
		using unsubscribe_fn = std::function<void(void)>;

		explicit store(state_type &&initial_state);
		~store();

		void dispatch(action_type&& action) override;
		void dispatch(action_type&& action, async_dispatch_tag) override;
		auto subscribe(subscriber_type* subscriber) -> unsubscribe_fn;
		void unsubscribe(subscriber_type* subscriber);

		auto state() noexcept -> const state_type&;
		void async_dispatch_worker(std::stop_token stop_token);
	private:
		using action_queue = std::queue<action_type>;
		using subscribers = std::unordered_set<subscriber_type*>;

		template<class UState, class UAction, class UReducer, class... TArgs>
		friend auto make_store(TArgs&&... args) -> std::shared_ptr<store<UState, UAction, UReducer>>;

		void notify_all_();

		// members
		action_queue async_dispatch_q_;
		state_type state_;
		subscribers subscribers_;
		reducer_type reducer_{};

		// async dispatch
		std::mutex state_mtx_;
		std::mutex async_dispatch_q_mtx_;
		std::mutex subscribers_mtx_;
		std::condition_variable async_dispatch_cv_;
		std::jthread async_dispatch_thread_{};
	};

	template<class TState, class TAction, reducer_concept<TState, TAction> TReducer>
	inline store<TState, TAction, TReducer>::~store()
	{ 
		async_dispatch_thread_.request_stop();
	}

	template<class TState, class TAction, reducer_concept<TState, TAction> TReducer>
	inline void store<TState, TAction, TReducer>::dispatch(action_type &&action)
	{
		// lock the state
		auto state_lock = std::scoped_lock{ state_mtx_ };
		// v1: define a reducer function in the same namespace as the store
		auto updated_state = reducer_(state_, action);
		if(state_ != updated_state)
		{
			state_ = updated_state;
			notify_all_();
		}
	}

	template<class TState, class TAction, reducer_concept<TState, TAction> TReducer>
	inline void store<TState, TAction, TReducer>::dispatch(action_type && action, async_dispatch_tag)
	{ 
		if(!async_dispatch_thread_.joinable()) // a thread is only joinable when it is an executing thread
		{
			async_dispatch_thread_ = std::jthread{[this](std::stop_token stop_token) { async_dispatch_worker(stop_token); }};
		}

		auto lock = std::scoped_lock { async_dispatch_q_mtx_ };
		async_dispatch_q_.push(action);
		async_dispatch_cv_.notify_all();
	}

	template<class TState, class TAction, reducer_concept<TState, TAction> TReducer>
	inline auto store<TState, TAction, TReducer>::subscribe(subscriber_type *subscriber) -> unsubscribe_fn
	{
		auto lock = std::scoped_lock { subscribers_mtx_ };
		constexpr auto do_nothing = [](){};

		if(!subscriber || subscribers_.contains(subscriber)) // prevent null and double insertion
		{
			return do_nothing;
		}

		subscribers_.insert(subscriber);
		return [this, subscriber]() { unsubscribe(subscriber); };
	}

	template<class TState, class TAction, reducer_concept<TState, TAction> TReducer>
	inline store<TState, TAction, TReducer>::store(state_type &&initial_state):
		state_{std::move(initial_state)}
	{
	}

	template<class TState, class TAction, reducer_concept<TState, TAction> TReducer>
	inline void store<TState, TAction, TReducer>::async_dispatch_worker(std::stop_token stop_token)
	{
		auto q_lock = std::unique_lock<std::mutex> { async_dispatch_q_mtx_ };
		async_dispatch_cv_.wait(q_lock, [this]() { return !async_dispatch_q_.empty(); });

		// woken up by cv, we know the queue is not empty
		do
		{
			// just invoke the non-async dispatch next
			dispatch(std::move(async_dispatch_q_.back()));
			// pop the action from the queue now that it has been dispatched
			async_dispatch_q_.pop();
		}
		while(!async_dispatch_q_.empty());

		q_lock.unlock();
	}

	template<class TState, class TAction, reducer_concept<TState, TAction> TReducer>
	inline void store<TState, TAction, TReducer>::notify_all_()
	{ 
		auto lock = std::scoped_lock{subscribers_mtx_};

		// strengthened by subscribe(): impossible to have subscribers
		for(auto *subscriber : subscribers_)
		{
			subscriber->handle_change(state_);
		}
	}

	template<class TState, class TAction, reducer_concept<TState, TAction> TReducer>
	inline void store<TState, TAction, TReducer>::unsubscribe(subscriber_type *subscriber)
	{
		auto lock = std::scoped_lock { subscribers_mtx_ };
		// the nullness of subscriber is actually irrelevant here since subscribe() prevents the insertion of null subscribers
		if(!subscribers_.contains(subscriber))
		{
			return;
		}

		subscribers_.erase(subscriber);
	}

	template<class TState, class TAction, reducer_concept<TState, TAction> TReducer>
	inline auto store<TState, TAction, TReducer>::state() noexcept -> const state_type &
	{
		return state_;
	}

	template<class UState, class UAction, class UReducer = reducer<UAction, UState>, class ...TArgs>
		requires std::constructible_from<UState, TArgs...>
	auto make_store(TArgs && ...args) -> std::shared_ptr<store<UState, UAction, UReducer>>
	{
		return std::make_shared<store<UState, UAction, UReducer>>(UState{std::forward<TArgs>(args)...});
	}
}