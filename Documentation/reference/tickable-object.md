# Tickable object

*`USCTickableObject` — lifecycle functions, tick intent, interval, the two engine gates, the events, and the messages it logs.*

## `USCTickableObject`

*Available in: C++ and Blueprint.* Abstract, `Blueprintable`, `BlueprintType`. Module
`StillCookingCore`, header `Objects/SCTickableObject.h`. Blueprint category
`StillCooking|Tickable`.

A base class for a `UObject` that ticks once per frame. It is abstract: you derive from it — in
C++, or as a Blueprint child — and put your logic in the `Tick` event. Ticking does not start in
the constructor; it starts when the owner calls `Initialize()`. The owner takes on two
obligations the class does not: [holding the object, which never roots itself, and calling
`Shutdown()` before dropping it](../concepts/lifecycle.md#the-contract).

**C++.** A minimal subclass overrides `ReceiveTick_Implementation`. The engine-facing interface
(`IsTickable`, `GetTickableTickType`, `GetTickableGameObjectWorld`, `BeginDestroy`,
`PostInitProperties`) is `protected`; the engine reaches it through base pointers, and a consumer
does not call it. `IsTickable()` is `final` — it holds the dead-world guard, and a subclass gates
its own ticking with `EnableTick()` / `DisableTick()` instead of overriding it. `GetWorld()` is
public and returns the cached world after `Initialize()`; before that it walks the outer chain.

```cpp
UCLASS()
class UMyTicker : public USCTickableObject
{
	GENERATED_BODY()

protected:
	virtual void ReceiveTick_Implementation(float DeltaTime) override
	{
		// once per frame while initialized and enabled
	}
};

UCLASS()
class AMyActor : public AActor
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UMyTicker> Ticker;

	virtual void BeginPlay() override
	{
		Super::BeginPlay();
		Ticker = NewObject<UMyTicker>(this);   // the actor's outer chain reaches the world
		Ticker->Initialize();
	}

	virtual void EndPlay(const EEndPlayReason::Type Reason) override
	{
		if (Ticker) { Ticker->Shutdown(); }
		Super::EndPlay(Reason);
	}
};
```

## Lifecycle

### `Initialize()`

*Available in: C++ and Blueprint.*

- **C++ call** — `Object->Initialize()`
- **Blueprint node** — `Initialize`

**Returns:** nothing.

Resolves the world from the outer chain, marks the object initialized, fires the `Initialize`
event and applies the tick intent: if the intent is *on* (see below), the object registers for
ticking and ticks from the next tick pass. Never call it from a constructor.

A second call on an initialized object fires
[an `ensure` saying `Initialize` was called twice](../troubleshooting/diagnostics.md#usctickableobject),
and does nothing.

An outer chain with no `UWorld` on it leaves the object uninitialized and logs
[an `Error` saying no world was found on the outer chain](../troubleshooting/diagnostics.md#usctickableobject).

### `Shutdown()`

*Available in: C++ and Blueprint.*

- **C++ call** — `Object->Shutdown()`
- **Blueprint node** — `Shutdown`

**Returns:** nothing.

Stops ticking, fires the `Shutdown` event and clears the initialized state. Idempotent — a second
call does nothing. Restores the tick intent to `bStartTickingOnInitialize`, so a later
`Initialize()` starts from the configured default again rather than from whatever
`EnableTick()`/`DisableTick()` last set.

It is the only path that runs subclass teardown:
[`BeginDestroy` does **not** call it for you](../concepts/lifecycle.md#when-you-forget-shutdown).

The object also calls `Shutdown()` on itself when its world is cleaned up — the end of a PIE
session, a map change — so the owner does not have to handle that case.

### `IsInitialized()`

*Available in: C++ and Blueprint (pure).*

- **C++ call** — `Object->IsInitialized()`
- **Blueprint node** — `Is Initialized`

**Returns:** `bool` — `true` between a successful `Initialize()` and the next `Shutdown()`.

## Tick control

### `EnableTick()`

*Available in: C++ and Blueprint.*

- **C++ call** — `Object->EnableTick()`
- **Blueprint node** — `Enable Tick`

Sets the tick intent to *on*. After `Initialize()`, takes effect at the next tick pass. Before
`Initialize()`, only records the intent, which `Initialize()` then applies — an explicit call
outranks `bStartTickingOnInitialize`.

### `DisableTick()`

*Available in: C++ and Blueprint.*

- **C++ call** — `Object->DisableTick()`
- **Blueprint node** — `Disable Tick`

Sets the tick intent to *off*. After `Initialize()`, unregisters the object from the tickable
array rather than leaving it polled every frame with nothing to do. Before `Initialize()`,
records the intent; `Initialize()` then leaves the object unregistered, even if
`bStartTickingOnInitialize` is `true`.

### `IsTickEnabled()`

*Available in: C++ and Blueprint (pure).*

- **C++ call** — `Object->IsTickEnabled()`
- **Blueprint node** — `Is Tick Enabled`

**Returns:** `bool` — the current tick intent.

### `SetTickInterval(NewTickInterval)`

*Available in: C++ and Blueprint.*

- **C++ call** — `Object->SetTickInterval(0.5f)`
- **Blueprint node** — `Set Tick Interval`

| Parameter | Meaning |
| --- | --- |
| `NewTickInterval` | seconds between `Tick` calls; `0` means every frame; negative values are clamped to `0` |

**Returns:** nothing.

The only supported way to change the interval at runtime; the backing property is private so a
direct write cannot skip the accumulator restart this function performs.

With a positive interval, `Tick` fires once the accumulated game time reaches the interval, and
its `DeltaTime` is that accumulated time minus the part below one interval, which carries over to
the next call. A body integrating against `DeltaTime` therefore sees the same total game time as
an every-frame body. There is no catch-up: a hitch spanning several intervals produces one
`Tick`, whose `DeltaTime` covers the whole hitch.

### `GetTickInterval()`

*Available in: C++ and Blueprint (pure).*

- **C++ call** — `Object->GetTickInterval()`
- **Blueprint node** — `Get Tick Interval`

**Returns:** `float` — the current interval in seconds.

## Properties

| Property | Type | Default | Meaning |
| --- | --- | --- | --- |
| `bStartTickingOnInitialize` | `bool` | `true` | The starting tick intent. Copied into the intent when the object's properties are initialized, and again by `Shutdown()`; `Initialize()` applies the intent as it stands. `EnableTick()`/`DisableTick()` override it. |
| `bTickWhenPaused` | `bool` | `false` | Keep ticking while the game is paused. |
| `bTickInEditor` | `bool` | `false` | Tick in an editor world (no Play session). See the surprise below. |

`TickInterval` is private in C++ and editable in a Blueprint child's class defaults; at runtime,
read it with `GetTickInterval()` and write it with `SetTickInterval()`.

**Set `bStartTickingOnInitialize` in class defaults, not on a live object.** The object copies
it into its tick intent once, while its properties are initialized during `NewObject`, and
`Initialize()` applies that copy. Assigning the property in C++ after `NewObject` changes nothing
until the next `Shutdown()` copies it again. To change the intent of an existing object, call
`EnableTick()` or `DisableTick()`.

## The two engine gates

`bTickWhenPaused` and `bTickInEditor` feed the engine's own tick dispatch, which checks them in a
fixed order: *editor* first, then *game world and not paused*, then *game world and paused and
tick-when-paused*. `bTickInEditor` is an **or**-branch, not a modifier — it bypasses the paused
and game-world rules entirely.

The consequence: the engine's editor flag, `GIsEditor`, is true for the whole editor process,
**including a PIE session**. So an object with `bTickInEditor = true` keeps ticking through a PIE
pause even with `bTickWhenPaused = false`. In a packaged build the flag is false, the branch never
fires, and the same object obeys the pause and stops. Setting `bTickInEditor` to get editor-world
ticking silently opts the object out of pause handling for the whole editor session.

Without `bTickInEditor`, an initialized and enabled object in an editor world stays silent.

## Events

| Event (Blueprint) | C++ override | When |
| --- | --- | --- |
| `Tick(DeltaTime)` | `ReceiveTick_Implementation(float DeltaTime)` | once per tick pass while initialized and enabled; `DeltaTime` is the interval consumed |
| `Initialize` | `ReceiveInitialize_Implementation()` | from `Initialize()`, after the world is resolved |
| `Shutdown` | `ReceiveShutdown_Implementation()` | from `Shutdown()`, before the initialized state is cleared |

## Messages

Three, each with the object's name in front. Their
[exact wording, and what each one means](../troubleshooting/diagnostics.md#usctickableobject):

| When | Kind |
| --- | --- |
| `Initialize()` is called on an already initialized object | ensure |
| the outer chain resolves no `UWorld` | Error |
| the object is garbage-collected before `Shutdown()` | ensure, every instance |

## Pitfalls

- **[Do not create the object in the Construction
  Script](../concepts/lifecycle.md#when-you-forget-shutdown).** Create it in `BeginPlay`.
- **`bTickInEditor` changes pause behaviour in PIE** — see above.
- **`IsTickable()` and the rest of the engine-facing interface are `protected`.** Code that calls
  them on this class does not compile; the engine reaches them through base pointers.
- **`TickInterval` is private** — read it with `GetTickInterval()`.

---

← [Reference](README.md) · [Documentation index](../../README.md#documentation)
