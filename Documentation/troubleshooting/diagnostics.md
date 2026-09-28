# Diagnostics

*Every message the plugin can log, what triggers it, and the procedures for a leaked tickable, a silent command and a wait that never ends.*

## The log category

Everything the plugin says goes to **`LogStillCooking`**. In the Output Log, filter on that
category. To see the `Verbose` lines — the console registry's per-command bookkeeping — raise
the verbosity from the console:

```
Log LogStillCooking Verbose
```

Two messages are `ensure`s rather than log lines: they show up as ensure reports with a call
stack, and they name the object.

## Every message

`<ObjectName>` is the tickable object's name; `<Name>` a console command name; `<Id>` a static
command identifier; `<ClassName>` the registry class without its `U` prefix. None of these
messages appears in a Shipping build, because the engine
[compiles logging and `ensure` out of it by default](../concepts/builds-and-shipping.md#what-is-gone-in-shipping).

### `USCTickableObject`

| Message | Kind | Trigger |
| --- | --- | --- |
| `<ObjectName>: Initialize called twice.` | ensure | `Initialize()` on an initialized object; the call does nothing |
| `<ObjectName>: Initialize found no world on the outer chain.` | Error | the outer chain resolves no `UWorld`; the object stays uninitialized |
| `<ObjectName>: destroyed while still initialized - the owner never called Shutdown().` | ensure, per instance | garbage-collected before `Shutdown()` |

### `USCConsoleCommandRegistrySubsystem`

| Message | Level | Trigger |
| --- | --- | --- |
| `Console command registry initialized (<ClassName>).` | Log | subsystem start; `<ClassName>` is the class that won — your Blueprint child, or the C++ base |
| `RegisterConsoleCommand: rejected '<Name>' - empty name or unbound callback.` | Warning | as stated |
| `RegisterConsoleCommand: '<Name>' already exists, skipped.` | Warning | name taken anywhere in the console manager |
| `RegisterConsoleCommand: '<Name>' was refused by the console manager.` | Warning | console manager returned nothing |
| `RegisterConsoleCommand: '<Name>' registered.` | Verbose | success |
| `UnregisterConsoleCommand: '<Name>' removed.` | Verbose | success |
| `PruneStaleCommands: '<Name>' removed - its owner is gone.` | Verbose | a registration swept a dead owner's command |
| `SC.<Id>: no game world - a statically registered command is visible from editor start, but its body lives in a Blueprint that exists only once the game or a PIE session runs.` | Warning | a static command run outside Play |

### Flow (all nodes)

| Message | Level | Trigger |
| --- | --- | --- |
| `A StillCooking\|Flow task was started without a world; it is aborted.` | Error | the world context resolved no world; the node broadcasts nothing |

On the two loop nodes the engine reports the same case first. They look the world up with the
engine's logging mode, so the engine writes its own `Warning` — not on `LogStillCooking` — just
before the `Error` above. `<ObjectPath>` is the world context object's path name. **Do For
Duration** and **Wait Until** look the world up silently and log only the `Error`.

| Engine message | Level | Trigger |
| --- | --- | --- |
| `A null object was passed as a world context object to UEngine::GetWorldFromContextObject().` | Warning | the world context object is null, or was destroyed before activation |
| `No world was found for object (<ObjectPath>) passed in to UEngine::GetWorldFromContextObject().` | Warning | the object exists but belongs to no world |

### The loop nodes

| Message | Level | Trigger |
| --- | --- | --- |
| `FSCFlowLoop has been suspended at index <i> of <n> for <s> seconds with no Continue() (Blueprint: 'Continue Loop'). The loop is waiting for that call, not stuck, and resumes the moment it arrives. Adjust SC.Flow.SuspendedLoopWarningSeconds, or set it to 0, if a body this long is expected here.` | Warning, once per loop | parked longer than `SC.Flow.SuspendedLoopWarningSeconds` (default 30) |

### `Wait Until`

| Message | Level | Trigger |
| --- | --- | --- |
| `Wait Until has no callable Condition - the pin was never bound, or the object it was bound to has been destroyed. A condition that can never answer has run out of time at zero, so the node takes On Timed Out. Bind Condition through Create Event to a Blueprint FUNCTION of the shape (int32 Attempt) -> bool; a custom event cannot return a value and will fail to compile.` | Error | unbound at activation, or lost mid-wait |
| `FSCFlowConditionPoll has polled <n> times over <s> seconds without its condition returning true, and has no deadline to end it (Blueprint: the 'Timeout' pin is 0 or less). It is waiting, not stuck. Set a Timeout, or adjust SC.Flow.WaitUntilWarningSeconds - set it to 0 - if a wait this long is expected here.` | Warning, once per node | no deadline, elapsed past `SC.Flow.WaitUntilWarningSeconds` (default 30) |

`Do For Duration` and `Print String Formatted` log nothing of their own.

## Procedure: a leaked tickable object

The ensure names the object — `<ObjectName>: destroyed while still initialized …` — and the name is
the `NewObject` name, which for an unnamed object is the class name plus a number. Two ways to
find the owner:

1. **From the name**, if you named the object on creation. Search the project for that name.
2. **From the timing.** The ensure fires during garbage collection, so it lands some time after
   the reference was dropped. If it fires on every actor move in the editor, the owner is a
   Construction Script. If it fires mid-game, an owner reassigns its `TObjectPtr` without
   shutting the old object down first, or drops its only reference to an object whose world is
   still alive.

[`BeginDestroy` does not run `Shutdown()` for you — it cannot, on that
path](../concepts/lifecycle.md#when-you-forget-shutdown) — so each leak is one ensure and no
teardown.

## Procedure: proving a console command reaches the registry

A command that "does nothing" has four places it can stop. Check them in this order:

1. **Is the name there at all?** Type its prefix in the console. Not listed: for a dynamic
   command, `Register Console Command` returned `false` (see the `Warning`); for a static one, the
   stub's module is not loaded, or
   [registration is compiled out of Shipping](../concepts/builds-and-shipping.md#what-is-gone-in-shipping).
2. **Is there a game world?** A static command is
   [visible from editor start and executable only with a game world](../concepts/builds-and-shipping.md#visible-is-not-executable);
   outside Play it logs the `no game world` warning and stops. Start Play.
3. **Which registry class is live?** Find `Console command registry initialized (<ClassName>)` in
   the startup log (it shows the class name without its `U` prefix). If `<ClassName>` is
   `SCConsoleCommandRegistrySubsystem`, no subclass was found — your Blueprint child does not
   exist, or, in a cooked build, was not loaded in time.
   [A second Blueprint child would show up as its own name, with two registries
   live](../reference/console-commands.md#one-blueprint-child-per-project).
4. **Does the body handle this identifier?** For a static command, `On Static Command` is one
   event for all of them; a `Switch on Name` with no case for the `CommandId` is silent by
   design. For a dynamic command, the bound custom event is the body — put a `Print String` in it.

At `Verbose`, every registration and removal is logged, which tells you whether a name was
registered and then removed (by `Unregister Commands For`, or by a prune after the owner died).

## Procedure: a Flow task that seems stuck

1. **Wait 30 seconds.** [Both waiting states — a suspended loop, a deadline-less poll — log a
   `Warning` after that long](../concepts/flow-family.md#warnings-instead-of-silence). The message
   says which task, where it is, and which call or pin ends the wait. If you lowered the
   corresponding console variable, the warning comes sooner; if you set it to `0`, it never comes.
2. **No warning after 30 seconds** and nothing fires: the task is not waiting, it is *gone*.
   Look for `was started without a world` (nothing ever started), or for a world teardown —
   [nothing is broadcast on teardown](../concepts/flow-family.md#how-a-task-ends). In C++ without
   a Game Instance, check that the node was kept alive — a node held by nothing is collected, and
   its core winds up with it.
3. **The warning names a loop you did not expect to be latent:** `Auto Continue` is cleared on
   a synchronous body. Check the advanced pins.

---

← [Troubleshooting](README.md) · [Documentation index](../../README.md#documentation)
