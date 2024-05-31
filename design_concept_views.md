# `flow` usage with different UI frameworks

This document contains some notes about how flow could / is desired to be used.

## ImGUI

Since **ImGUI** is immediate mode we can assume the following:

```pseudo
view = f(mutable state)
```

For example:

```cpp
struct counter
{
    int value;
};

void counter_component(counter& state)
{
    ImGui::Label(std::to_string(state.value).c_str());
    if(ImGui::Button("Increment"))
    {
        state.value++;
    }
    if(ImGui::Button("Decrement"))
    {
        state.value--;
    }
    if(ImGui::Button("Reset"))
    {
        state.value = 0;
    }
}
```

The problem this causes is that state may change while still going down the render tree which causes state tearing (inconsistend state in the same frame).

To fix this, we can render with an immutable *current* state and a dispatch context to mutate it in the next frame

```pseudo
view = f(immutable state, dispatch_context)
```

Combined with `flow` (using the `counter` example implementation):

```cpp
void counter_component(counter state,
                       std::shared_ptr<flow::dispatch_context<counter_action>> ctx)
{
    ImGui::Label(std::to_string(state.value).c_str());
    if(ImGui::Button("Increment"))
    {
        ctx->dispatch(counter_action::increment);
    }
    if(ImGui::Button("Decrement"))
    {
        ctx->dispatch(counter_action::decrement);
    }
    if(ImGui::Button("Reset"))
    {
        ctx->dispatch(counter_action::reset);
    }
}
```

* Since the UI is newly evaluated **every frame** we dont need to relay on lenses to get data
* The state is passed into the view function every frame, dispatched actions are applied to the state *which will be passed in next frame*

### Object oriented ImGui wrappers

Some people use components wrapping around a view function, which means they still hold some state.

With the counter example from before that could look like this:

```cpp
struct counter_component
{
    flow::lens<store<counter, counter_action>, int> counter_value{store, [](counter state) { return state.value; }};
    // stateful components could hold more state here

    void view(std::shared_ptr<flow::dispatch_context<counter_action>> ctx)
    {
        /* code from before, swap out state.value to *counter_value */
    }
};
```

* notice that we are using `flow::lens` here to keep ourselves updated about a part of the state
  * this might seem unnecessary here but can be pretty useful with bigger states

## Godot / QML

In this case the UI lives in a different runtime and calls into types defined by us. We can:

* expose a "service" of some sort to the runtime which under the hood dispatches actions to the store and emits events on store updates

```cpp
class counter_service: flow::subscriber<counter>
{
    // Framework boilerplate... either QML or GDNative
    // Either can be registered as a singleton (the service is THE application state) or attached to the runtime sometime
public:
    counter_service()
    {
        store_->subscribe(this);
    }

    ~counter_service()
    {
        store_->unsubscribe(this);
    }

    // Public API: these functions will be exposed to the runtime (QObject / godot::Object)
    void increment()
    {
        store_->dispatch(counter_action::increment);
    }

    void decrement()
    {
        store_->dispatch(counter_action::decrement);
    }

    void reset()
    {
        store_->dispatch(counter_action::reset);
    }

    // overrides
    void handle_change(const counter& counter) override
    {
        // emit the state_changed signal
        emit state_changed();
    }
signals: // or register Godot signals
    void state_changed();
private:
    std::shared_ptr<flow::store<counter, counter_action>> store_ = flow::make_store<counter, counter_action>();
};
```

* write the component as an extension into C++
  * the code would be roughly the same as the `counter_service`, the difference is how the object is used in the framework

## Additional Notes

* there could be another library that specializes on integrating `flow` into `ImGui`
  * name proposal: `river` (to keep the "flow" theme)
  * should probably also implement the base loop implementation of `ImGui`, probably the GLFW/OpenGL backend

API for `river` (roughly)

```cpp
// as defined in example
struct counter;
enum class counter_action;
void counter_view(counter state, std::shared_ptr<flow::dispatch_context<counter, counter_action>> ctx);

int main()
{
    auto store = flow::make_store<counter, counter_action>();
    /*
    subject to change
    basically we just need something to hold the graphics subsystems and window information
    we pass some basic parameters to define the window containing our UI
    the window object then handles the rest
    */
    auto window = river::make_window("Counter Application", 800, 600);
    /*
    here we pass the store as a dispatch context and the view function to construct the render loop
    the window starts the view loop here. in the loop it passes the stores current state and the store itself as a dispatch context
    */
    window.view(store, counter_view);

    return 0;
}
```
