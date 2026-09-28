#include "Objects/SCTickableObject.h"

#include "Engine/World.h"
#include "SCLogChannels.h"

USCTickableObject::USCTickableObject()
	: FTickableGameObject(ETickableTickType::Never)
{
	// Empty on purpose: Tickable.h forbids registering for tick here, and archetype / Blueprint
	// defaults land after the constructor body.
}

void USCTickableObject::PostInitProperties()
{
	Super::PostInitProperties();

	bTickEnabled = bStartTickingOnInitialize;
}

void USCTickableObject::Initialize()
{
	// ensureAlwaysMsgf: a plain ensure fires once per callsite, so the first double-Initialize()
	// would silence every later one.
	if (!ensureAlwaysMsgf(!bInitialized, TEXT("%s: Initialize called twice."), *GetName()))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogStillCooking, Error, TEXT("%s: Initialize found no world on the outer chain."), *GetName());
		return;
	}

	CachedWorld = World;
	TimeSinceLastTick = 0.f;
	bInitialized = true;

	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &USCTickableObject::HandleWorldCleanup);

	ReceiveInitialize();

	// Applies whatever the intent is by now - the configured default, or an EnableTick()/DisableTick()
	// from the owner or from ReceiveInitialize().
	ApplyTickIntent();
}

void USCTickableObject::Shutdown()
{
	if (!bInitialized)
	{
		return;
	}

	// Not DisableTick(): that would record "off" as the owner's intent and a later Initialize() would
	// refuse to tick a reusable object.
	SetTickableTickType(ETickableTickType::Never);

	ReceiveShutdown();

	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	WorldCleanupHandle.Reset();

	bInitialized = false;
	bTickEnabled = bStartTickingOnInitialize;
	CachedWorld.Reset();
}

void USCTickableObject::BeginDestroy()
{
	// No Shutdown() here: on the GC path the object is already flagged Unreachable, and a Blueprint
	// ReceiveShutdown would hit UObject::ProcessEvent's checkf(!IsUnreachable()) and hard-crash.
	// This override only makes the object safe to delete, as UTickableWorldSubsystem does.
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	WorldCleanupHandle.Reset();

	// Tickable.h requires the tick type to be Never by BeginDestroy at the latest.
	SetTickableTickType(ETickableTickType::Never);
	bTickEnabled = false;

	ensureAlwaysMsgf(!bInitialized,
		TEXT("%s: destroyed while still initialized - the owner never called Shutdown()."), *GetName());

	Super::BeginDestroy();
}

UWorld* USCTickableObject::GetWorld() const
{
	if (UWorld* World = CachedWorld.Get())
	{
		return World;
	}

	// Before Initialize() fall through to the outer walk - Initialize() resolves the world through here.
	return Super::GetWorld();
}

void USCTickableObject::EnableTick()
{
	bTickEnabled = true;
	ApplyTickIntent();
}

void USCTickableObject::DisableTick()
{
	bTickEnabled = false;
	ApplyTickIntent();
}

void USCTickableObject::ApplyTickIntent()
{
	// Before Initialize() the intent is only recorded: registering an uninitialized object would put it
	// in front of the tick pass without a resolved world.
	if (!bInitialized)
	{
		return;
	}

	// Conditional literally, not GetTickableTickType(): a subclass returning Always would skip
	// IsTickable(), where the dead-world guard sits.
	SetTickableTickType(bTickEnabled ? ETickableTickType::Conditional : ETickableTickType::Never);
}

void USCTickableObject::SetTickInterval(const float NewTickInterval)
{
	const float ClampedInterval = FMath::Max(0.f, NewTickInterval);

	// While the interval is non-positive Tick() never touches TimeSinceLastTick, so its leftover
	// contents are time already delivered - carrying it over would deliver it twice and fire early.
	if (TickInterval <= 0.f && ClampedInterval > 0.f)
	{
		TimeSinceLastTick = 0.f;
	}

	TickInterval = ClampedInterval;
}

void USCTickableObject::Tick(const float DeltaTime)
{
	if (TickInterval <= 0.f)
	{
		ReceiveTick(DeltaTime);
		return;
	}

	TimeSinceLastTick += DeltaTime;
	if (TimeSinceLastTick < TickInterval)
	{
		return;
	}

	// Carry the sub-interval remainder and report the elapsed time minus it, so no slice is delivered
	// twice. No catch-up: a hitch spanning several intervals produces a single call covering all of them.
	const float Carried = FMath::Fmod(TimeSinceLastTick, TickInterval);
	const float Elapsed = TimeSinceLastTick - Carried;
	TimeSinceLastTick = Carried;
	ReceiveTick(Elapsed);
}

ETickableTickType USCTickableObject::GetTickableTickType() const
{
	// Never for the CDO and before Initialize: the object stays out of the tickable array instead of
	// sitting in it and being polled every frame.
	return (IsTemplate() || !bInitialized) ? ETickableTickType::Never : ETickableTickType::Conditional;
}

bool USCTickableObject::IsTickable() const
{
	// CachedWorld.IsValid() closes the window before HandleWorldCleanup runs Shutdown(): a null
	// GetTickableGameObjectWorld() matches the nullptr World UGameEngine::Tick passes, which would
	// migrate this object onto the global engine tick pass with a dead world under it.
	return bInitialized && bTickEnabled && CachedWorld.IsValid() && IsValidChecked(this);
}

UWorld* USCTickableObject::GetTickableGameObjectWorld() const
{
	return CachedWorld.Get();
}

TStatId USCTickableObject::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(USCTickableObject, STATGROUP_Tickables);
}

void USCTickableObject::ReceiveTick_Implementation(float DeltaTime)
{
}

void USCTickableObject::ReceiveInitialize_Implementation()
{
}

void USCTickableObject::ReceiveShutdown_Implementation()
{
}

void USCTickableObject::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (World == CachedWorld.Get())
	{
		Shutdown();
	}
}
