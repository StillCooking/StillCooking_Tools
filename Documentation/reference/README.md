# Reference

*How to read a card, the pins several cards share, and then every class and node with its parameters, defaults and return values; the C++ cores; the shared enums, delegates and console variables.*

One page per entry. A page is complete on its own: it lists every function, property, pin and
delegate of the entry, the literal messages the entry can log, and the pitfalls specific to it.

## How to read a card

**The availability line** says where you can call the function from. "Available in: C++ and
Blueprint" means the function is a `UFUNCTION` with both a Blueprint node and a C++ signature.
"Available in: C++" means there is no node — either the function is not reflected, or it is a
static helper meant to be called from your own C++.

**The Blueprint node name** is the node's display name in the palette, which is not always the
C++ name: the C++ `Continue` is the node `Continue Loop`, the C++ `Resolve` is the node `Get
Console Command Registry`. Cards give both.

**Defaults** are the values a freshly placed node shows on its pins, and the default arguments of
the C++ signature. They are the same.

**Events** on a Blueprintable class are marked either with the C++ override you implement
instead (`ReceiveTick_Implementation` for the tickable object's `Tick` event) or as *Blueprint
event only* — the console registry's `On Register Commands` and `On Static Command` have no C++
override.

## Pins the cards share

Six pins appear on more than one node with the same meaning. The cards list them all, because a
card is complete on its own; here they are once, with the page that explains each in full.

| Pin | On | What it is |
| --- | --- | --- |
| `WorldContextObject` | every Flow node; also `Get Console Command Registry` | Hidden in Blueprint, filled in by the graph's own `self`. The cards list it because a C++ caller has to supply one, and [a Flow task started without a world is aborted](cpp-flow-cores.md#fscflowtickingtask) — the registry returns `nullptr` instead. |
| The proxy pin | every Flow node | The output pin that hands out the node object itself — `Loop` on the loops, `Action` on the others. It is what you call `Continue Loop`, `Break Loop` or `Cancel` on. [Why the pin exists, and promoting it to a variable](../concepts/flow-family.md#the-proxy-pin). |
| `Count` | both loop nodes | Iterations, from `0` to `Count − 1`. `0` or less completes immediately, without a single body call — [one of the three cases where a Flow callback does not wait for a tick](../concepts/flow-family.md#two-timing-rules). |
| `Auto Continue` | both loop nodes | Checked (the default), the loop advances as soon as `Loop Body` returns; cleared, it waits for `Continue Loop`. [Which one a body needs, and what happens when it never arrives](for-each-index.md#auto-continue-and-the-latent-body-protocol). |
| `Tick When Paused` | **Do For Duration**, **Wait Until** | Whether the task runs at all while the game is paused. Off by default, so a paused game freezes it. [What each of the two clock pins decides](../concepts/flow-family.md#two-clocks). |
| `Use Unscaled Time` | **Do For Duration**, **Wait Until** | Which delta the task integrates once it is *already* running: the world's dilated delta, or the application's real one. [Independent of `Tick When Paused`, and why](../concepts/flow-family.md#two-clocks). |

The loop nodes have neither clock pin: a paused game runs no iterations and banks none. Everything
else in a card's parameter table — `Delay`, `Curve`, `Condition`, `Items Per Tick` — is that
entry's own input. The exec output pins differ in name from card to card but share their
[delegate signatures](types.md#dynamic-delegates); the data output pins come from the first
delegate only — [which is why `Completed` carries no reason in
Blueprint](../concepts/flow-family.md#completed-without-a-reason).

## What is not here

- **Engine behaviour.** The pages assume you know what a tick, a subsystem, a Game Instance and a
  latent node are. Where an entry's behaviour depends on a specific engine mechanism, the page
  names the mechanism and stops.
- **Design rationale.** The *why* lives in [Concepts](../concepts/README.md); the cards say what
  happens.

## Pages in this section

| Page | What it covers |
| --- | --- |
| [Tickable object](tickable-object.md) | `USCTickableObject` — lifecycle functions, tick intent, interval, the two engine gates, the events, and the messages it logs. |
| [Console commands](console-commands.md) | `USCConsoleCommandRegistrySubsystem` — the dynamic and the static path, registration and cleanup, and the naming convention. |
| [For Each Index](for-each-index.md) | **For Each Index With Delay** and **For Each Index Per Tick** — the loop node, its latent-body protocol, `Continue Loop` and `Break Loop`. |
| [Do For Duration](do-for-duration.md) | **Do For Duration** — the alpha drive, the curve, the final-frame guarantee, `Cancel`. |
| [Wait Until](wait-until.md) | **Wait Until** — the condition function, poll interval, timeout, the three exec outputs, and the binding rule that fails at compile time. |
| [Print String Formatted](print-string-formatted.md) | The graph node — argument pins from braces, accepted types, the option pins, the number-formatting caveat, and why there is no C++ side. |
| [C++ Flow cores](cpp-flow-cores.md) | `FSCFlowTickingTask`, `FSCFlowLoop`, `FSCFlowDuration`, `FSCFlowConditionPoll` and their parameter structs — the EXPERIMENTAL layer. |
| [Types](types.md) | `ESCFlowStep`, `ESCFlowFinish`, the delegate signatures, the function aliases, the console variables and the log category. |

← [Documentation index](../../README.md#documentation)
