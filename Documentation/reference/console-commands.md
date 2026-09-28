# Console commands

*`USCConsoleCommandRegistrySubsystem` — the dynamic and the static path, registration and cleanup, and the naming convention.*

## `USCConsoleCommandRegistrySubsystem`

*Available in: C++ and Blueprint.* `UGameInstanceSubsystem`, `Blueprintable`. Display name
**SC Console Command Registry**. Module `StillCookingCore`, header
`Debug/SCConsoleCommandRegistrySubsystem.h`. Blueprint category `StillCooking|Console`.

Lets a Blueprint define console commands. The engine's `UFUNCTION(Exec)` cannot do that, and its
command names cannot contain dots, so they never group in autocomplete. Two paths:

| | Dynamic | Static |
| --- | --- | --- |
| Command defined in | Blueprint, entirely | a C++ stub + a Blueprint body |
| C++ per new command | none | a few lines |
| Name exists from | game or PIE start | module load, editor included |
| Needs a Blueprint child of the registry | only to register from `On Register Commands`; any object can register through `Get Console Command Registry` | yes |

Both execute only when a game world exists — which is why
[a name can be in the console before it can run](../concepts/builds-and-shipping.md#visible-is-not-executable). And both are
development tooling: [registration is compiled out of Shipping](../concepts/builds-and-shipping.md#what-is-gone-in-shipping),
and every command carries `ECVF_Cheat`.

## One Blueprint child per project

The subsystem yields to its subclasses: if any derived class exists, the base class does not
create itself, and the derived one runs instead. That is what lets a Blueprint child *replace*
the C++ registry rather than run beside it as a second instance. It also has two costs. Two
Blueprint children that both derive directly from the registry are two live registries. A C++
subclass in an always-loaded module replaces the real registry in every editor session. Make
exactly one Blueprint child and no C++ ones.

## The dynamic path

1. Create a Blueprint child of **SC Console Command Registry**.
2. Implement the `On Register Commands` event.
3. In it, call `Register Console Command` for each command, binding `Callback` to a custom event
   on the same Blueprint.

The commands exist from the moment the Game Instance starts and are removed when it shuts down.

Any other object can register commands too — an actor from its `BeginPlay`, through
`Get Console Command Registry` — and remove them from `EndPlay` with `Unregister Commands For`.

### `Resolve(WorldContextObject)`

*Available in: C++ and Blueprint (pure).*

- **C++ call** — `USCConsoleCommandRegistrySubsystem::Resolve(this)`
- **Blueprint node** — `Get Console Command Registry`

| Parameter | Meaning |
| --- | --- |
| `WorldContextObject` | the world context; hidden in Blueprint, filled in by `self` |

**Returns:** `USCConsoleCommandRegistrySubsystem*` — the live registry for that world: the C++
class, or the Blueprint child if one exists. `nullptr` without a game world.

### `RegisterConsoleCommand(Name, Help, Callback)`

*Available in: C++ and Blueprint.*

- **C++ call** — `Registry->RegisterConsoleCommand(TEXT("MyGame.Debug.GiveItem"), TEXT("..."), Callback)`
- **Blueprint node** — `Register Console Command`

| Parameter | Meaning |
| --- | --- |
| `Name` | the console name, e.g. `MyGame.Debug.GiveItem`; dots group it in autocomplete |
| `Help` | the text the console shows for it; under the advanced arrow in Blueprint |
| `Callback` | an `FSCBlueprintConsoleCommand` delegate — in Blueprint, a custom event with one `Args` pin of type `Array of String`; in C++, a `UFUNCTION` with the signature `void F(const TArray<FString>& Args)` |

**Returns:** `bool` — `false` when the name is empty, the callback is unbound, the name is already
taken (in this registry or anywhere else in the console manager), or the build is Shipping.

`Args` are the raw, unparsed console tokens after the command name. The owner of the callback is
held weakly: a command whose owner has been garbage-collected does nothing instead of crashing.

### `UnregisterConsoleCommand(Name)`

*Available in: C++ and Blueprint.*

- **C++ call** — `Registry->UnregisterConsoleCommand(TEXT("MyGame.Debug.GiveItem"))`
- **Blueprint node** — `Unregister Console Command`

| Parameter | Meaning |
| --- | --- |
| `Name` | the name given to `RegisterConsoleCommand` |

**Returns:** `bool` — `false` if this registry never owned that name.

### `UnregisterCommandsFor(Owner)`

*Available in: C++ and Blueprint.*

- **C++ call** — `Registry->UnregisterCommandsFor(this)`
- **Blueprint node** — `Unregister Commands For`

| Parameter | Meaning |
| --- | --- |
| `Owner` | the object whose callbacks were bound; `nullptr` (Blueprint: `None`) removes nothing |

**Returns:** `int32` — how many commands were removed.

Pairs with the registration site: register in `BeginPlay`, call this in `EndPlay`, and you never
have to track names. An empty `Owner` is rejected, not treated as a wildcard.

### `GetRegisteredCommandNames()`

*Available in: C++ and Blueprint (pure).*

- **C++ call** — `Registry->GetRegisteredCommandNames()`
- **Blueprint node** — `Get Registered Command Names`

**Returns:** `TArray<FString>` — the names this registry currently owns. Dynamic path only:
static commands belong to no registry.

### `On Register Commands`

*Blueprint event only* (`BlueprintImplementableEvent`) — implement it in the Blueprint child; there
is no C++ override.

The dynamic path's entry point. Called once when the registry initializes; call
`RegisterConsoleCommand` from here. Not called in Shipping.

## The static path

A static command is a few lines of C++ in **your own module** that declare the name, the help
text and the cheat flag, and forward every invocation to the registry. The body lives in the
Blueprint child's `On Static Command` event — which is why this path needs a child at all: an
event needs a Blueprint to implement it. The name is in the editor's autocomplete from module
load, before any Play session.

The plugin ships one such command as a worked example, `SC.Debug.Ping`, from the file
`Source/StillCookingCore/Private/Debug/SCConsoleCommands_Debug.cpp`:

```cpp
#include "Debug/SCConsoleCommandRegistrySubsystem.h"

#if !UE_BUILD_SHIPPING

#include "HAL/IConsoleManager.h"

static FAutoConsoleCommandWithWorldAndArgs GSCCmdPing(
	TEXT("SC.Debug.Ping"),
	TEXT("Raises the On Static Command event with the identifier 'Ping'. Usage: SC.Debug.Ping [Args...]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
		[](const TArray<FString>& Args, UWorld* World)
		{
			USCConsoleCommandRegistrySubsystem::DispatchStaticCommand(World, TEXT("Ping"), Args);
		}),
	ECVF_Cheat);

#endif // !UE_BUILD_SHIPPING
```

Copy the file into your module (which needs `StillCookingCore` in its `Build.cs` — the registry
class is exported), change the name and the identifier, and never touch the registry class.
Three rules, each of which the example follows:

- **Gate the whole file, not the callback.** A static `FAutoConsoleCommand` registers itself when
  the module loads; a guard inside the callback would run far too late.
- **`FAutoConsoleCommandWithWorldAndArgs`, never plain `FAutoConsoleCommand`.** The world arrives
  with each invocation, so nothing caches a registry pointer that could dangle after PIE or point
  at the wrong world under multi-PIE.
- **Name it `<YourPrefix>.<Area>.<Action>`, PascalCase.** The dots are the only grouping the
  console has. The `SC.` prefix belongs to the plugin's own commands.

### `DispatchStaticCommand(World, CommandId, Args)`

*Available in: C++.* Static, not reflected; exported from `StillCookingCore`, so your module needs
it in `Build.cs`.

- **C++ call** — `USCConsoleCommandRegistrySubsystem::DispatchStaticCommand(World, TEXT("Ping"), Args)`

| Parameter | Meaning |
| --- | --- |
| `World` | the world the console passed to the stub |
| `CommandId` | the identifier the Blueprint switches on — `Ping` in the example |
| `Args` | the console tokens, passed through verbatim |

**Returns:** `bool` — `false`, with a `Warning`, when there is no game world; `false` in Shipping.

### `On Static Command`

*Blueprint event only* (`BlueprintImplementableEvent`), parameters `CommandId` (Name) and `Args`
(Array of String) — implement it in the Blueprint child; there is no C++ override.

One event for every statically registered command — switch on `CommandId`. With no Blueprint
child, the event has no implementation and dispatch reaches the registry and does nothing; that
is the contract, and it is why `SC.Debug.Ping` prints nothing in a fresh project.

## Cleanup

- **On `Deinitialize`** the registry unregisters every command it owns from the console manager.
  A console command object belongs to the console manager, not to the garbage collector, and
  outlives the subsystem; leaving one behind would keep the name in the console after PIE and
  make the next session's registration bounce off the duplicate guard.
- **On every registration** the registry first sweeps out commands whose owners are gone, then
  checks for duplicates — so a name held by a dead owner cannot block a fresh registration of the
  same name.
- **Explicitly**, through `UnregisterConsoleCommand` and `UnregisterCommandsFor`.

The console manager makes no promise that unregistering a command from inside its own execution
survives it. Unregister after the command has returned — from the next tick, or from `EndPlay`.

## Messages

Eight, all on `LogStillCooking`. Their
[exact wording, and what each one means](../troubleshooting/diagnostics.md#uscconsolecommandregistrysubsystem):

| When | Level |
| --- | --- |
| the subsystem starts — the class name in it tells you whether your Blueprint child won | Log |
| a registration is rejected for an empty name or an unbound callback | Warning |
| the name is already taken, so the first registration stays | Warning |
| the console manager refused the name | Warning |
| a command is registered, removed, or swept out because its owner is gone | Verbose, three messages |
| a static command is run outside Play | Warning |

## Pitfalls

- **The Construction Script is the wrong host for registration.** It runs in the editor world,
  where there is no Game Instance and therefore no registry, so the call silently does nothing
  outside PIE — which makes it look like it works when tried only in PIE. It also re-runs on
  every reconstruction, and a second actor of the same class gets `false` back, which is rarely
  checked. Register in `BeginPlay`, unregister in `EndPlay`.
- **No C++ subclass of the registry** — see the one-child rule above.
- **A dynamic delegate target has to be a `UFUNCTION` on a `UObject`.** A plain lambda cannot be
  bound to `FSCBlueprintConsoleCommand`.
- **The editor console lists static commands that cannot run** outside Play. By design: the
  command registers with the console manager at module load and nothing can withdraw the name
  until a world exists — and autocomplete before Play is the reason the static path exists at
  all. The warning names the command.
- **The class-selection rule needs care in a cooked build** — [what would go wrong, and the
  workaround](../concepts/builds-and-shipping.md#the-registrys-class-selection-in-a-cooked-build).

---

← [Reference](README.md) · [Documentation index](../../README.md#documentation)
