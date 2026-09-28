# Builds and shipping

*What each module type means for a packaged game, which entries are compiled out of Shipping, why a console command can be visible before it can run, and what has been verified where.*

## Two module types

| Module | Type | Editor | Packaged Development | Packaged Shipping / Test |
| --- | --- | --- | --- | --- |
| `StillCookingCore` | Runtime | present | present | present |
| `StillCookingCoreEditor` | UncookedOnly | present | absent | absent |

A Blueprint graph node exists only until Blueprint compilation, where it is replaced by the
ordinary engine calls it expands into. The module that defines **Print String Formatted**
therefore has to be present wherever Blueprints are *compiled* — the editor and uncooked
commandlets — and has no job in a cooked game. `UncookedOnly` is the engine's module type for
exactly that, and the same type the engine's own K2Node-hosting plugins use.

## What is gone in Shipping

**Print String Formatted** is compiled out of **Shipping and Test** builds. The node carries the
engine's *Development Only* enabled state — the dashed banner you see in the graph — and the
compiler drops it, exactly as it drops the engine's `Print String`. A graph that relies on the
node for anything but debug output stops doing that thing in Shipping and Test.

**The console command registry's registration** is compiled out of **Shipping**. Everything the
registry does at runtime — `RegisterConsoleCommand`, `On Register Commands`, the static stub —
is behind `#if !UE_BUILD_SHIPPING`, and every command it registers carries `ECVF_Cheat`. The
class itself still exists in Shipping, so a Blueprint child compiles and a call site links;
`RegisterConsoleCommand` returns `false` and `On Register Commands` is not called.

Nothing else in the plugin is compiled out.

The engine, by default, also compiles logging and `ensure` out of Shipping, so the warnings,
errors and ensures listed in [Diagnostics](../troubleshooting/diagnostics.md) do not appear in a
Shipping build.

## Visible is not executable

The console registry has two paths for defining a command, and they differ in *when the name
appears*:

- A **dynamic** command, registered from a Blueprint child's `On Register Commands`, exists from
  the moment the Game Instance starts — Play in the editor, or game start in a packaged build.
- A **static** command, declared by a C++ stub, registers itself the moment its module loads —
  in the editor, before any Play session, it is already in the console's autocomplete.

Both paths end in a Blueprint graph, and the registry lives on the Game Instance, so both can only
**execute** while a game world exists. A static command typed into the editor console outside
Play is found, runs its stub, and the registry logs a `Warning` that names the command and says
there is no game world.

## The registry's class selection in a cooked build

[The registry picks its live class by asking which subclasses
exist](../reference/console-commands.md#one-blueprint-child-per-project). In a cooked build a
Blueprint child may not be loaded yet when the subsystem collection initializes, in which case the
C++ class is created and `On Register Commands` is never called — silently. A hard reference to
the Blueprint class from something loaded early avoids it.

---

← [Concepts](README.md) · [Documentation index](../../README.md#documentation)
