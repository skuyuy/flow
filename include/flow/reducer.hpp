#pragma once
#include <concepts>

namespace flow
{
    template<class T, class TState, class TAction>
    concept reducer_concept = std::invocable<T, TState, TAction> &&
                              std::same_as<std::invoke_result_t<T, TState, TAction>,
                                           TState>;

    template<class TState, class TAction>
    struct reducer
    {
        auto operator()(TState state, TAction action) -> TState
        {
            static_assert("reducer not implemented!");
            return state;
        }
    };
}