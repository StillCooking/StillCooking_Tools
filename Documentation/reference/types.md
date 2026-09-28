# Types

*`ESCFlowStep`, `ESCFlowFinish`, the delegate signatures, the function aliases, the console variables and the log category.*

## Enums

Both are plain C++ `enum class`es in `Flow/SCFlowTypes.h`, not `UENUM`s: invisible to Blueprint
and not serialized, so no asset stores them. Both belong to the EXPERIMENTAL layer and so
[may change shape without a major version bump](../concepts/overview.md#the-stable-surface-and-the-experimental-layer).

### `ESCFlowStep`

What a loop body reports when it returns.

| Value | Meaning |
| --- | --- |
| `Continue` | the iteration is done; schedule the next one |
| `Suspend` | the iteration is still in flight; wait for `Continue()` or `Break()` |
| `Break` | end the loop after this iteration, as `Broken` |

A synchronous body returns `Continue` (or `Break`), never `Suspend`.

### `ESCFlowFinish`

How a Flow task ended — a loop, a duration drive, or a condition poll:

| Value | Meaning |
| --- | --- |
| `Completed` | count reached, duration elapsed, condition true |
| `Broken` | `Break()`, `Cancel()`, a loop body returning `ESCFlowStep::Break`, or [a **Wait Until** whose `Condition` became uncallable](wait-until.md#outputs) |
| `Aborted` | the world was torn down, or the task started without one |
| `TimedOut` | a condition poll's deadline elapsed |

[What a Blueprint node fires for each value, and why `Aborted` makes it fire
nothing](../concepts/flow-family.md#how-a-task-ends).

`TimedOut` was appended in 0.7.0, after `Aborted`, so no earlier value's ordinal moved.

## Dynamic delegates

Declared in `Flow/SCFlowDelegates.h` (Flow) and `Debug/SCConsoleCommandRegistrySubsystem.h`
(console). These are the types of the Blueprint pins; in C++ you bind them with `AddDynamic` /
`BindDynamic` / `BindUFunction` to a `UFUNCTION` on a `UObject`.

| Delegate | Signature | Used by |
| --- | --- | --- |
| `FSCFlowIndexSignature` | `(int32 Index)` | `Loop Body` on both loop nodes |
| `FSCFlowCompletedSignature` | `(bool bCompletedFully)` | `Completed` on the loops and on **Do For Duration**; in Blueprint [`bCompletedFully` reaches no pin](../concepts/flow-family.md#completed-without-a-reason) |
| `FSCFlowAlphaSignature` | `(float Alpha, float DeltaTime)` | `On Update` on **Do For Duration** |
| `FSCFlowConditionSignature` | `(int32 Attempt) -> bool` | `Condition` on **Wait Until**; single-cast, with a return value — [bind a Blueprint *function*, not an event](wait-until.md#binding-condition) |
| `FSCFlowSignalSignature` | `()` | `On Satisfied`, `On Timed Out`, `On Cancelled` on **Wait Until** |
| `FSCBlueprintConsoleCommand` | `(const TArray<FString>& Args)` | `Callback` on `Register Console Command`; `Args` are the raw console tokens |

A dynamic delegate holds a **weak** reference to its target. For the console registry that is
what makes a command whose owner was collected harmless; for **Wait Until** it is why a
condition can become uncallable mid-wait.

## Function aliases

`TFunction` aliases in `Flow/SCFlowTypes.h`, taken by the C++ cores. EXPERIMENTAL, like the enums
above.

| Alias | Signature | Contract |
| --- | --- | --- |
| `FSCFlowLoopBody` | `ESCFlowStep(int32 Index)` | Called once per iteration with the zero-based index. Must not synchronously tear down the world the loop runs on; everything else that can end the loop from inside is deferred until it returns. |
| `FSCFlowFinished` | `void(ESCFlowFinish)` | Called exactly once, when the task ends, for any reason. |
| `FSCFlowAlphaShaper` | `float(float RawAlpha)` | Shapes raw `0..1` progress into the alpha the update receives. Empty means identity. The result is never clamped. |
| `FSCFlowDurationUpdate` | `void(float Alpha, float DeltaTime)` | Called every tick of a drive. Nothing is deferred: a cancel from inside clears this function while it runs — reading a capture after the cancelling statement is a use-after-free. |
| `FSCFlowCondition` | `bool(int32 Attempt)` | Answers "are we there yet" for a zero-based attempt. Returning `true` ends the wait. A cancel from inside is deferred until the call returns. |

## Console variables and commands

| Name | Kind | Default | Help |
| --- | --- | --- | --- |
| `SC.Flow.SuspendedLoopWarningSeconds` | variable | `30` | Seconds a suspended StillCooking\|Flow loop may wait for Continue() before it logs one warning. Zero or less disables the warning. |
| `SC.Flow.WaitUntilWarningSeconds` | variable | `30` | Seconds a StillCooking\|Flow Wait Until with no Timeout may poll before it logs one warning. Zero or less disables the warning. |
| `SC.Debug.Ping` | command, `ECVF_Cheat`, not in Shipping | — | Raises the On Static Command event with the identifier 'Ping'. Usage: SC.Debug.Ping [Args...] |

The two variables exist in every build configuration. The command goes with
[the registry's registration, which Shipping compiles out](../concepts/builds-and-shipping.md#what-is-gone-in-shipping).

## Log category

**`LogStillCooking`**, declared in `SCLogChannels.h`. The one category for everything in the
plugin. Verbose is where the console registry reports every registration and removal —
[how to filter the log and raise its verbosity](../troubleshooting/diagnostics.md#the-log-category).

---

← [Reference](README.md) · [Documentation index](../../README.md#documentation)
