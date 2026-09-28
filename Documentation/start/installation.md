# Installation

*The requirements, the clone, the build, the module dependencies for C++ callers, and what the two modules are.*

## Requirements

| | |
| --- | --- |
| Engine | Unreal Engine 5.7 or 5.8 |
| Platform | Win64 |
| Project type | C++ |
| Toolchain | Visual Studio 2022 with the *Game development with C++* workload |

Those are the only combinations
[built and tested; others are not verified](../project/compatibility.md). Unreal Engine 5.5 and
5.6 [do not compile](../project/compatibility.md#known-not-to-build).

**The project has to be a C++ project.** The plugin is distributed as source and compiled together
with your project, so a Blueprint-only project will not build it; add one C++ class through
**Tools → New C++ Class** first.

## Clone and build

1. From the root of your project, clone the repository into `Plugins/`:

   ```
   git clone https://github.com/StillCooking/StillCooking_Tools.git Plugins/StillCooking_Tools
   ```

   The directory name is not significant to the engine; keeping `StillCooking_Tools` lets the
   update command in [Updates](../project/updates.md) work as written.

   Without Git, download the repository as a ZIP, unpack it into `Plugins/`, and rename the
   unpacked folder (GitHub appends the branch name, e.g. `-main`) to `StillCooking_Tools`.

2. Right-click the `.uproject` and choose **Generate Visual Studio project files**.

3. Build in the **Development Editor** configuration.

4. Open the editor. A plugin in `Plugins/` is enabled automatically; you do not need to list it in
   your `.uproject`. If you toggled it by hand in the Plugins browser, restart the editor.

## Modules

| Module | Type | What it does |
| --- | --- | --- |
| `StillCookingCore` | Runtime | Everything that runs in the game: `USCTickableObject`, `USCConsoleCommandRegistrySubsystem`, the four Flow nodes and the C++ cores under them, `LogStillCooking`. |
| `StillCookingCoreEditor` | UncookedOnly | The `Print String Formatted` graph node. |

Neither module needs an entry in your `.uproject`. They do not both survive packaging:
[which module exists in which build configuration](../concepts/builds-and-shipping.md#two-module-types),
and [why the editor one has to be `UncookedOnly`](../concepts/builds-and-shipping.md#two-module-types).

## Module dependencies

**If you use the classes from C++** — deriving from `USCTickableObject`, calling the console
registry, or driving a Flow core directly — add the runtime module to your own module's
`Build.cs`:

```csharp
PublicDependencyModuleNames.AddRange(new string[] { "StillCookingCore" });
```

`StillCookingCore` lists `Core`, `CoreUObject` and `Engine` as *public* dependencies, so linking
it brings those along. The public headers are under `Source/StillCookingCore/Public/` — for the
tickable object, `#include "Objects/SCTickableObject.h"`; each reference page names its own.

**If you work only in Blueprints**, you skip this step entirely — every node and every
Blueprintable class works with no changes to `Build.cs`.

Nothing links against `StillCookingCoreEditor`, and there is no reason to add it anywhere.

## Verification

The plugin has no menu entry and no project settings, so the check is in the Blueprint editor:
open any actor's event graph, right-click, and search for `For Each Index With Delay`. If the node
is there under `StillCooking|Flow`, the runtime module built and loaded. Two more checks, one of
them for the editor module, are in [Verification](verification.md).

---

← [Start here](README.md) · [Documentation index](../../README.md#documentation)
