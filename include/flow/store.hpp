#pragma once
#include "observable.hpp"
#include "reducer.hpp"

namespace flow
{
    /**
     * @brief A redux-like store holding a state and a reducer function to produce mutations on the state
     * @tparam TState State type held by the store
     * @tparam TAction Action type dispatched by the store
     * @tparam TReducer The reducer functor to use for the store
     */
    template<class TState, 
             class TAction, 
             reducer_concept<TState, TAction> TReducer = reducer<TState, TAction>>
    class store: public observable<TState>
    {
    public:
        using state_type = TState;
        using base = observable<state_type>;
        using action_type = TAction;
        using reducer_type = TReducer;
        /**
         * @brief Construct a store and default initialize the state
         */
        store() requires std::is_default_constructible_v<state_type>
            : base()
        { }
        /**
         * @brief Construct a store with an initial state
         */
        explicit store(const state_type &initial_state)
            : base(initial_state)
        { }
        /**
         * @brief Create a store with an initial state constructed from the passed parameters
         */
        template<class... TArgs>
            requires std::constructible_from<state_type, TArgs...>
        static auto create(TArgs&&... args) -> store
        {
            return store {state_type{std::forward<TArgs>(args)...}};
        }
        /**
         * @brief Dispatch an action to the state and notify listeners about a state
         */
        void dispatch(const action_type &action)
        {
            base::set(reducer_(base::get(), action));
        }		
    private:
        reducer_type reducer_{};
    };
}