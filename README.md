# Flow

Flow is a library which implements unidirectional data flow / architecture.

As of now it is still largely incomplete, but I am looking to extend and improve it over time.

## Unidirectional Data Flow

Unidirecitonal Data Flow is an approach to application design which arranges data flow in such a way that a View never directly accesses the data model and reacts to changes made by it instead. A well known example for this is [React Redux](https://react-redux.js.org/introduction/why-use-react-redux).
This architecture allows data models to be purely structures containing data, which simplifies development a lot.

## Usage

Currently, the library supports the following features:

### Reducer

A Reducer is a function which takes a state and an action to apply onto the state, and then returns the mutated state:

```pseudo
state = reducer(state, action)
```

In `flow`, a reducer is a concept for a function with the following signature:

```cpp
template<class TState, class TAction>
auto reducer(TState, TAction) -> TState;
```

Notable:

- `TAction` can be anything. The library does not dictate how actions should look and the user of the library should define the shape of an action and how it is processed by template specialization
- `TState` is pass by value to allow return type optimization

The reducer function by itself is just a function, but it is used in *Stores* as part of a system

### Store

A Store is a structure holding a specific state and updating this state with a reducer. To create a store, simply call one of its factory functions:

```cpp
// assume these declarations have definitions somewhere
namespace ns
{
    struct my_model;
    enum class my_action;

    struct my_custom_reducer
    {
        auto operator()(my_model m, my_action a) const -> my_model;
    }
}

// V1: create a store using flow::reducer<ns::my_model, ns::my_action>
// obviously you would need to specialize that template
auto store = flow::make_store<ns::my_model, 
                              ns::my_action>();
// V2: create a store using a custom reducer
// the reducer needs to be default constructible
auto store2 = flow::make_store<ns::my_model, 
                               ns::my_action,
                               ns::my_reducer>();
// V3: create a store using a lambda
// not recommended since this blows up your type definition, but nice for simple reducers
auto store3 = flow::make_store<ns::my_model,
                               ns::my_action>([](ns::my_model m, ns::my_action a) -> ns::my_model {
                                   // ... do something
                                   return m;
                               });
```

Use `store::dispatch(action a)` to apply a mutation to the state:

```cpp
store.dispatch(my_action::action_1);
```

The state is an observable, so it will notify its subscribers about a state change. To subscribe to the store, use `store::subscribe` (conveniently, this returns the function to unsubscribe again, inspired by React Redux):

```cpp
struct some_subscriber: public flow::change_listener<ns::my_state>
{
    void on_change(const my_state &s) {}
};

auto listener = some_subscriber{};
auto unsubscribe =  store.subscribe(&listener);

// later...

unsubscribe();
```

#### Async dispatch

You can dispatch asynchronous actions by passing a tag struct into `dispatch()`:

```cpp
store.dispatch(my_async_action{ .url = "..." }, flow::async_dispatch);
```

This will enqueue the the action into an asynchronous queue which will be processed on dispatch. The store will notify its subscribers as usual once the action has been processed.

### Selectors

TODO
