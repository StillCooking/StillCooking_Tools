# Using the C++ cores

*When a core is the right tool instead of a node, how to keep a handle, how to answer a suspended loop, and the two constraints on callbacks.*

The signatures are on [C++ Flow cores](../reference/cpp-flow-cores.md). Everything here is
EXPERIMENTAL, so [a C++ caller reads the changelog on every update](../reference/cpp-flow-cores.md).

## Node or core

Use the **node** — even from C++ — when the body has to reach Blueprint: a designer's event, a
widget, an animation. The node's delegates are dynamic, bind to `UFUNCTION`s, and are safe under
garbage collection.

Use the **core** when the whole thing is C++: the body is a lambda, there is no `UObject` you
want to create for it, and you would rather not have a `UBlueprintAsyncActionBase` in the
picture. A core costs no `UObject`, needs no Game Instance, and its body is a `TFunction` with a
return value that a node cannot offer.

## Keeping a handle

`Start()` returns a `TSharedRef`. [The core holds a reference to *itself* while it
runs](../concepts/flow-family.md#one-core-one-node), so the returned reference is not what keeps
it alive — it is your handle for `Continue()`, `Break()` and `Cancel()`. Three ways to hold it:

- **Drop it.** A fire-and-forget loop or drive needs nothing kept:
  `FSCFlowLoop::Start(World, Params, Body, Finished);` and move on.
- **Keep a `TWeakPtr<FSCFlowLoop>`** on the object that started it, to `Break()` from `EndPlay`
  without extending the core's life by a frame. As long as nothing else holds a `TSharedPtr`,
  `IsValid()` on the weak pointer tells you whether the task is still running, since the core is
  destroyed when it finishes; `IsRunning()` is the direct question. It already reads `false`
  inside `Finished`.
- **Keep a `TSharedPtr`** when something else must inspect the core after it finishes. This is
  rarely needed and keeps the (finished, inert) core alive until you release it.

[A world torn down under the task aborts it](../concepts/flow-family.md#how-a-task-ends) and
releases it; a `TWeakPtr` goes invalid, a `TSharedPtr` keeps an inert object.

## Answering a suspended loop

A latent body returns `ESCFlowStep::Suspend` and then, from wherever the asynchronous work
completes, calls `Continue()` on the loop. The callback that completes the work needs a handle to
the loop, and the loop is created *by* the `Start()` call that takes the body — so capture a
handle that is filled in after `Start()` returns:

```cpp
// members of the owning object:
//   TSharedPtr<FSCFlowLoop> LoadLoop;      // the handle, outlives every callback
//   TArray<FSoftObjectPath> Assets;
//   FStreamableManager      Streamable;

FSCFlowLoopParams Params;
Params.Count = Assets.Num();

TWeakObjectPtr<ThisClass> WeakThis(this);
LoadLoop = FSCFlowLoop::Start(GetWorld(), Params,
	[WeakThis](int32 Index)
	{
		ThisClass* Self = WeakThis.Get();
		if (Self == nullptr) { return ESCFlowStep::Break; }

		Self->Streamable.RequestAsyncLoad(Self->Assets[Index], [WeakThis]()
		{
			if (ThisClass* Owner = WeakThis.Get(); Owner && Owner->LoadLoop.IsValid())
			{
				Owner->LoadLoop->Continue();
			}
		});
		return ESCFlowStep::Suspend;
	},
	[WeakThis](ESCFlowFinish)
	{
		if (ThisClass* Self = WeakThis.Get()) { Self->LoadLoop.Reset(); }
	});
```

`Continue()` never runs the next iteration synchronously — it lands on a later tick — so calling
it from inside a completion callback is safe with respect to re-entrancy. On the last index it
does finish the loop synchronously: `Finished` runs inside `Continue()` and resets `LoadLoop`
while `Continue()` is still on the stack. That is safe, because the core keeps itself alive
until the call returns.

`Finished` can also run inside `Start()` — with no world, or with `Assets` empty, so `Count` is
`0`. It then resets `LoadLoop` *before* `Start()` returns, and the assignment stores a core that
has already finished. Nothing breaks, since `Continue()` on a finished core does nothing, but
`LoadLoop.IsValid()` no longer means "still loading" — ask `LoadLoop->IsRunning()` instead.

If the completion callback never fires, the loop stays parked and [after 30 seconds logs the
suspension warning](../concepts/flow-family.md#warnings-instead-of-silence), naming `Continue()`.
`Break()` from anywhere ends it.

## The two callback constraints

The loop body and the condition defer a `Break()` or `Cancel()` raised inside them; the duration
update does not:

| | `FSCFlowLoopBody` / `FSCFlowCondition` | `FSCFlowDurationUpdate` |
| --- | --- | --- |
| A `Break()` / `Cancel()` from inside | **deferred** until the callback returns | **immediate** — the callback is destroyed while running |
| Safe after the cancelling statement | yes | **no** — reading a capture is a use-after-free |
| World teardown from inside | not deferred, in either | not deferred |

If you cancel from inside a duration `Update`, make `Cancel()` the last statement that touches a
capture. Nothing catches a violation for you.

No callback may synchronously tear down the world the task runs on.

## Using a node from C++

If you want the node's delegates but are in C++, call the factory and `Activate()`:

```cpp
USCFlowForEachIndexWithDelay* Node = USCFlowForEachIndexWithDelay::ForEachIndexWithDelay(
	this, /*Count*/ 3, /*Delay*/ 0.2f, /*bDelayBeforeFirstIteration*/ false, /*bAutoContinue*/ true);
Node->LoopBody.AddDynamic(this, &AMyActor::HandleIndex);     // UFUNCTION() void HandleIndex(int32 Index)
Node->Completed.AddDynamic(this, &AMyActor::HandleDone);     // UFUNCTION() void HandleDone(bool bCompletedFully)
Node->Activate();
```

Bind the delegates *before* `Activate()`: the loop starts in `Activate()` so that a `Count` of `0`
still reaches a `Completed` handler bound after the factory returned.

The factory registers the node with the Game Instance, which keeps the node alive until it
finishes. With no Game Instance — a bare world — that registration silently does nothing.
Hold the node in a `UPROPERTY` or a `TStrongObjectPtr` there, and anywhere you intend to call
`Break()` on it later.

---

← [Advanced](README.md) · [Documentation index](../../README.md#documentation)
