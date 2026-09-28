# Lifecycle

*The tickable object's contract — why ticking starts in `Initialize()` and not in the constructor, the two obligations the owner takes on, how tick intent is set, and what happens when `Shutdown()` is forgotten.*

## The problem `USCTickableObject` corrects

The engine's `FTickableGameObject` registers an object for ticking from its own constructor. That
means the engine can call `Tick()` on an object that is not finished: the derived constructor
body has not run, the Blueprint's defaults have not been applied, the owner has configured
nothing. The class default object gets registered alongside real instances, too.

`USCTickableObject` constructs with ticking set to *never* and registers itself in
`Initialize()`, which the owner calls once construction is complete. A half-built object is never
exposed to the tick pass.

## The contract

Four steps, in this order:

1. **Construct** with an outer whose chain resolves a `UWorld` — an actor, a component, the world
   itself. `Initialize()` walks the outer chain to find the world; an outer with no world (the
   transient package, for instance) makes `Initialize()` log an `Error` and leave the object
   uninitialized.
2. **Hold the object in a `UPROPERTY(TObjectPtr<>)`.** The class never roots itself. An object
   nobody references is collected like any other.
3. **Call `Initialize()`** after construction. Never from a constructor. It resolves the world,
   marks the object initialized, fires the `Initialize` event, and applies the tick intent.
4. **Call `Shutdown()` before dropping the reference.** It stops ticking, fires the `Shutdown`
   event and clears the initialized state. It is idempotent, and it is the *only* path that runs
   subclass teardown.

Steps 2 and 4 are the owner's obligations. Neither is optional, and the class checks step 4 (see
below).

In Blueprint, make a Blueprint child of `USCTickableObject`. Create it with **Construct Object
from Class**, with an actor or component as the outer, and store it in a variable. Call
`Initialize` and `Shutdown` on it — typically from the owner's `BeginPlay` and `EndPlay`.
The `Tick`, `Initialize` and `Shutdown` events are where the Blueprint's own logic goes.

## Tick intent

There is one switch — *should this object tick* — and three ways to set it:

- **`bStartTickingOnInitialize`** (default `true`) is the switch's *starting value*. It is a
  class-default property, so a Blueprint child or an archetype sets it, and it is
  [copied into the switch when the object is created, not when `Initialize()`
  runs](../reference/tickable-object.md#properties), and again by `Shutdown()`.
- **`EnableTick()` / `DisableTick()`** overwrite the switch at any time. Called *before*
  `Initialize()`, they only record the intent, and [`Initialize()` then applies it, so an explicit
  call outranks the configured default](../reference/tickable-object.md#enabletick). Called after,
  they take effect at the next tick pass.
- **`Shutdown()`** restores the switch to `bStartTickingOnInitialize`, so an
  `Initialize`/`Shutdown`/`Initialize` cycle behaves identically every time.

Two more gates sit in front of the switch and belong to the engine's tick dispatch, not to the
plugin: **`bTickWhenPaused`** (default `false`) and **`bTickInEditor`** (default `false`). Their
interaction has one surprise —
[what `bTickInEditor` does to pause handling](../reference/tickable-object.md#the-two-engine-gates).

## When the world goes away

The object watches for its world's cleanup and runs `Shutdown()` itself when the world is torn
down — the end of a PIE session, a map change. An object initialized against a world that
has since been marked garbage also stops ticking. Neither case needs anything from the owner;
both are verified.

## When you forget `Shutdown`

If the object is garbage-collected while still initialized, `BeginDestroy` fires an
[`ensure` naming the object and the `Shutdown()` that never came](../troubleshooting/diagnostics.md#usctickableobject).
It fires for *every* leaked instance, not only the first, so a leak in a loop is reported as many
times as it happens. `BeginDestroy` does **not** call `Shutdown()` for you: on the garbage
collection path the object is already flagged unreachable, and dispatching a Blueprint event to
an unreachable object is a hard crash in the engine, not a recoverable error. The ensure is the
safe substitute.

The common way to hit this in Blueprint is the **Construction Script**. It re-runs on every
reconstruction — every time the actor is moved in the editor — and each run creates a new object
while the previous one loses its reference and is collected without `Shutdown()`. The
Construction Script and the ownership contract are mutually exclusive; create the object in
`BeginPlay`, or, for an editor-world host, in an Editor Utility Widget with explicit create and
shutdown buttons.

---

← [Concepts](README.md) · [Documentation index](../../README.md#documentation)
