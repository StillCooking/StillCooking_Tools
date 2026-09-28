# Third-party code and dependencies

**StillCooking_Tools 0.8.0** — last verified 2026-09-28.

StillCooking_Tools contains no third-party code and depends on no other plugins. Every module it
links is part of Unreal Engine.

Use of Unreal Engine itself is governed by the
[Unreal Engine End User License Agreement](https://www.unrealengine.com/eula).

## Engine modules linked

### Runtime module

`StillCookingCore` — public dependencies, so a module that links `StillCookingCore` inherits them:

| Module | Why |
| --- | --- |
| Core | Containers, strings, logging, `TFunction`, `TSharedRef`. |
| CoreUObject | `USCTickableObject` derives from `UObject`; the subsystem and the nodes are reflected classes. |
| Engine | `FTickableGameObject`, `UGameInstanceSubsystem`, `UBlueprintAsyncActionBase`, `UWorld`. |

### UncookedOnly module

`StillCookingCoreEditor` — private dependencies only; nothing links against this module:

| Module | Why |
| --- | --- |
| Core, CoreUObject, Engine | As above. |
| BlueprintGraph | `UK2Node` and the K2 schema, which `Print String Formatted` is built on. |
| KismetCompiler | `FKismetCompilerContext`, used when the node expands into engine calls. |
| UnrealEd | Blueprint editor utilities and the compiler results log. |

### Engine plugins

None. `StillCooking_Tools.uplugin` enables no other plugin.

## When this list changes

The list is re-checked whenever a `Build.cs` or the `.uplugin` changes; the version and date at
the top say when it was last checked.
