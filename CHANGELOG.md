# Changelog

All notable changes to this plugin are recorded here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions follow
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Anything in a `Public/` header is treated as a contract: renaming or removing a public symbol,
or changing a `UFUNCTION` signature, takes a major version bump and a changelog entry.
The exception is the EXPERIMENTAL C++ layer — see
[Documentation/project/updates.md](Documentation/project/updates.md#what-the-version-number-means).

## [Unreleased]

## [0.8.0] - 2026-09-28

### Added

- Unreal Engine 5.8 — built for the editor and for Development and Shipping game targets, and the
  full automated suite passes on Win64. It joins 5.7 in the
  [compatibility table](Documentation/project/compatibility.md).
- Documentation — graph images: the quick start shows the finished first graph
  (`Documentation/start/quick-start-graph.png`), and **For Each Index Per Tick** and the
  thousand-item scan recipe show the scan graph (`Documentation/reference/per-tick-scan-graph.png`),
  the fade-in recipe shows its graph (`Documentation/advanced/fade-widget-graph.png`), and
  **Wait Until** and the boss recipe show the condition function and the node bound to it
  (`Documentation/reference/wait-until-condition-graph.png`,
  `Documentation/reference/wait-until-node-graph.png`), and **Print String Formatted** shows its
  argument pins (`Documentation/reference/print-string-formatted-graph.png`).

### Changed

- Documentation — the `Documentation/README.md` index is gone: it repeated this repository's
  `README.md` almost line for line. The README's **Documentation** section is now the index every
  page links back to, and the README links to the product page.
- Documentation — the glossary moved to its own page, `Documentation/concepts/glossary.md`, and
  each term links to the page that explains it.

### Fixed

- Documentation — Unreal Engine 5.5 and 5.6 are listed as known not to compile, instead of as
  not verified: both were built, and both stop on engine API that first appeared in 5.7.
- `For Each Index With Delay`, `For Each Index Per Tick` and `Do For Duration` — the tooltips of the
  `Completed` pin and of `Break Loop` / `Cancel` no longer describe a `false` value the Blueprint
  node has no pin for. They now say that a full run and a stopped one leave through the same pin,
  and that only a C++ handler receives `bCompletedFully`.
- Documentation — the recipes page says which recipes were built in the editor.
- Documentation — the loop nodes and `Do For Duration` have no `Completed Fully` output pin in
  Blueprint: an async node takes its data pins from its first delegate only. The quick start no
  longer asks for that pin, and the Flow concepts page explains how to tell a full run from a
  stopped one.
- Documentation — `bStartTickingOnInitialize` is copied into the tick intent when the object is
  created and again by `Shutdown()`, not read by `Initialize()`.
- Documentation — the loop nodes log the engine's own world-context warning before the plugin's
  `Error` when they run without a world; both engine messages are now listed.
- Documentation — `FSCFlowDurationUpdate` takes `(float Alpha, float DeltaTime)`.
- Documentation — the C++ cores can call `Finished` inside `Start()` (no world, or a loop with
  `Count` of `0` or less), and `IsRunning()` already returns `false` inside `Finished`.

## [0.7.1] - 2026-09-11

### Fixed

- `Print String Formatted` — argument pins came back as **wildcards while still connected** after
  an editor restart, so the Blueprint refused to compile. The node now restores each argument
  pin's type from its connection after a reconstruction; reopening an existing asset is enough.

## [0.7.0] - 2026-08-25

### Added

- `Wait Until` — Blueprint node that polls a `Condition` every `Poll Interval` seconds until it
  returns true, then fires `On Satisfied`; `On Timed Out` and `On Cancelled` are the other two
  endings, and a `Timeout` of `0` or less waits without a deadline. `Condition` must be bound
  through **Create Event** to a Blueprint *function* of the shape `(int32 Attempt) -> bool`, which
  also makes the node event-graph only.
- `SC.Flow.WaitUntilWarningSeconds` — console variable for the warning a deadline-less wait logs,
  so a forgotten wait is never silent. Default 30 seconds; 0 or less disables it.
- `FSCFlowConditionPoll` and `FSCFlowConditionSignature` — the C++ core under the node, and the
  delegate type that lets a Blueprint function answer a question asked from C++. **EXPERIMENTAL**,
  like the rest of the `StillCooking|Flow` C++ layer.
- `ESCFlowFinish::TimedOut` — appended to the existing enum, so no other value's ordinal moved.
  Plain C++, not a `UENUM`: invisible to Blueprint and in no asset.

## [0.6.0] - 2026-08-25

### Added

- `Print String Formatted` — a `Print String` whose argument pins come from the string itself:
  typing `{Index}` into `Format` grows an input pin called `Index`, which accepts the same
  argument types as the engine's `Format Text`. Marked **Development Only** like the engine's
  `Print String`, so it is compiled out of Shipping and Test builds.

  **Numbers are formatted by the current culture**, so `1234` prints as `1 234` under a Polish
  culture and `1,234` under an English one. A future version will move to culture-independent
  formatting, which **will change this output** — do not parse the grouping.

- `StillCookingCoreEditor` — a new module in the descriptor, of type `UncookedOnly`, hosting the
  graph node above. It is absent from a packaged game and requires no action from you beyond
  rebuilding.

## [0.5.0] - 2026-08-19

### Added

- `For Each Index Per Tick` — Blueprint node that runs a loop body once per index, spread across
  frames rather than waiting on a clock, at `Items Per Tick` (default 1) iterations per frame.
  Same contract as `For Each Index With Delay`; with `Auto Continue` cleared the body is latent,
  and `Items Per Tick` then has no effect.
- `FSCFlowLoopParams::ItemsPerTick` — the batch size on the **EXPERIMENTAL** C++ loop core.
  Defaults to 1, so existing C++ callers are unaffected.

## [0.4.0] - 2026-08-19

### Added

- `Do For Duration` — Blueprint node that drives an alpha from 0 to 1 over `Duration` seconds,
  firing `On Update` every frame with the alpha and the delta it integrated, then `Completed`.
  The last update lands at the end of the drive: an alpha of exactly `1.0` with no curve, or the
  `Curve`'s value at its `t = 1` key when one is wired in. `Use Unscaled Time`, `Tick When Paused`
  (advanced) and `Cancel` round it out; this node is the stable public surface.
- `FSCFlowDuration` and `FSCFlowTickingTask` — the C++ core the node above is built on.
  **EXPERIMENTAL**: its shape may change without a major version bump.

### Changed

- **EXPERIMENTAL, source-breaking for C++ consumers of the Flow layer**: `FSCFlowLoopFinished`
  renamed to `FSCFlowFinished`, now that a second core shares it. Blueprint graphs are unaffected —
  this is a C++ type rename only.

## [0.3.1] - 2026-08-18

### Added

- A suspended `For Each Index With Delay` now says so: waiting for a `Continue Loop` that never
  arrives logs one warning to `LogStillCooking` after 30 seconds, naming the index it is parked on
  and the call it is waiting for. The loop is still waiting, not stopped, and resumes the moment
  the call arrives.
- `SC.Flow.SuspendedLoopWarningSeconds` — console variable for the threshold above. Raise it for a
  long-running body, or set it to `0` to switch the warning off.

## [0.3.0] - 2026-08-16

### Added

- `For Each Index With Delay` — Blueprint node that runs a body once per index from 0 to
  `Count - 1`, waiting `Delay` seconds between iterations. The advanced `Auto Continue` pin is set
  by default, so the loop moves on as soon as the body returns; clear it for a latent body and the
  loop waits for `Continue Loop` instead, while `Break Loop` ends the loop early in either mode
  with `bCompletedFully = false`. This node is the stable public surface.
- `FSCFlowLoop` — the C++ latent-loop core the node above is built on. **EXPERIMENTAL**: it may
  change without a major version bump.

## [0.2.0] - 2026-08-15

### Added

- `USCConsoleCommandRegistrySubsystem` — lets Blueprints define console commands on two paths.
  Dynamic: a Blueprint child registers them from the `On Register Commands` event, with no C++ per
  command. Static: a small C++ stub registers the name at module load and forwards to a
  `BlueprintImplementableEvent`, which puts the command in the editor's autocomplete before PIE
  starts. Commands carry `ECVF_Cheat` and every registration path is compiled out of Shipping.
- `FSCBlueprintConsoleCommand` — the delegate a dynamic command binds its body to.
- `SC.Debug.Ping` — worked example of the static path.
- `USCTickableObject::IsTickEnabled()` — whether the object is currently asking to tick,
  independent of whether it has been initialized.
- `USCConsoleCommandRegistrySubsystem::UnregisterCommandsFor()` — removes every command registered
  on behalf of one owner, so an actor that registers in `BeginPlay` can drop its commands in
  `EndPlay` without tracking names. Commands whose owner has been destroyed are swept out
  automatically on the next registration.

### Changed

- `USCTickableObject` — `EnableTick()` and `DisableTick()` called before `Initialize()` now record
  the intent, and `Initialize()` applies it; previously `DisableTick()` was silently overridden by
  `bStartTickingOnInitialize` and `EnableTick()` was rejected with a warning. `Shutdown()` restores
  the configured starting value, so an `Initialize`/`Shutdown`/`Initialize` cycle behaves the same
  way every time.
- `USCTickableObject` — the `FTickableGameObject` interface, `BeginDestroy` and
  `PostInitProperties` moved to `protected`. Behaviour is unchanged; C++ code calling
  `IsTickable()` and friends on this class directly no longer compiles.
- `USCTickableObject::TickInterval` is no longer `BlueprintReadOnly` — use `GetTickInterval()`.
- Doc comments on the public headers are shorter; the rationale they used to carry — engine
  references, rejected alternatives, the reasoning behind each contract — moved out of the headers.

## [0.1.0] - 2026-08-14

First release. Built and tested against Unreal Engine 5.7 on Win64.

### Added

- `USCTickableObject` — abstract base class for a `UObject` that ticks once per frame.
  Constructs with `ETickableTickType::Never` and starts ticking in `Initialize()`, so a
  half-built object is never exposed to the tick pass. Blueprint-extendable through the
  `Tick`, `Initialize` and `Shutdown` events, with tick interval throttling, pause and
  editor-world gates, and automatic shutdown on world teardown.
- `LogStillCooking` — umbrella log category for the plugin.
- `StillCookingCore` (Runtime) module.
