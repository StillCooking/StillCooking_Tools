# Common problems

*From symptom to cause: a loop that stops after one iteration, an ensure when an actor is moved, a console command that does nothing, a node that will not compile, pins that come back as wildcards.*

## A loop runs one iteration and then nothing happens

**`Auto Continue` is cleared and the body never calls `Continue Loop`.** The loop is waiting,
not stuck: [after 30 seconds it logs a `Warning` on
`LogStillCooking`](../concepts/flow-family.md#warnings-instead-of-silence) that says so and names
the call. Either wire `Continue Loop` (on the node's `Loop` pin) at the end of the latent body, or
check `Auto Continue` if the body is synchronous.

If the body is synchronous and `Auto Continue` is checked, the loop cannot park — look for a
`Break Loop` fired from the body at index `0`, and check `Count`.

## `Completed` never fires

In order of likelihood:

1. **The loop is suspended** — see above.
2. **The world was torn down under the task.** [`Completed` is deliberately not broadcast on
   teardown](../concepts/flow-family.md#how-a-task-ends).
3. **The node was started with no world.** The log has
   `A StillCooking|Flow task was started without a world; it is aborted.` and nothing fires —
   [the one message every Flow node can write](diagnostics.md#flow-all-nodes). On a loop node
   it comes right after the engine's own warning about the world context object. In C++, pass a
   world context that resolves to a world.
4. **`Count` was `0` and the delegate was bound after `Activate()`** in C++. `Completed` fires
   during activation in that case; bind first.

## `Wait Until` never fires anything

**`Timeout` is `0`** — no deadline — and the condition never becomes true. After 30 seconds
the log carries a `Warning` starting `FSCFlowConditionPoll has polled …` —
[the warning for a wait with no deadline, in full](diagnostics.md#wait-until). Set a `Timeout`, or
fix the condition.

**`On Timed Out` fired at once, with an `Error` in the log** starting `Wait Until has no
callable Condition` — [the error for a condition that can never answer](diagnostics.md#wait-until):
[the `Condition` pin is not bound, or the object owning the bound function was
destroyed](../reference/wait-until.md#outputs). Bind through **Create Event** to a Blueprint
*function*, on an object that outlives the wait.

**`On Timed Out` fired without the condition being asked on any tick**: [`Poll Interval` is at or
above `Timeout`](../reference/wait-until.md#the-deadline-and-the-poll). Lower the interval or
raise the timeout.

## An `ensure` fires every time an actor is moved in the editor

[The ensure that names the object and the `Shutdown()` that never came](diagnostics.md#usctickableobject):

```
<ObjectName>: destroyed while still initialized - the owner never called Shutdown().
```

The tickable object is created from the **Construction Script**, which
[is mutually exclusive with the ownership contract](../concepts/lifecycle.md#when-you-forget-shutdown).
Create the object in `BeginPlay` and shut it down in `EndPlay`.

The same ensure mid-game means an owner dropped or reassigned its reference without calling
`Shutdown()` first; the message names the object. (A missing `Shutdown()` in `EndPlay` alone
does not produce it at the end of a Play session — the object shuts itself down when its world
is cleaned up.)

## The tickable object does not tick

1. **Was `Initialize()` called?** Ticking starts there, not in the constructor. `IsInitialized()`
   tells you.
2. **Did `Initialize()` log `Initialize found no world on the outer chain.`?**
   [That `Error`, and the two other messages this class writes](diagnostics.md#usctickableobject).
   The outer passed to `NewObject` / **Construct Object from Class** resolves no world. Use an actor, a component
   or the world as the outer, not the transient package.
3. **Is it an editor world?** Without `bTickInEditor` an initialized, enabled object stays
   silent outside Play.
4. **Was `DisableTick()` called before `Initialize()`?** That is honoured — [an explicit call
   outranks `bStartTickingOnInitialize`](../reference/tickable-object.md#disabletick).
5. **Is the game paused** and `bTickWhenPaused` off?

## The object keeps ticking while PIE is paused

`bTickInEditor` is `true`, and [the editor branch it turns on bypasses the pause rule for the
whole editor process, PIE included](../reference/tickable-object.md#the-two-engine-gates). Clear
`bTickInEditor` if you need pause handling; in the editor you cannot have both. A packaged build
does not have this behaviour.

## A console command is in autocomplete but does nothing

**Outside Play:** the name is there and the body is not — a static command is
[visible from editor start and executable only with a game world](../concepts/builds-and-shipping.md#visible-is-not-executable).
[The log says so](diagnostics.md#uscconsolecommandregistrysubsystem):

```
SC.<Id>: no game world - a statically registered command is visible from editor start, but its body lives in a Blueprint that exists only once the game or a PIE session runs.
```

Start Play and run it again.

**During Play:** there is no Blueprint child of the registry implementing `On Static Command`,
or its switch has no case for this `CommandId`. Check the registry's startup line in the log —
`Console command registry initialized (<ClassName>)` — to see whether your Blueprint child is the
class that runs. If the class name is the C++ one, no subclass was found: the child does not
exist, or, in a cooked build, it was not loaded when the subsystem collection initialized.

## `Register Console Command` returns `false`

In the log, a `Warning` names the reason — these three, and
[every other message the registry can write](diagnostics.md#uscconsolecommandregistrysubsystem):

| Message | Cause → fix |
| --- | --- |
| `rejected '<Name>' - empty name or unbound callback.` | no name, or nothing bound → give it a name; bind `Callback` to a custom event with an `Args` pin |
| `'<Name>' already exists, skipped.` | another owner has the name — a second actor of the same class, or a previous session's command that was never unregistered; the first registration stays → choose a unique name, or unregister the old one |
| `'<Name>' was refused by the console manager.` | the console manager rejected the name → choose a different name |

No message at all and `false`: the build is Shipping, where
[the registry's registration is compiled out](../concepts/builds-and-shipping.md#what-is-gone-in-shipping).

**Registered from the Construction Script?** [It runs in the editor world, where there is no
registry, and silently does nothing outside PIE](../reference/console-commands.md#pitfalls). Move
it to `BeginPlay`.

## `unresolved external symbol` when building

Your module calls a `StillCookingCore` class and does not list the module. Add
`"StillCookingCore"` to [`PublicDependencyModuleNames` in your `Build.cs`](../start/installation.md).
Blueprint-only use never sees this error.

## A node is missing from the palette

- **All four Flow nodes** missing: the runtime module did not build or the plugin is disabled.
  [Verification](../start/verification.md) walks through it.
- **Only `Print String Formatted`** missing: the `StillCookingCoreEditor` module did not build.
  It is a separate module; check the build output for it.
- **All four Flow nodes missing, but only inside a function graph**: that is expected — they are
  [placeable on an event graph only](../concepts/flow-family.md#where-a-flow-node-can-go).

## `Print String Formatted` pins are wildcards after restarting the editor

Versions 0.6.0 and 0.7.0: on load, connected argument pins came back typed as wildcards and the
Blueprint would not compile until each was unplugged and plugged back in. Fixed in 0.7.1.
Update, reopen the asset; no resave is needed.

## `Create Event Signature Error` on the `Condition` pin

The compiler, not the plugin, writes this one —
[why `Condition` has to be a function](../reference/wait-until.md#binding-condition):

```
Create Event Signature Error: The function/event '<name>' does not match the necessary signature - has the delegate or function/event changed?
```

The pin is bound to a **custom event**. `Condition` returns a Boolean, and an event cannot. Create
a Blueprint **function** with input `Attempt` (Integer) and a Boolean output, and bind that.

## Behaviours that look like a fault

- **`For Each Index With Delay` runs index `0` immediately**, not after `Delay`. That is the
  default; `Delay Before First Iteration` changes it.
- **`Completed` follows the last iteration with no delay.** [The delay is *between*
  iterations](../reference/for-each-index.md#for-each-index-with-delay).
- **`Do For Duration`'s first `Alpha` is not `0`.** The first update is one delta in.
- **`Do For Duration` delivers an `Alpha` above `1`.** [The curve overshoots; that is
  honoured](../reference/do-for-duration.md#the-final-frame-guarantee).
- **`Items Per Tick` seems ignored.** [`Auto Continue` is
  cleared](../reference/for-each-index.md#for-each-index-per-tick).
- **`1234` prints as `1 234`** from `Print String Formatted`. [Numeric arguments are formatted by
  the current culture](../reference/print-string-formatted.md#numbers-and-culture).
- **`SC.Debug.Ping` does nothing** in a fresh project. There is no Blueprint child implementing
  `On Static Command`; reaching the registry is all the example promises.

---

← [Troubleshooting](README.md) · [Documentation index](../../README.md#documentation)
