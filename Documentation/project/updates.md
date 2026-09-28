# Updates

*Pulling a new version, what the version number promises, the EXPERIMENTAL exception, the two source-breaking changes so far, and why there is no roadmap page.*

## Where to check the current version

The `VersionName` field in `StillCooking_Tools.uplugin`, and the first versioned heading in
`CHANGELOG.md`.

## The update procedure

1. **Read the changelog** for every version between yours and the new one. Entries under
   *Changed* and *Removed* are the ones that can touch your code. If you call the Flow cores from
   C++, read every entry.
2. **Commit or back up** your project.
3. **Pull.** If you cloned as the installation page suggests:

   ```
   git -C Plugins/StillCooking_Tools pull
   ```

   Otherwise replace the directory. Either way, delete the plugin's `Binaries/` and
   `Intermediate/` before building, so nothing stale survives.
4. **Regenerate project files and build.**
5. **Open the editor and compile the Blueprints** that use the plugin's nodes. A node whose pin
   set changed reports it on compile.
6. **Build Shipping** if you ship one, and check that your debug tooling degrades as intended —
   [what the console registry and the print node lose there](../concepts/builds-and-shipping.md#what-is-gone-in-shipping)
   is by design, and a graph that leaned on either stops doing that thing.

## What the version number means

Semantic versioning, with one exception.

| Part | Goes up when |
| --- | --- |
| Major | anything in a `Public/` header of the stable surface is renamed, removed, or has its `UFUNCTION` signature changed — [a `Public/` header is a contract](../concepts/overview.md#the-stable-surface-and-the-experimental-layer) |
| Minor | new, backward-compatible functionality — a new node, a new function, a new class |
| Patch | bug fixes with no observable API change |

**The exception is the EXPERIMENTAL C++ layer.** `FSCFlowLoop`, `FSCFlowDuration`,
`FSCFlowConditionPoll`, `FSCFlowTickingTask` and the types that go with them may change shape in
a minor release. The changelog records every such change, and the header of every affected class
carries the marker. The Blueprint nodes built on them are stable under the normal rules.

The two source-breaking changes so far, both inside that layer:

- **0.4.0** renamed `FSCFlowLoopFinished` to `FSCFlowFinished`. C++ callers of `FSCFlowLoop`
  rename the type; Blueprint users see nothing.
- **0.7.0** appended `TimedOut` to `ESCFlowFinish`. Existing values kept their ordinals; a
  `switch` on the enum gains a case.

## Version history

| Version | Date | Headline |
| --- | --- | --- |
| 0.8.0 | 2026-09-28 | Unreal Engine 5.8 alongside 5.7; graph images in the documentation |
| 0.7.1 | 2026-09-11 | fix: **Print String Formatted** argument pins came back as wildcards on editor restart |
| 0.7.0 | 2026-08-25 | **Wait Until**, `SC.Flow.WaitUntilWarningSeconds`, `FSCFlowConditionPoll`, `ESCFlowFinish::TimedOut` |
| 0.6.0 | 2026-08-25 | **Print String Formatted**, the `StillCookingCoreEditor` module |
| 0.5.0 | 2026-08-19 | **For Each Index Per Tick**, `ItemsPerTick` |
| 0.4.0 | 2026-08-19 | **Do For Duration**, `FSCFlowDuration`, `FSCFlowTickingTask`; `FSCFlowLoopFinished` → `FSCFlowFinished` |
| 0.3.1 | 2026-08-18 | the suspended-loop warning, `SC.Flow.SuspendedLoopWarningSeconds` |
| 0.3.0 | 2026-08-16 | **For Each Index With Delay**, `FSCFlowLoop` |
| 0.2.0 | 2026-08-15 | the console command registry, `SC.Debug.Ping`; tick-intent changes and `IsTickEnabled()` on the tickable object |
| 0.1.0 | 2026-08-14 | `USCTickableObject`, `LogStillCooking` |

Every release up to 0.7.1 targets UE 5.7 on Win64; 0.8.0 adds UE 5.8. The
full text of each entry: [`CHANGELOG.md`](../../CHANGELOG.md).

## Why there is no roadmap page

There is no roadmap page. The one change announced so far — culture-independent number formatting
in `Print String Formatted` — is in
[how the grouping is formatted today, and that it will
change](../reference/print-string-formatted.md#numbers-and-culture), and in the changelog entry
that introduced the node.

## Rolling back

The C++ Flow layer — its enums, structs and aliases — is not serialized into assets; nothing
there is a `UENUM` or a `USTRUCT`. What your assets do carry is the same as for any plugin: the
classes of the nodes placed in graphs and of your Blueprint children, with their pin and property
values, plus the `Print String Formatted` node's argument pin names.

Rolling back to a version that lacks a node or a pin your assets use breaks those assets until
you roll forward again. For example, a Blueprint using **Wait Until** fails to compile on 0.6.0
and compiles again on 0.7.0. Nothing else breaks.

## Modifying the source

[MIT lets you](license.md). If your fork changes the *stable* surface, you merge upstream changes
by hand on every update. A fork that only *adds* — a new node on the Flow cores, a subclass of the
tickable object — merges cleanly, apart from the EXPERIMENTAL layer it builds on. Renaming
`UK2Node_SCPrintStringFormatted` or its serialized pin-name property orphans the node in every
Blueprint that already uses it.

---

← [Versions, support, and license](README.md) · [Documentation index](../../README.md#documentation)
