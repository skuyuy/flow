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
}

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
                       std::shared_ptr<flow::dispatch_context<counter_action> ctx)
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
