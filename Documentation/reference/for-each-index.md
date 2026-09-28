# For Each Index

***For Each Index With Delay** and **For Each Index Per Tick** — the loop node, its latent-body protocol, `Continue Loop` and `Break Loop`.*

## `USCFlowForEachIndexWithDelay`

*Available in: C++ and Blueprint.* `UBlueprintAsyncActionBase`, `BlueprintType`, proxy pin
`Loop`. Module `StillCookingCore`, header `Flow/SCFlowForEachIndexWithDelay.h`. Category
`StillCooking|Flow`. Built on `FSCFlowLoop`.

Two palette entries, both created from this class, both running a body once per index from `0`
to `Count − 1`. They differ in what separates one iteration from the next: a delay on the clock,
or the next frame.

## `For Each Index With Delay`

- **C++ call** — `USCFlowForEachIndexWithDelay::ForEachIndexWithDelay(WorldContextObject, Count, Delay, bDelayBeforeFirstIteration, bAutoContinue)`, then `Activate()`
- **Blueprint node** — `For Each Index With Delay`

| Parameter | Pin | Default | Meaning |
| --- | --- | --- | --- |
| `WorldContextObject` | hidden | — | filled in by `self` |
| `Count` | `Count` | none in C++; the pin shows `0` | number of iterations; `0` or less completes during activation, without a single iteration |
| `Delay` | `Delay` | `0.2` | seconds between the end of one iteration and the start of the next; `0` or less means the next tick |
| `bDelayBeforeFirstIteration` | `Delay Before First Iteration` | `false` | wait one `Delay` before index `0` as well |
| `bAutoContinue` | `Auto Continue` | `true` | advanced; see below |

[The first iteration lands on the first tick after activation, never
synchronously](../concepts/flow-family.md#two-timing-rules). The delay clock runs only *between*
iterations: with `Auto Continue` cleared, it does not advance while a latent body is in flight.
After a hitch longer than several delays, one iteration runs, not a burst; the part of the hitch
below one `Delay` carries into the next wait. There is no delay after the last iteration;
`Completed` follows it immediately, and a trailing pause is a `Delay` node after `Completed`.

## `For Each Index Per Tick`

- **C++ call** — `USCFlowForEachIndexWithDelay::ForEachIndexPerTick(WorldContextObject, Count, ItemsPerTick, bAutoContinue)`, then `Activate()`
- **Blueprint node** — `For Each Index Per Tick`

| Parameter | Pin | Default | Meaning |
| --- | --- | --- | --- |
| `WorldContextObject` | hidden | — | filled in by `self` |
| `Count` | `Count` | none in C++; the pin shows `0` | number of iterations; `0` or less completes during activation, without a single iteration |
| `ItemsPerTick` | `Items Per Tick` | `1` | iterations per frame; values below `1` are treated as `1`; there is no upper bound |
| `bAutoContinue` | `Auto Continue` | `true` | advanced; see below |

Runs `ItemsPerTick` iterations in one frame, then waits for the next frame. The node has no
`Delay` pin. Use it when one iteration is cheap and you want the whole loop to finish in a few
frames without stalling any one of them. A value at or above `Count` runs the whole loop in one
frame; there is no upper bound.

**With `Auto Continue` cleared, `ItemsPerTick` has no effect.** A suspended body ends the
current batch, and the rest of the batch is skipped, not made up later — the loop degrades to
one iteration per frame by itself.

![For Each Index Per Tick scanning an array of actors for invalid entries](per-tick-scan-graph.png)

***For Each Index Per Tick** with `Count` = `1000` and `Items Per Tick` = `50`: `Loop Body` checks
the item at `Index`, fifty times per frame, and `Completed` fires after twenty frames.*

## `Auto Continue` and the latent-body protocol

`Loop Body` is a delegate; the node broadcasts it once per index and cannot see what the graph
does with it. Two kinds of body exist:

- A **synchronous** body — a chain of ordinary nodes — has finished when the broadcast returns.
- A **latent** body — one containing a `Delay`, a `Move Component To`, an asset load — has
  *returned* long before it has *finished*.

**`Auto Continue` checked (the default):** the loop moves on as soon as the broadcast returns.
Right for a synchronous body, which then needs nothing beyond the loop itself. With a latent
body, the loop does not wait: the next iteration starts on schedule while the previous body is
still in flight.

**`Auto Continue` cleared:** the loop parks after each broadcast and waits for `Continue Loop`.
Right for a latent body: call `Continue Loop` on the `Loop` pin as the last thing the body does.
A body that never sends one parks the loop for good — and [after 30 seconds the loop logs a
`Warning` saying exactly that](../concepts/flow-family.md#warnings-instead-of-silence).

Calling `Continue Loop` while `Auto Continue` is checked is safe: the loop counts one advance for
the iteration, not two. A `Continue Loop` sent mid-delay, between iterations, is ignored rather
than skipping a body call.

The default is *checked* because the other default failed silently: a synchronous body with the
`Loop` pin left unconnected ran one iteration and stopped forever, with no error and no log.
Checked, a synchronous body needs nothing; cleared, a latent body needs one checkbox and one
call — and a forgotten call now produces a warning.

## Functions

### `Continue()`

*Available in: C++ and Blueprint.*

- **C++ call** — `Node->Continue()`
- **Blueprint node** — `Continue Loop`, on the `Loop` pin

Reports the current iteration as finished and schedules the next one — never synchronously: on
the next tick when `Delay` is `0`, otherwise once the delay has elapsed. Does nothing when the
loop is not waiting for it: under `Auto Continue`, or between iterations. On the last index there
is no next iteration: a `Continue Loop` sent after the body returned ends the loop inside the
call, and `Completed` fires before `Continue Loop` returns.

### `Break()`

*Available in: C++ and Blueprint.*

- **C++ call** — `Node->Break()`
- **Blueprint node** — `Break Loop`, on the `Loop` pin

Ends the loop early and fires `Completed` — the same pin as a full run; in Blueprint
[nothing tells the two apart](../concepts/flow-family.md#completed-without-a-reason). Works in
either mode, from inside the body or from anywhere else that holds the `Loop` pin. From anywhere
else, `Completed` fires before `Break Loop` returns. A `Break Loop` from inside the
body is applied after the body returns, and wins over a `Continue Loop` sent from the same body.
A second `Break Loop` is a no-op.

## Outputs

| Pin / delegate | Type | When |
| --- | --- | --- |
| `Loop Body` (`LoopBody`) | `FSCFlowIndexSignature(int32 Index)` | once per index, zero-based |
| `Completed` (`Completed`) | `FSCFlowCompletedSignature(bool bCompletedFully)` | once, when the loop ends; `bCompletedFully` is `false` when `Break Loop` stopped it early — C++ only, the Blueprint node has no pin for it |
| `Loop` | `USCFlowForEachIndexWithDelay*` | the node itself, for `Continue Loop` and `Break Loop` — [the proxy pin every Flow node carries](../concepts/flow-family.md#the-proxy-pin) |

[`Completed` does **not** fire when the world is torn down under the
loop](../concepts/flow-family.md#how-a-task-ends).

In Blueprint the node's only data pin is `Index`, from `Loop Body`. On the `Completed` path it
still holds the last index delivered, or `0` if none was —
[why `Completed` carries no reason](../concepts/flow-family.md#completed-without-a-reason).

## Messages

Two of the plugin's own, both on `LogStillCooking`, plus one from the engine. The exact wording
of [the suspended-loop warning](../troubleshooting/diagnostics.md#the-loop-nodes), of
[the message every Flow node can write](../troubleshooting/diagnostics.md#flow-all-nodes) and of
the engine's warning that precedes it on the loop nodes:

| When | Level |
| --- | --- |
| `Auto Continue` is cleared and no `Continue Loop` arrives for longer than the threshold | Warning, once per loop |
| the world context resolved no world, so nothing fires | Error |
| the same, reported first by the engine's world lookup, which names the world context object | Warning, engine's own |

The threshold is the console variable `SC.Flow.SuspendedLoopWarningSeconds`; `0` or less disables
the warning — [why it is a variable and not a
pin](../concepts/flow-family.md#warnings-instead-of-silence). The warning fires once per loop
instance and never again, even across several suspensions; a loop resumed under the threshold
says nothing, and the clock restarts on each suspension.

## Pitfalls

- **One iteration, then silence** is a latent body with `Auto Continue` cleared and no
  `Continue Loop`. Wait for the warning or check the graph.
- **A latent body under `Auto Continue`** is not waited for — the next body starts on schedule
  while the previous one is still in flight. Clear `Auto Continue` if that is not what you want.
- **`Delay` = `0`** on the delayed node is one iteration per tick, the same as the per-tick node
  at `Items Per Tick` = `1`.
- **`Delay Before First Iteration`** defers index `0` by one `Delay`; without it index `0` runs
  on the first tick regardless of `Delay`.

---

← [Reference](README.md) · [Documentation index](../../README.md#documentation)
