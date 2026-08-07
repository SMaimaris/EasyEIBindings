<img src="Resources/Icon128.png" align="right" width="110" alt="Easy EI Bindings icon">

# Easy EI Bindings

**Convention-over-configuration input bindings for Unreal Engine's Enhanced Input.**

Add a component, name your handler functions after your Input Actions, and they are bound automatically at runtime — no per-action `BindAction` boilerplate. The included editor tooling creates Input Action assets, generates the handler stubs for you (C++ or Blueprint), and shows at a glance which bindings are live.

![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.x-313131?logo=unrealengine)
![License: MIT](https://img.shields.io/badge/License-MIT-blue)

## How it works

At `BeginPlay`, the **Easy EI Bindings** component looks at each configured Input Action and searches its owning actor for functions following this naming convention:

```
IA_<ActionName>_<TriggerEvent>
```

| Input Action asset | Trigger event | Bound handler |
|---|---|---|
| `IA_Jump` | Triggered | `IA_Jump_Triggered` |
| `IA_Jump` | Completed | `IA_Jump_Completed` |
| `IA_Sprint` | Started | `IA_Sprint_Started` |

Every handler it finds is bound through the owner's `UEnhancedInputComponent`; anything missing is simply skipped. Which trigger events are considered (`Triggered`, `Started`, `Ongoing`, `Completed`, `Canceled`) is selectable per action straight from the details panel.

## Features

- **Auto-binding by naming convention** — no manual `BindAction` calls; handlers can be C++ `UFUNCTION`s or Blueprint custom events.
- **Per-action trigger event selection** — a bitmask per Input Action controls exactly which of the five trigger events get bound.
- **C++ stub generation** — one click inserts ready-to-fill `IA_..._Triggered(const FInputActionValue& Value)` declarations and definitions into the owner class's header and source, inside clearly marked regions. Existing functions are never duplicated.
- **Blueprint event generation** — for Blueprint actors, one click drops the matching Enhanced Input Action event nodes into the Event Graph.
- **Asset workflow helpers** — create a new Input Action asset (pre-wired into the component) or bulk-add existing ones via an asset picker, without leaving the details panel.
- **Binding status at a glance** — the details panel shows how many enabled handlers are bound versus missing.
- **Project-wide defaults** — default asset folder, asset name prefix, and default trigger events are configurable under *Project Settings → Plugins → Easy EI Bindings*.

## Installation

1. Copy the repository into your project's `Plugins` folder:
   ```
   YourProject/Plugins/EasyEIBindings/
   ```
2. Regenerate project files and build (a C++ project is required to compile the plugin).
3. Enable **Easy EI Bindings** in *Edit → Plugins* if it isn't enabled automatically, and make sure the **Enhanced Input** plugin is enabled (it is by default in UE 5.1+).

## Quick start

### C++

1. Add the **Easy EI Bindings** component to your pawn, character, or controller.
2. Add your Input Actions to the component's **Input Bindings** array (or use *Add IAs from Folder…* / *+ New Input Action…*), and pick the trigger events you care about for each.
3. Click **Generate Function Stubs**. The plugin inserts the handlers into your class:
   ```cpp
   protected:
       //Input actions
       UFUNCTION()
       void IA_Jump_Triggered(const FInputActionValue& Value);

       UFUNCTION()
       void IA_Jump_Completed(const FInputActionValue& Value);
       //Input actions END
   ```
4. Rebuild, fill in the function bodies, and play. The component binds everything on `BeginPlay`.

Your Input Mapping Context still needs to be added to the local player subsystem as usual — the plugin takes over the *binding* side, not context management.

### Blueprint

1. Add the **Easy EI Bindings** component to your Blueprint actor.
2. Configure the **Input Bindings** array the same way.
3. Click **Generate BP Event Stubs** to place the Enhanced Input event nodes in the Event Graph, ready to wire up.

## Runtime API

| Function | Description |
|---|---|
| `SetupInputActions(EnhancedInputComponent)` | Binds all matching handlers. Called automatically on `BeginPlay`; pass an input component explicitly to bind against something other than the owner's. |
| `RebindInputActions()` | Clears this component's bindings and binds again — useful after possession changes. |
| `ClearInputBindings()` | Removes every binding this component has made. |

## Project settings

*Project Settings → Plugins → Easy EI Bindings*

| Setting | Default | Description |
|---|---|---|
| Default Input Action Path | `/Game/Input` | Folder offered when creating new Input Action assets. |
| Default Enabled Events | `Triggered, Completed` | Trigger events enabled when an action is added to a component. |
| Input Action Prefix | `IA_` | Prefix for new assets; stripped from asset names when deriving handler names. |
| Show Binding Status | `true` | Toggles the bound/missing summary in the details panel. |

## Requirements

- Unreal Engine 5.x (developed and tested against 5.8) with the Enhanced Input plugin.
- A C++ project (the plugin ships as source).

## License

[MIT](LICENSE) — © Stylianos Maimaris
