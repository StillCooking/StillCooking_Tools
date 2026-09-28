<p align="center">
  <img src="Resources/Icon128.png" width="128" alt="StillCooking_Tools">
</p>

# StillCooking_Tools

A grab-bag of small, reusable Unreal Engine utilities — base classes, helpers and common
operations that are useful across unrelated projects, kept in one place with one version instead
of being copied from project to project. It is deliberately not a framework:
every entry stands on its own, so you can take one class or one Blueprint node and ignore the
rest. Nothing here depends on anything outside the engine.

| | |
|---|---|
| **Version** | 0.8.0 (see [`CHANGELOG.md`](CHANGELOG.md)) |
| **Status** | Beta |
| **Engine** | Unreal Engine 5.7, 5.8 |
| **Platform** | Win64 |
| **Distribution** | C++ source on GitHub — [the project has to be a C++ project](Documentation/start/installation.md#requirements) |
| **Dependencies** | [No third-party code, and no other plugins](THIRD-PARTY.md). Engine modules only. |
| **License** | MIT |
| **Product page** | [StillCooking_Tools on stillcooking.dev](https://stillcooking.dev/en/products/stillcooking-tools/) |

## What's inside

| I need | Entry | Kind | What it does |
|---|---|---|---|
| A `UObject` that ticks every frame without being an Actor or a Component | `USCTickableObject` | C++ base class, Blueprintable | Ticking starts explicitly in `Initialize()`, not in the constructor. Tick interval throttling, pause and editor-world gates, automatic shutdown on world teardown. The class never roots itself: the owner holds it in a `UPROPERTY` and calls `Shutdown()` before dropping the reference — [the ownership contract](Documentation/concepts/lifecycle.md#the-contract). |
| A console command whose body is a Blueprint event | `USCConsoleCommandRegistrySubsystem` | Game Instance Subsystem, Blueprintable | Registered at runtime from [the project's one Blueprint child](Documentation/reference/console-commands.md#one-blueprint-child-per-project), or declared at module load by a few lines of C++ that forward to `On Static Command` — [visible in the editor's autocomplete before Play starts](Documentation/concepts/builds-and-shipping.md#visible-is-not-executable). |
| to run something N times with a pause between runs | **For Each Index With Delay** | Blueprint node | Runs a body once per index, waiting `Delay` seconds between iterations. A synchronous body needs no extra wiring; for a latent body, clear `Auto Continue` and call `Continue Loop` when the body is done. |
| to spread N iterations across frames instead of doing them all at once | **For Each Index Per Tick** | Blueprint node | Runs a body once per index, spread across frames — `Items Per Tick` iterations per frame, the next batch on the next frame. |
| An alpha driven from 0 to 1 over time, with a guaranteed final update | **Do For Duration** | Blueprint node | Drives an alpha from `0` to `1` over `Duration` seconds, firing `On Update` every frame. [The last update is guaranteed to land at the end of the drive: with no curve, at an alpha of exactly `1`](Documentation/reference/do-for-duration.md#the-final-frame-guarantee). An optional `Curve` shapes the alpha. |
| to wait until a condition becomes true, with a timeout | **Wait Until** | Blueprint node | Polls a `Condition` function on an interval until it returns true, then fires `On Satisfied`; gives up on `Timeout` with `On Timed Out`, or on `Cancel` with `On Cancelled`. |
| A `Print String` with `{Placeholders}` that become pins | **Print String Formatted** | Blueprint node, Development Only | Type `{Index}` into `Format` and the node grows an input pin called `Index`. |
| The same loop, drive or poll from C++ without a Blueprint node | `FSCFlowLoop`, `FSCFlowDuration`, `FSCFlowConditionPoll` | C++ cores — [marked EXPERIMENTAL: what that commits the plugin to](Documentation/concepts/overview.md#the-stable-surface-and-the-experimental-layer) | The classes the Flow nodes are built on, usable directly: no `UObject`, no Game Instance, and a body that is a `TFunction` with a return value. |

Everything the plugin says goes to one log category, `LogStillCooking` —
[how to filter the log and raise its verbosity](Documentation/troubleshooting/diagnostics.md#the-log-category).

## Installation

From the root of your project:

```
git clone https://github.com/StillCooking/StillCooking_Tools.git Plugins/StillCooking_Tools
```

Then regenerate the project files (right-click the `.uproject` → **Generate Visual Studio
project files**) and build in the **Development Editor** configuration.
[A plugin in `Plugins/` is enabled automatically; you do not need to list it in your
`.uproject`](Documentation/start/installation.md#clone-and-build).

The Blueprint nodes work with no changes to your `Build.cs`. To call the C++ classes from your
own module, [add the runtime module to your own module's
`Build.cs`](Documentation/start/installation.md#module-dependencies):

```csharp
PublicDependencyModuleNames.AddRange(new string[] { "StillCookingCore" });
```

[Full instructions and a build check](Documentation/start/installation.md).

## Modules

| Module | Type | What it does |
|---|---|---|
| `StillCookingCore` | Runtime | Everything that runs in the game: the tickable base class, the console command registry, the Flow nodes and the C++ cores under them. Depends only on `Core`, `CoreUObject` and `Engine`. |
| `StillCookingCoreEditor` | UncookedOnly | The `Print String Formatted` graph node. Present wherever Blueprints are compiled — the editor and uncooked commandlets — and absent from a packaged game. |

[Which module exists in which build configuration](Documentation/concepts/builds-and-shipping.md#two-module-types),
and why the second one is `UncookedOnly`.

## Requirements

- A C++ project — [the plugin is distributed as source and compiled together with your
  project](Documentation/start/installation.md#requirements)
- Visual Studio 2022 with the *Game development with C++* workload
- Unreal Engine 5.7 or 5.8 on Win64

Those are the only combinations [built and tested; others are not
verified](Documentation/project/compatibility.md). Every release up to 0.7.1 was built against 5.7 only;
0.8.0 is the first built and tested against 5.8 as well. **Unreal Engine 5.5 and 5.6 do
not compile** — the plugin uses engine API that first appeared in 5.7. Other platforms, and engine
versions not named here, may work: the plugin needs only `Core`,
`CoreUObject` and `Engine`, plus the Blueprint-graph modules for its editor module. None of them is
verified, and a problem on any of them is worth reporting.
[The matrix release by release, and where the verification gaps are recorded](Documentation/project/compatibility.md).

## Limits worth knowing up front

- **A C++ project is required.** The plugin is distributed as source and compiled together with
  your project, so [a Blueprint-only project will not build
  it](Documentation/start/installation.md#requirements).
- **Two entries are development-only.** The console command registry
  [compiles its registration out of Shipping](Documentation/concepts/builds-and-shipping.md#what-is-gone-in-shipping),
  and `Print String Formatted` is [compiled out of Shipping and Test
  builds](Documentation/concepts/builds-and-shipping.md#what-is-gone-in-shipping).
- **The C++ layer under the Flow nodes is EXPERIMENTAL.** The Blueprint nodes are stable; the
  `FSCFlow*` classes under them
  [may change shape without a major version bump](Documentation/concepts/overview.md#the-stable-surface-and-the-experimental-layer).
- **[Nothing replicates](Documentation/advanced/limits.md#plugin-wide).** No entry knows about the
  network.
- **[Game Thread only](Documentation/advanced/limits.md#plugin-wide).** Every entry is driven by
  the world's tick pass or by the console. Nothing in the plugin is thread-safe; calls from other
  threads are unsupported.

[The whole list of limits](Documentation/advanced/limits.md).

## Documentation

The full documentation is in [`Documentation/`](Documentation/), one folder per group. Every
page stands on its own: read the one for the class or node you came for and skip the rest.

| Group | Covers |
|---|---|
| [`start/`](Documentation/start/README.md) | Installation of a source-distributed plugin, the first node in a graph, and three checks that the build is good. |
| [`concepts/`](Documentation/concepts/README.md) | Why the plugin is a set of independent entries, the shape every Flow node shares, the tickable object's ownership contract, what disappears in a packaged build, and the terms the other pages use. |
| [`reference/`](Documentation/reference/README.md) | How to read a card, the pins several cards share, and then every class and node with its parameters, defaults and return values; the C++ cores; the shared enums, delegates and console variables. |
| [`advanced/`](Documentation/advanced/README.md) | Recipes that combine entries, driving the Flow cores from C++, and the plugin's limits. |
| [`troubleshooting/`](Documentation/troubleshooting/README.md) | Symptoms and their causes, the literal messages the plugin logs, and the diagnostic procedures. |
| [`project/`](Documentation/project/README.md) | Compatibility, the update procedure, how to report an issue, and the MIT license. |

The terms the pages use without defining them each time — entry, Flow node, proxy pin, latent
body — are in the [Glossary](Documentation/concepts/glossary.md).

## Versioning

The version in `StillCooking_Tools.uplugin` follows semantic versioning.
[Anything in a `Public/` header is treated as a
contract](Documentation/concepts/overview.md#the-stable-surface-and-the-experimental-layer):
renaming or removing a public symbol, or changing a `UFUNCTION` signature, takes a major version
bump and a changelog entry.
[The exception is the EXPERIMENTAL C++ layer](Documentation/project/updates.md#what-the-version-number-means),
described above. Release-by-release history: [`CHANGELOG.md`](CHANGELOG.md).

## Support

Bug reports and feature requests:
[GitHub Issues](https://github.com/StillCooking/StillCooking_Tools/issues). Include your engine
version, the plugin version and, where relevant, the log excerpt —
[what to put in a report so the problem can be reproduced](Documentation/project/issues.md).

## License

MIT — the full text is in [`LICENSE`](LICENSE), and it is the binding document. You may use,
modify and redistribute the plugin, commercially or not, as long as the copyright notice and the
license text travel with the code.
[A compiled game that uses the plugin needs no notice](Documentation/project/license.md#in-plain-terms).
