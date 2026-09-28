# Do For Duration

***Do For Duration** — the alpha drive, the curve, the final-frame guarantee, `Cancel`.*

## `USCFlowDoForDuration`

*Available in: C++ and Blueprint.* `UBlueprintAsyncActionBase`, `BlueprintType`, proxy pin
`Action`. Module `StillCookingCore`, header `Flow/SCFlowDoForDuration.h`. Category
`StillCooking|Flow`. Built on `FSCFlowDuration`. **[Placeable on an event graph
only](../concepts/flow-family.md#where-a-flow-node-can-go)**, like every Flow node.

Drives an alpha from `0` to `1` over `Duration` seconds, firing `On Update` every frame and then
`Completed`. Unlike a hand-written `Alpha += DeltaTime / Duration` loop, it delivers the final
update at alpha `1` instead of skipping it when the last frame overshoots.

### `DoForDuration(...)`

- **C++ call** — `USCFlowDoForDuration::DoForDuration(WorldContextObject, Duration, Curve, bUseUnscaledTime, bTickWhenPaused)`, then `Activate()`
- **Blueprint node** — `Do For Duration`

| Parameter | Pin | Default | Meaning |
| --- | --- | --- | --- |
| `WorldContextObject` | hidden | — | filled in by `self` |
| `Duration` | `Duration` | `1.0` | seconds from alpha `0` to alpha `1`; `0` or less delivers a single update at alpha `1` and completes |
| `Curve` | `Curve` | none | a `Curve Float` asset that shapes the alpha; see below |
| `bUseUnscaledTime` | `Use Unscaled Time` | `false` | advanced; integrate real time instead of the world's dilated time |
| `bTickWhenPaused` | `Tick When Paused` | `false` | advanced; keep driving while the game is paused |

[The first `On Update` lands on the first tick after
activation](../concepts/flow-family.md#two-timing-rules), one delta in — the alpha starts at
`DeltaTime / Duration`, not at `0`. A paused drive without `Tick When Paused` banks no time: it
freezes, and resumes one update per frame on unpause. The two advanced pins are independent —
[what each one decides](../concepts/flow-family.md#two-clocks).

## The final-frame guarantee

The last `On Update` is guaranteed to land at the end of the drive: with no curve, at an alpha
of exactly `1`. The update goes out *before* the completion test, so the frame that reaches or
overshoots the duration still delivers, at raw progress clamped to `1`.

The guarantee attaches to **raw progress**, not to the delivered alpha. With a curve set, the
last update carries the curve's value at time `1` — whatever its author drew there. A curve
ending at `0.5` delivers `0.5` last; a curve that overshoots past `1` and settles back delivers
its overshoot on the way and its final value at the end. The curve's output is not clamped:
clamping it would silently flatten an overshoot its author drew on purpose.

## The `Curve` pin

The drive's raw `0..1` progress is looked up on the curve and the result is what `On Update`
receives. The node holds the asset and consults it every frame while the asset is valid; a node
collected before its drive ends degrades the lookup to the identity rather than reading a dead
asset.

A curve built in code works the same as an asset, with the delegate binding and `Activate()`
that any node driven from C++ needs:

```cpp
TStrongObjectPtr<UCurveFloat> Curve(NewObject<UCurveFloat>());
Curve->FloatCurve.AddKey(0.f, 0.f);
Curve->FloatCurve.AddKey(1.f, 0.5f);

USCFlowDoForDuration* Node = USCFlowDoForDuration::DoForDuration(World, /*Duration*/ 1.f, Curve.Get());
Node->OnUpdate.AddDynamic(Listener, &UMyListener::HandleUpdate);       // UFUNCTION() void HandleUpdate(float Alpha, float DeltaTime)
Node->Completed.AddDynamic(Listener, &UMyListener::HandleCompleted);   // UFUNCTION() void HandleCompleted(bool bCompletedFully)
Node->Activate();
// the last delivered alpha is 0.5 — the curve's value at one — not 1
```

## Functions

### `Cancel()`

*Available in: C++ and Blueprint.*

- **C++ call** — `Node->Cancel()`
- **Blueprint node** — `Cancel`, on the `Action` pin

Ends the drive early and fires `Completed` — the same pin as a full run; in Blueprint
[nothing tells the two apart](../concepts/flow-family.md#completed-without-a-reason).
`Completed` fires before `Cancel` returns, from `On Update` too. A second `Cancel` does
nothing.

## Outputs

| Pin / delegate | Type | When |
| --- | --- | --- |
| `On Update` (`OnUpdate`) | `FSCFlowAlphaSignature(float Alpha, float DeltaTime)` | every frame the drive advances; `DeltaTime` is the delta the drive integrated, on the clock it chose |
| `Completed` (`Completed`) | `FSCFlowCompletedSignature(bool bCompletedFully)` | once, when the drive ends; `bCompletedFully` is `false` when `Cancel` stopped it — C++ only, the Blueprint node has no pin for it |
| `Action` | `USCFlowDoForDuration*` | the node itself, for `Cancel` — [the proxy pin every Flow node carries](../concepts/flow-family.md#the-proxy-pin) |

[`Completed` does **not** fire when the world is torn down under the
drive](../concepts/flow-family.md#how-a-task-ends).

In Blueprint the node's data pins are `Alpha` and `Delta Time`, from `On Update`. On the
`Completed` path they still hold the last values delivered —
[why `Completed` carries no reason](../concepts/flow-family.md#completed-without-a-reason).

## Messages

One, on `LogStillCooking`: an `Error` when the world context resolved no world, after which nothing
fires. Its exact wording is with
[the message every Flow node can write](../troubleshooting/diagnostics.md#flow-all-nodes).

## Pitfalls

- **Read `Alpha` as the value to apply, not as a fraction of time.** With a curve, `Alpha` is
  the curve's output and can exceed `1`.
- **[A second `Activate()` on the same node is
  ignored](../concepts/flow-family.md#two-timing-rules).** Call `Do For Duration` again for a new
  drive.
- **Cancelling from inside the update callback.** On the C++ core, a `Cancel()` issued from
  inside the update lambda — directly, or through the node's `Cancel` called from `On Update` —
  destroys that lambda while it is still on the stack. A `Cancel` from inside `On Update` works
  today only because the node's own handler reads nothing after that point; a future change
  could break it. In C++, make the cancelling statement the last one
  that touches a capture. Details in [C++ Flow cores](cpp-flow-cores.md#fscflowduration).

---

← [Reference](README.md) · [Documentation index](../../README.md#documentation)
