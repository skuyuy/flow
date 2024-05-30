#pragma once
#include <functional>
#include <memory>
#include <unordered_set>

#include "subscriber.hpp"

namespace flow
{
	template<class TStore, class TValue>
	class lens : public subscriber<typename TStore::state_type>
	{
	public:
		using base_type = subscriber<typename TStore::state_type>;
		using store_type = TStore;
		using value_type = TValue;
		using observed_input_type = typename base_type::observed_input_type;
		using state_type = typename store_type::state_type;
		using transform_fn = std::function<value_type(observed_input_type)>;
		using unsubscribe_fn = typename store_type::unsubscribe_fn;

		lens(const std::shared_ptr<store_type> store, transform_fn transform);
		~lens();

		void handle_change(observed_input_type value) override;

		auto operator*() noexcept -> const value_type&;
		auto operator->() noexcept -> const value_type&;
	protected:
		value_type value_;
		transform_fn transform_;
	private:
		std::shared_ptr<store_type> store_ref_; // safety
		unsubscribe_fn unsubscribe_;
	};

	template<class TStore, class TValue>
	class relay_lens: public lens<TStore, TValue>
	{
	public:
		using base_type = lens<TStore, TValue>;
		using store_type = TStore;
		using value_type = TValue;
		using observed_input_type = typename base_type::observed_input_type;
		using state_type = typename store_type::state_type;
		using transform_fn = std::function<value_type(observed_input_type)>;
		using unsubscribe_fn = typename store_type::unsubscribe_fn;
		using self_unsubscribe_fn = std::function<void(void)>;
		using subscriber_type = subscriber<value_type>;

		using base_type::lens;

		void handle_change(observed_input_type value) override;

		auto subscribe(subscriber_type *subscriber) -> self_unsubscribe_fn;
		void unsubscribe(subscriber_type *subscriber);
	private:
		using subscribers = std::unordered_set<subscriber_type *>;

		void notify_all_();

		subscribers subscribers_;
	};

	template<class TStore, class TValue>
	inline lens<TStore, TValue>::lens(const std::shared_ptr<store_type> store, transform_fn transform):
		store_ref_{store},
		transform_{transform},
		value_{transform(store->state())}, // call the transformation initially once
		unsubscribe_{store->subscribe(this)}
	{
	}

	template<class TStore, class TValue>
	inline lens<TStore, TValue>::~lens()
	{
		if(unsubscribe_)
		{
			unsubscribe_();
		}
	}

	template<class TStore, class TValue>
	inline void lens<TStore, TValue>::handle_change(observed_input_type value)
	{
		if(!transform_)
		{
			return;
		}

		auto result = transform_(value);
		if(value_ != result)
		{
			value_ = result;
		}
	}

	template<class TStore, class TValue>
	inline auto lens<TStore, TValue>::operator*() noexcept -> const value_type &
	{
		return value_;
	}

	template<class TStore, class TValue>
	inline auto lens<TStore, TValue>::operator->() noexcept -> const value_type &
	{
		return value_;
	}

	template<class TStore, class TValue>
	inline void relay_lens<TStore, TValue>::handle_change(observed_input_type value)
	{
		if(!transform_)
		{
			return;
		}

		auto result = transform_(value);
		if(value_ != result)
		{
			value_ = result;
			notify_all_();
		}
	}
	template<class TStore, class TValue>
	inline auto relay_lens<TStore, TValue>::subscribe(subscriber_type *subscriber) -> self_unsubscribe_fn
	{
		constexpr auto do_nothing = []() { };

		if(!subscriber || subscribers_.contains(subscriber)) // prevent null and double insertion
		{
			return do_nothing;
		}

		subscribers_.insert(subscriber);
		return [this, subscriber]() { unsubscribe(subscriber); };
	}

	template<class TStore, class TValue>
	inline void relay_lens<TStore, TValue>::unsubscribe(subscriber_type *subscriber)
	{ 
		// the nullness of subscriber is actually irrelevant here since subscribe() prevents the insertion of null subscribers
		if(!subscribers_.contains(subscriber))
		{
			return;
		}

		subscribers_.erase(subscriber);
	}

	template<class TStore, class TValue>
	inline void relay_lens<TStore, TValue>::notify_all_()
	{
		// strengthened by subscribe(): impossible to have subscribers
		for(auto *subscriber : subscribers_)
		{
			subscriber->handle_change(state_);
		}
	}
}