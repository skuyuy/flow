#pragma once
#include <concepts>

namespace flow
{
    template<class T, class TValue>
    concept subscriber_concept = requires(T t, const TValue& v)
    {
        { t.on_change(v) } -> std::same_as<void>;
    };

    namespace detail
    {
        template<class TValue>
        class subscriber_interface
        {
        public:
            virtual void on_change(const TValue& value) const = 0;
        };

        template<class T, class TValue>
            requires subscriber_concept<T, TValue>
        class subscriber_proxy: public subscriber_interface<TValue>
        {
        public:
            using type = T;
            using value_type = TValue;

            subscriber_proxy(T impl):
                impl_{impl}
            {}

            void on_change(const value_type &value) const override
            {
                if(!impl_)
                {
                    return;
                }

                impl_.on_change(value);
            }
        private:
            type impl_;
        };
    }

    template<class TValue>
    class subscriber
    {
    public:
        using value_type = TValue;

        ~subscriber()
        {
            if(iface_)
            {
                delete iface_;
                iface_ = nullptr;
            }
        }

        void on_change(const value_type &value) const
        {
            iface_->on_change(value);
        }
    private:
        template<subscriber_concept<TValue> T>
        subscriber(T t):
            iface_{new detail::subscriber_proxy<T, TValue>(t)}
        {}

        template<class T, class TValue, class... TArgs>
        friend auto make_subscriber(TArgs&&...) -> subscriber<TValue>;

        detail::subscriber_interface<TValue>* iface_{nullptr};
    };

    template<class T, class TValue, class... TArgs>
        requires subscriber_concept<T, TValue> &&
                 std::constructible_from<T, TArgs...>
    auto make_subscriber(TArgs&&... args) -> subscriber<TValue>
    {
        return subscriber(T{std::forward<TArgs>(args)...});
    }

    /**
     * @brief Basic observable value
     * @tparam T Type of value the observable holds
     */
    template<class T>
    class observable
    {
    public:
        using type = T;
        using subscriber_type = subscriber<type>;
        using unsubscribe_fn = std::function<void(void)>;

        /**
         * @brief Default construct the observable
         */
        observable() requires std::is_default_constructible_v<type> {}
        /**
         * @brief Copy construct the observable (with constraint)
         */
        observable(const T& in) requires std::copy_constructible<type> 
            : value_{in}
        {}
        /**
         * @brief Move construct the observable (with constraint)
         */
        observable(T&& in) requires std::move_constructible<type>
            : value_{in}
        {}
        /**
         * @brief Copy assign a value to the observable and notify all subscribers
         */
        auto operator=(const T &in) -> observable& requires std::is_copy_assignable_v<type>
        {
            set(in);
            return *this;
        }
        /**
         * @brief Move assign a value to the observable and notify all subscribers
         */
        auto operator=(T &&in) -> observable& requires std::is_move_assignable_v<type>
        {
            set(in);
            return *this;
        }

        void notify_all()
        {
            for(auto *listener : listeners_)
            {
                if(!listener)
                {
                    continue;
                }
                listener->on_change(value_);
            }
        }

        /**
         * @brief Subscribes a new change listener to the store
         * @return The unregistering function
         * @details Returning the unsub function allows code like this:
         *
         * auto unsub = store.subscribe(this);
         * ...
         * unsub();
         */
        auto subscribe(subscriber_type *instance) -> unsubscribe_fn
        {
            listeners_.insert(instance).second;
            return [this, instance]() { unsubscribe(instance); };
        }
        /**
         * @brief Unregisters a change listener from the store
         */
        void unsubscribe(subscriber_type *instance)
        {
            listeners_.erase(instance);
        }

        auto get() const noexcept -> const type & { return value_; }
        auto get() noexcept -> type & { return value_; }
        /**
         * @brief Assign a new value to the observable and notify subscribers
         */
        void set(const type &value) noexcept
        {
            if(value_ != value)
            {
                value_ = value;
                notify_all();
            }
        }
    private:
        type value_;
        std::unordered_set<subscriber_type*> listeners_ {};
    };
}