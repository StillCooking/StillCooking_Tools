# Limits

*What the plugin does not do by design, what disappears in Shipping, and the engine versions and platforms it has not been built for.*

## Plugin-wide

- **A C++ project is required.** The plugin is
  [compiled with your project](../start/installation.md).
- **UE 5.7 and 5.8 on Win64 are the only built-and-tested combinations**, and 5.5 and 5.6 do not
  compile — see [what has been built and tested, release by release](../project/compatibility.md).
- **Nothing replicates.** No entry knows about the network. A loop, a drive or a wait runs
  where it was started and nowhere else.
- **The C++ Flow layer is EXPERIMENTAL** and
  [may change shape without a major version bump](../concepts/overview.md#the-stable-surface-and-the-experimental-layer).
- **Game Thread only.** Every entry is driven by the world's tick pass or by the console.
  Nothing in the plugin is thread-safe; calls from other threads are unsupported.
- **The four Flow nodes are [placeable on an event graph
  only](../concepts/flow-family.md#where-a-flow-node-can-go)**, never inside a function graph.
- **No content ships.** The descriptor allows content; the distribution carries none.

## Per entry

**`USCTickableObject`**

- [Does not root itself; the owner holds it](../concepts/lifecycle.md#the-contract).
- [Does not run subclass teardown on garbage
  collection](../concepts/lifecycle.md#when-you-forget-shutdown); [only `Shutdown()`
  does](../concepts/lifecycle.md#the-contract). A leak is reported, not repaired.
- [Cannot be hosted from a Construction Script](../concepts/lifecycle.md#when-you-forget-shutdown).
- No catch-up on a throttled interval: a long hitch yields one `Tick`, not several.

**`USCConsoleCommandRegistrySubsystem`**

- Commands [execute only while a game world exists](../concepts/builds-and-shipping.md#visible-is-not-executable) — never in
  the editor outside Play. A command that should act on the editor itself needs an editor
  subsystem, which this is not.
- [One Blueprint child per project, and no C++ subclass in an always-loaded
  module](../reference/console-commands.md#one-blueprint-child-per-project).
- Unregister a command after it has returned; the console manager makes no promise that
  unregistering from inside a command's own execution survives it.
- An empty `Owner` is rejected, not treated as "everything".

**The loop nodes**

- [There is no delay after the last iteration; `Completed` follows it
  immediately](../reference/for-each-index.md#for-each-index-with-delay).
- [`Items Per Tick` has no effect with `Auto Continue`
  cleared](../reference/for-each-index.md#for-each-index-per-tick).
- `Delay` together with `Items Per Tick` is C++-only.
- [No pause options: a paused game runs no iterations](../concepts/flow-family.md#two-clocks).

**`Do For Duration`**

- [One drive per node object; a second `Activate()` is
  ignored](../concepts/flow-family.md#two-timing-rules).
- [The alpha is not clamped when a curve
  overshoots](../reference/do-for-duration.md#the-final-frame-guarantee).
- Cancelling from inside the update: on the C++ core, nothing may touch a capture after the
  cancelling statement. The node's `Cancel` from `On Update` works today because the node's
  handler touches nothing after it — a property of the current code, not a guarantee.

**`Wait Until`**

- [`Condition` must be a Blueprint function; an event fails to
  compile](../reference/wait-until.md#binding-condition).
- [`Poll Interval` at or above `Timeout` never polls on a
  tick](../reference/wait-until.md#the-deadline-and-the-poll); with `Check Immediately` the
  condition is asked once, at activation, and then the node times out.
- [An unbound or lost condition ends through `On Timed
  Out`](../reference/wait-until.md#outputs).

**`Print String Formatted`**

- [Numbers are culture-formatted](../reference/print-string-formatted.md#numbers-and-culture); the
  grouping is unstable and will change.
- [Case variants of an argument name share one
  pin](../reference/print-string-formatted.md#format-and-argument-pins).
- [There is no C++ side to this node and nothing to
  call](../reference/print-string-formatted.md#no-c-side).

## What disappears in Shipping and Test

| Entry | Shipping | Test |
| --- | --- | --- |
| `Print String Formatted` | compiled out | compiled out |
| Console command registration, `SC.Debug.Ping` | compiled out | present |
| Everything else | present | present |

Why each entry goes, and what still exists of it in the build that drops it:
[the print node is dropped by the Blueprint compiler, the registry by a preprocessor guard](../concepts/builds-and-shipping.md#what-is-gone-in-shipping).

One more thing goes from Shipping, and the plugin does not decide it: the engine, by default,
[compiles logging and `ensure` out of it](../concepts/builds-and-shipping.md#what-is-gone-in-shipping), so every message in
[Diagnostics](../troubleshooting/diagnostics.md) is absent there too.

## Not verified

- **Platforms other than Win64, and engine versions other than UE 5.5–5.8** — the dependency list
  suggests they would work; none has been built. UE 5.5 and 5.6 have been built and
  [do not compile](../project/compatibility.md#known-not-to-build), so they are not on this list.

A report confirming or contradicting this is useful: [Issues](../project/issues.md).

---

← [Advanced](README.md) · [Documentation index](../../README.md#documentation)
