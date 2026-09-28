# Overview

*A grab-bag, not a framework: what "every entry stands on its own" commits the plugin to, and where the stable surface ends and the EXPERIMENTAL layer begins.*

## Where it comes from

The classes in this plugin were written for one project and turned out to be useful in
unrelated ones. The plugin exists so that the code has one home and one version number
instead of a copy per project. That origin decides its shape: it is a collection of things
that were individually useful, not a system designed top-down.

## Independent entries

Every entry — a class or a Blueprint node — is usable without the others:

- **No entry depends on another entry.** The Flow nodes share a C++ base, but a project that uses
  only `USCTickableObject` never touches Flow, and a project that uses only **Wait Until** never
  touches the tickable object or the console registry.
- **No entry depends on anything outside the engine.** The runtime module links `Core`,
  `CoreUObject` and `Engine`, and nothing else. [There is no third-party code and no other
  plugin](../../THIRD-PARTY.md) to install.
- **No entry needs configuration.** There are no project settings, no config sections, no tags
  to register.

Because there is no unifying model, each reference page is complete on its own, and the pages in
this section explain the two places where entries do share something:
[The Flow family](flow-family.md) shares one shape across four nodes, and
[what a packaged build drops, and why a console command can be visible before it can
run](builds-and-shipping.md#what-is-gone-in-shipping), which applies to two entries at once.

## The stable surface and the EXPERIMENTAL layer

The plugin has two kinds of public API.

**Stable.** Every Blueprint node and the classes behind them: `USCTickableObject`,
`USCConsoleCommandRegistrySubsystem`, `USCFlowForEachIndexWithDelay`, `USCFlowDoForDuration`,
`USCFlowWaitUntil`, and the `Print String Formatted` node. Anything in a `Public/` header is
treated as a contract: renaming or removing a public symbol, or changing a `UFUNCTION` signature,
takes a major version bump and a changelog entry.

**EXPERIMENTAL.** The C++ cores the Flow nodes are built on — `FSCFlowLoop`, `FSCFlowDuration`,
`FSCFlowConditionPoll`, their shared base `FSCFlowTickingTask`, and the parameter structs, enums
and function aliases that go with them (`FSCFlowLoopParams`, `FSCFlowDurationParams`,
`FSCFlowWaitUntilParams`, `ESCFlowStep`, `ESCFlowFinish`, `FSCFlowLoopBody`, `FSCFlowFinished`,
`FSCFlowAlphaShaper`, `FSCFlowDurationUpdate`, `FSCFlowCondition`), and the delegate
`FSCFlowConditionSignature` that lets a Blueprint function answer the condition. Their headers
say so. They may change shape without a major version bump, and twice they have —
[which types changed, and in which release](../project/updates.md#what-the-version-number-means).

The marker is a statement about *evidence*, not about quality. The shared base was extracted when
the second core landed and held unchanged under the third, and the marker stays until a node that
runs several iterations inside a single tick has been tried against it too. Until then
[a C++ caller of the cores reads the changelog on every update](../reference/cpp-flow-cores.md);
a Blueprint user of the nodes does not have to.

Using the cores from C++ is documented in [C++ Flow cores](../reference/cpp-flow-cores.md) and
[Using the C++ cores](../advanced/cpp-cores.md).

## What the plugin does not do

- It does not provide a framework, a base game mode, a manager, or a subsystem you have to
  register things with. The one subsystem it contains is a tool for defining console commands.
- It does not replicate anything. Nothing here knows about the network.
- It does not ship content. `CanContainContent` is set in the descriptor, but the distribution
  carries no assets.
- It does not support engine versions or platforms beyond the ones in the compatibility table,
  [which records what has been built and tested](../project/compatibility.md).

The plugin as a whole is marked **beta**: the descriptor sets `IsBetaVersion`, so the editor's
Plugins browser shows it with a Beta label.

---

← [Concepts](README.md) · [Documentation index](../../README.md#documentation)
