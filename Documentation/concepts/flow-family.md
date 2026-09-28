# The Flow family

*One shape under four nodes: a core that keeps itself alive, a node that hands itself out on a pin, four ways to end, two timing rules, two clock options, and a warning instead of silence.*

## One core, one node

Each Flow node is a thin Blueprint adapter over a C++ core:

| Node | Core | What the core drives |
| --- | --- | --- |
| **For Each Index With Delay**, **For Each Index Per Tick** | `FSCFlowLoop` | an index from 0 to `Count − 1` |
| **Do For Duration** | `FSCFlowDuration` | an alpha from 0 to 1 |
| **Wait Until** | `FSCFlowConditionPoll` | a condition, until it answers true |

The three cores share a base, `FSCFlowTickingTask`: a world the task is bound to, a
self-reference that keeps the task alive while it runs, and exactly one way to end. Each core adds
only what it is about — iteration and delay, elapsed time and an alpha, attempts and a deadline.

The cores are not `UObject`s. A node's factory registers the node with the Game Instance, which
keeps it alive until it finishes; with no Game Instance that registration silently does nothing.
The core therefore does not rely on the node: it holds a shared reference to itself for as long
as it runs, and releases it when it finishes. If the node itself is garbage-collected mid-run, it
winds its core up from `BeginDestroy`, and the task ends without broadcasting anything. Either
way the core never outlives its work.

## Where a Flow node can go

All four nodes are async-action nodes, so they are placeable on an event graph only, not inside
a function graph.

## The proxy pin

Every Flow node has an output pin that hands out the node object itself: `Loop` on the two loop
nodes, `Action` on **Do For Duration** and **Wait Until**. That pin is what you call `Continue
Loop`, `Break Loop` or `Cancel` on.

Without that pin the graph would hold no reference to the thing it has to call. The nodes are
`BlueprintType`, so the pin can be promoted to a variable. That is how you cancel a drive from a
different event than the one that started it, or break a loop from a button rather than from
inside its own body.

## How a task ends

A Flow task ends exactly once, for one of these reasons:

| Reason | What happened | What the node fires |
| --- | --- | --- |
| Completed | the count was reached, the duration elapsed, or the condition returned true | `Completed`; on **Wait Until**, `On Satisfied` |
| Broken | `Break Loop`, `Cancel`, or a **Wait Until** whose `Condition` was lost | `Completed` — the same pin as a full run; on **Wait Until**, `On Cancelled` — or [`On Timed Out` when its `Condition` was lost](../reference/wait-until.md#outputs) |
| Timed out | a **Wait Until** deadline elapsed | `On Timed Out` |
| Aborted | the world was torn down under the task, or the task was started without a world | **nothing** — the C++ core still calls its `Finished` callback with `Aborted`; the node broadcasts nothing |

The silent ending is deliberate. Broadcasting into a world that is being torn down is the classic
async-node bug — the node fires at an actor that no longer exists. When the world goes, the task
releases itself and says nothing. Do not wire teardown logic to `Completed`; it will not fire.

### `Completed` without a reason

A Blueprint async node takes its *data* output pins from the signature of its **first** output
delegate only; the engine's async-node expansion builds them that way. On the loop nodes the
first delegate is `Loop Body`, so the only data pin is `Index`. On **Do For Duration** it is
`On Update`, so the data pins are `Alpha` and `Delta Time`. The `bCompletedFully` parameter of
`Completed` reaches no pin. Two consequences:

- **A full run and a stopped one look the same.** Both leave through `Completed`. If the graph
  needs to know, record it yourself: set a flag *before* the `Break Loop` or `Cancel` call,
  because [`Completed` can fire before that call
  returns](../reference/cpp-flow-cores.md#fscflowtickingtask). Or, from the node's `then` pin,
  use **Bind Event to Completed** on the proxy pin; the custom event it binds receives
  `bCompletedFully`. The engine runs the node's activation before `then`, so that binding misses
  a loop with `Count` `0` or less, which completes during activation.
- **The data pins keep their last value.** Read on the `Completed` path, `Index`, `Alpha` and
  `Delta Time` hold whatever the last `Loop Body` or `On Update` delivered, and `Index` is `0` if
  no iteration ran. They say nothing about how the task ended.

C++ does not have this gap: a handler bound to `Completed` with `AddDynamic` receives
`bCompletedFully` — [a node used from C++](../advanced/cpp-cores.md#using-a-node-from-c).

## Two timing rules

Both apply to all four nodes:

- **The first callback lands on a tick, never inside the node's own activation.** A loop body,
  an `On Update`, a condition poll — none of them runs synchronously from the call that starts
  the task. Three exceptions: a loop with `Count` of `0` or less fires `Completed` during
  activation; **Wait Until** with `Check Immediately` polls once during activation and can fire
  `On Satisfied` in the same frame; **Wait Until** with an unbound `Condition` fires
  `On Timed Out` during activation.
- **A second activation is ignored.** One node object runs one task. Calling a node's factory
  again produces a new node object with its own task.

## Two clocks

[**Do For Duration**](../reference/do-for-duration.md) and **Wait Until** carry two advanced pins
that look alike and are independent:

- **`Tick When Paused`** decides *whether the task runs at all* while the game is paused. Off
  (the default), a paused game freezes the task — a drive banks no time and a poll asks nothing,
  and both resume where they were on unpause. On, the task keeps running through the pause;
  that is what anything animating in a pause menu needs.
- **`Use Unscaled Time`** decides, *inside a tick that is already running*, which delta the task
  integrates: the world's dilated delta, or the application's real delta. On, a `Slomo 0.5`
  elsewhere in the game does not stretch a two-second drive to four seconds.

They are independent because they answer different questions. A pause-menu animation wants to
tick while paused and may or may not want to ignore time dilation; a hit-stop effect wants to
respect the pause and ignore dilation. The delta a task reports to its body (`Delta Time` on
`On Update`) is the delta it integrated, so a body accumulating against `Delta Time` never runs on
a different clock than the driver.

The loop nodes have neither pin. A paused game runs no iterations and banks none.

## Warnings instead of silence

Two situations in this family look, from the outside, like a task that stopped for no reason:

- a loop with `Auto Continue` cleared whose body never calls `Continue Loop`, and
- a **Wait Until** with `Timeout` = `0` whose condition never becomes true.

Neither is a fault — the task is waiting, not stuck. Both log one `Warning` on `LogStillCooking`
after 30 seconds, naming what would end the wait (`Continue Loop`; a `Timeout`) and the console
variable that controls the threshold: `SC.Flow.SuspendedLoopWarningSeconds` for the loop,
`SC.Flow.WaitUntilWarningSeconds` for **Wait Until**. Set either to `0` to switch its warning
off. The literal wording of each:
[the suspended loop](../troubleshooting/diagnostics.md#the-loop-nodes),
[the wait with no deadline](../troubleshooting/diagnostics.md#wait-until).

The threshold is a console variable and not a pin because adding a pin is a permanent change to
a stable node — too costly for a diagnostic.

---

← [Concepts](README.md) · [Documentation index](../../README.md#documentation)
