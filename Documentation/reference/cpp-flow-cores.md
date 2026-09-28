# C++ Flow cores

*`FSCFlowTickingTask`, `FSCFlowLoop`, `FSCFlowDuration`, `FSCFlowConditionPoll` and their parameter structs — the EXPERIMENTAL layer.*

> [!warning] EXPERIMENTAL
> [Everything on this page may change shape without a major version bump, and twice it
> has](../concepts/overview.md#the-stable-surface-and-the-experimental-layer). The
> Blueprint nodes built on these classes are stable. **If you call the cores from C++, read the
> changelog on every update.**

All classes live in module `StillCookingCore`, under `Flow/`. They are plain C++ — not `UObject`s,
not `USTRUCT`s — created with a static `Start()` that returns a `TSharedRef`. [A running core
holds a shared reference to itself](../concepts/flow-family.md#one-core-one-node), so **the caller
may drop the returned reference immediately**; the core is destroyed once it has finished and
nothing else holds it. Keep a `TSharedPtr` or `TWeakPtr` if you intend to call `Continue()`,
`Break()` or `Cancel()` later.

The signatures below follow the headers.

## `FSCFlowTickingTask`

Header `Flow/SCFlowTickingTask.h`. The part every core has in common: a world it is bound to, a
self-reference that keeps it alive while it runs, and exactly one way to end. You do not use it
directly; its behaviour is what the three cores share:

- **No body call, update or poll runs synchronously from `Start()`** — the first one lands on
  the first tick. The one exception is `FSCFlowConditionPoll` with `bCheckImmediately`, which
  polls once inside `Start()`.
- **`Finished` can run inside `Start()`.** A task started without a world finishes as `Aborted`
  (every core), and `FSCFlowLoop` with `Count` `0` or less finishes as `Completed`; with no
  world the loop aborts whatever its `Count`. Either way `Start()` returns a core that has
  already finished: `Finished` ran *before* the returned reference reached your variable, so a
  handle assigned from `Start()` holds an inert core and `IsRunning()` on it is `false`.
- **A call that ends the task runs `Finished` before it returns**, unless it is deferred:
  `Break()` on a loop from outside its body, `Cancel()` on a condition poll from outside its
  condition, `Cancel()` on a duration from anywhere, `Continue()` on a suspended loop's last
  index, and `FlushImmediateCheck()` after an immediate poll that returned `true`.
- **A task started without a world is aborted** with
  [an `Error` on `LogStillCooking` saying so](../troubleshooting/diagnostics.md#flow-all-nodes).
  The `Finished` callback receives `ESCFlowFinish::Aborted`.
- **[World teardown aborts the task](../concepts/flow-family.md#how-a-task-ends)**, calls
  `Finished` with `Aborted`, and releases the core.
- **`Finished` is called exactly once**, for any reason. A second `Break()` or `Cancel()` is a
  no-op.
- **`IsRunning()`** is `true` from `Start()` until the task finishes. It turns `false` just
  before `Finished` is called, so inside `Finished` it already reads `false`.

## `FSCFlowLoop`

Header `Flow/SCFlowLoop.h`. Runs a body once per index, waiting a delay between iterations. The
core under both loop nodes.

```cpp
static TSharedRef<FSCFlowLoop> Start(UWorld* World, const FSCFlowLoopParams& Params,
                                     FSCFlowLoopBody Body, FSCFlowFinished Finished);
void Continue();
void Break();
bool IsRunning() const;
```

| `FSCFlowLoopParams` field | Meaning |
| --- | --- |
| `Count` | iterations; `0` or less finishes as `Completed` inside `Start()` without a body call |
| `Delay` | seconds between iterations; `0` or less means one iteration (or one batch) per tick |
| `ItemsPerTick` | iterations per tick; below `1` is treated as `1`; default `1` |
| `bDelayBeforeFirstIteration` | wait one `Delay` before index `0` |

**The body returns [`ESCFlowStep`](types.md#escflowstep).** A synchronous body returns `Continue`
and never learns the `Continue()`/`Break()` protocol exists. A latent body returns `Suspend` and
owes the loop a `Continue()` or `Break()` once it is done. `Continue()` lands the next iteration
on the next tick (with `Delay` at `0`) or after the delay, never synchronously; on the last
index there is no next iteration, and `Continue()` finishes the loop as `Completed` inside the
call. The delay clock
does not advance while the loop is suspended, and a body that never answers is [reported by the
suspension warning after 30 seconds](../concepts/flow-family.md#warnings-instead-of-silence).
Returning `ESCFlowStep::Break` ends the loop as `Broken` after the body returns.

```cpp
FSCFlowLoopParams Params;
Params.Count = 3;
Params.Delay = 0.5f;

TSharedRef<FSCFlowLoop> Loop = FSCFlowLoop::Start(World, Params,
	[](int32 Index)
	{
		// ... work for this index ...
		return ESCFlowStep::Continue;
	},
	[](ESCFlowFinish Reason)
	{
		// Completed, Broken or Aborted
	});
```

A latent body, answered from outside:

```cpp
TSharedRef<FSCFlowLoop> Loop = FSCFlowLoop::Start(World, Params,
	[](int32) { return ESCFlowStep::Suspend; },
	[](ESCFlowFinish) {});

// later, when the asynchronous work for the current index is done:
Loop->Continue();   // the next iteration lands on a later tick, never synchronously
```

Batching:

```cpp
Params.Count = 6;
Params.Delay = 0.f;
Params.ItemsPerTick = 3;   // three iterations per tick, done in two ticks
```

`Delay > 0` together with `ItemsPerTick > 1` is legal and means "run this many, then wait". No
Blueprint node exposes the combination; it is reachable from C++ only. A `Suspend` ends the
current batch, and the rest of the batch is skipped, not made up later.

**Constraint on the body.** The body must not synchronously tear down the world the loop runs on.
Everything else that can end the loop from inside the body — `Break()`, `Continue()`, a returned
`Break` — is deferred until the body returns; world cleanup is not, and finishing clears the body
`TFunction` while its own `operator()` is on the stack.

## `FSCFlowDuration`

Header `Flow/SCFlowDuration.h`. Drives a normalized alpha from `0` to `1` over a duration,
calling an update every tick, and guarantees a final update at raw progress exactly `1`. The
core under **Do For Duration**.

```cpp
static TSharedRef<FSCFlowDuration> Start(UWorld* World, const FSCFlowDurationParams& Params,
                                         FSCFlowDurationUpdate Update, FSCFlowAlphaShaper Shaper,
                                         FSCFlowFinished Finished);
void Cancel();
bool IsRunning() const;
```

| `FSCFlowDurationParams` field | Meaning |
| --- | --- |
| `Duration` | seconds; `0` or less delivers one update at alpha `1` on the first tick, then completes |
| `bTickWhenPaused` | keep driving while the world is paused |
| `bUseUnscaledTime` | integrate `FApp::GetDeltaTime()` instead of the world's dilated delta |

**The shaper** is a `TFunction<float(float)>`, not a curve asset: hand it any easing — `[](float
T){ return FMath::InterpEaseOut(0.f, 1.f, T, 2.f); }` — or an empty `FSCFlowAlphaShaper()` for the
identity. It is called with the raw progress, clamped to `0..1`, and [its *result* is never
clamped](do-for-duration.md#the-final-frame-guarantee); the last update carries `Shape(1)`. The
node wraps its curve asset in a shaper that captures the asset weakly and degrades to the identity
if the asset is gone; the core never learns curve assets exist.

```cpp
FSCFlowDurationParams Params;
Params.Duration = 1.f;

TSharedRef<FSCFlowDuration> Drive = FSCFlowDuration::Start(World, Params,
	[](float Alpha, float DeltaTime) { /* apply Alpha */ },
	FSCFlowAlphaShaper(),                  // identity
	[](ESCFlowFinish Reason) {});
```

**Constraint on the update — a latent use-after-free.** Nothing in the update path is deferred.
Cancelling the drive from inside `Update` — through `Drive->Cancel()`, or through the node's
`Cancel()` from an `On Update` handler — clears the update `TFunction` while its own `operator()`
is on the stack. Make the cancelling statement the last one that touches a capture. The node's
own handler follows that rule today; a statement added after the cancel would turn it into a real
use-after-free.

## `FSCFlowConditionPoll`

Header `Flow/SCFlowConditionPoll.h`. Polls a condition every `PollInterval` until it returns
`true`, and gives up on a hard deadline. The core under **Wait Until**; its parameter struct is
`FSCFlowWaitUntilParams`.

```cpp
static TSharedRef<FSCFlowConditionPoll> Start(UWorld* World, const FSCFlowWaitUntilParams& Params,
                                              FSCFlowCondition Condition, FSCFlowFinished Finished);
void FlushImmediateCheck();
void Cancel();
bool IsRunning() const;
```

| `FSCFlowWaitUntilParams` field | Meaning |
| --- | --- |
| `PollInterval` | seconds between polls; `0` or less polls every tick |
| `Timeout` | seconds; `0` or less means no deadline |
| `bCheckImmediately` | poll once inside `Start()`; see `FlushImmediateCheck()` |
| `bTickWhenPaused` | keep polling, and keep the deadline advancing, while paused |
| `bUseUnscaledTime` | integrate real time |

**The deadline is tested before the poll** on every tick, with `>=`: a condition that would
answer `true` on the frame the deadline lands does not win, and a `PollInterval` at or above
`Timeout` means the condition is never asked on a tick (only the `bCheckImmediately` poll, if
set, happens). `Finished` receives `ESCFlowFinish::TimedOut`.

**`FlushImmediateCheck()` is the C++ caller's obligation with `bCheckImmediately`.** `Start()`
polls once but only *records* the answer, so that the caller's condition is not invoked from
inside the caller's own construction with a finish already broadcast. Call `FlushImmediateCheck()`
right after `Start()` returns; it turns a recorded `true` into `Finished(Completed)`. A caller
that never flushes loses a recorded `true`, has already spent attempt `0`, and — if the
immediate check itself cancelled the task — has that cancel honoured only after the first tick's
poll. The node does this for you.

```cpp
FSCFlowWaitUntilParams Params;
Params.PollInterval = 0.25f;
Params.Timeout = 1.f;

TSharedRef<FSCFlowConditionPoll> Task = FSCFlowConditionPoll::Start(World, Params,
	[](int32 Attempt) { return /* are we there yet? */ false; },
	[](ESCFlowFinish Reason) { /* Completed, TimedOut, Broken or Aborted */ });

// with Params.bCheckImmediately = true, additionally:
Task->FlushImmediateCheck();
```

**A `Cancel()` from inside the condition is deferred** until the condition returns, so — unlike
`FSCFlowDurationUpdate` — the closure is never freed while its own `operator()` is on the stack.
World teardown is the one route that is not deferred, as for `FSCFlowLoopBody`.

**The no-deadline warning** fires once per task after `SC.Flow.WaitUntilWarningSeconds` (default
30 s) when `Timeout` is `0` or less; a task with a deadline never warns.

---

← [Reference](README.md) · [Documentation index](../../README.md#documentation)
