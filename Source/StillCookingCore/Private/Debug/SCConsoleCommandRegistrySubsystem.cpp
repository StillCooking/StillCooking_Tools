#include "Debug/SCConsoleCommandRegistrySubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "SCLogChannels.h"
#include "UObject/UObjectHash.h"

USCConsoleCommandRegistrySubsystem* USCConsoleCommandRegistrySubsystem::Resolve(const UObject* WorldContextObject)
{
	// ReturnNull, not Assert: console commands are routinely typed with no game running, so "no world"
	// is an expected answer rather than a programming error.
	const UWorld* World = GEngine != nullptr
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;

	UGameInstance* GameInstance = World != nullptr ? World->GetGameInstance() : nullptr;
	if (GameInstance == nullptr)
	{
		return nullptr;
	}

	// Asking for the base class finds a Blueprint child too: a SubsystemMap miss falls through to an
	// IsChildOf sweep. Nothing is cached - the world arrives with each console invocation, so no
	// pointer can dangle after PIE or point at the wrong world under multi-PIE.
	return GameInstance->GetSubsystem<USCConsoleCommandRegistrySubsystem>();
}

bool USCConsoleCommandRegistrySubsystem::DispatchStaticCommand(
	UWorld* World, const FName CommandId, const TArray<FString>& Args)
{
#if UE_BUILD_SHIPPING
	return false;
#else
	USCConsoleCommandRegistrySubsystem* Registry = Resolve(World);
	if (Registry == nullptr)
	{
		UE_LOG(LogStillCooking, Warning,
			TEXT("SC.%s: no game world - a statically registered command is visible from editor start, ")
			TEXT("but its body lives in a Blueprint that exists only once the game or a PIE session runs."),
			*CommandId.ToString());
		return false;
	}

	Registry->OnStaticCommand(CommandId, Args);
	return true;
#endif
}

bool USCConsoleCommandRegistrySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}

	// The collection instantiates every non-abstract subclass, so a Blueprint child would be created
	// alongside this one rather than in place of it. Yield so only the most-derived class survives.
	// The condition cannot tell an intended subclass from an incidental one - see the class comment.
	TArray<UClass*> DerivedClasses;
	GetDerivedClasses(GetClass(), DerivedClasses, /*bRecursive*/ true);
	return DerivedClasses.Num() == 0;
}

void USCConsoleCommandRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogStillCooking, Log, TEXT("Console command registry initialized (%s)."), *GetClass()->GetName());

#if !UE_BUILD_SHIPPING
	OnRegisterCommands();
#endif
}

void USCConsoleCommandRegistrySubsystem::Deinitialize()
{
	UnregisterAll();
	Super::Deinitialize();
}

bool USCConsoleCommandRegistrySubsystem::RegisterConsoleCommand(
	const FString& Name, const FString& Help, const FSCBlueprintConsoleCommand& Callback)
{
#if UE_BUILD_SHIPPING
	return false;
#else
	if (Name.IsEmpty() || !Callback.IsBound())
	{
		UE_LOG(LogStillCooking, Warning,
			TEXT("RegisterConsoleCommand: rejected '%s' - empty name or unbound callback."), *Name);
		return false;
	}

	// Before the duplicate check: a name held by a dead owner must not block a fresh registration.
	PruneStaleCommands();

	if (IConsoleManager::Get().FindConsoleObject(*Name) != nullptr)
	{
		UE_LOG(LogStillCooking, Warning, TEXT("RegisterConsoleCommand: '%s' already exists, skipped."), *Name);
		return false;
	}

	// The captured dynamic delegate holds an FWeakObjectPtr, so ExecuteIfBound turns into a no-op once
	// the owning Blueprint is gone - the usual ban on lambdas outliving their captures does not apply.
	// The IConsoleCommand is the part that genuinely outlives us; Deinitialize tears it down.
	const IConsoleCommand* Command = IConsoleManager::Get().RegisterConsoleCommand(
		*Name, *Help,
		FConsoleCommandWithArgsDelegate::CreateLambda(
			[Callback](const TArray<FString>& Args) { Callback.ExecuteIfBound(Args); }),
		ECVF_Cheat);

	if (Command == nullptr)
	{
		UE_LOG(LogStillCooking, Warning, TEXT("RegisterConsoleCommand: '%s' was refused by the console manager."), *Name);
		return false;
	}

	RegisteredCommands.Add(Name, FWeakObjectPtr(Callback.GetUObject()));
	UE_LOG(LogStillCooking, Verbose, TEXT("RegisterConsoleCommand: '%s' registered."), *Name);
	return true;
#endif
}

bool USCConsoleCommandRegistrySubsystem::UnregisterConsoleCommand(const FString& Name)
{
	// No Shipping guard needed: nothing ever reaches the map there, so this reports false on its own.
	if (RegisteredCommands.Remove(Name) == 0)
	{
		return false;
	}

	IConsoleManager::Get().UnregisterConsoleObject(*Name);
	UE_LOG(LogStillCooking, Verbose, TEXT("UnregisterConsoleCommand: '%s' removed."), *Name);
	return true;
}

int32 USCConsoleCommandRegistrySubsystem::UnregisterCommandsFor(const UObject* Owner)
{
	// A null owner is not a wildcard - one stray call with an unset Blueprint pin would empty the map.
	if (Owner == nullptr)
	{
		return 0;
	}

	TArray<FString> Doomed;
	for (const TPair<FString, FWeakObjectPtr>& Registered : RegisteredCommands)
	{
		if (Registered.Value.Get() == Owner)
		{
			Doomed.Add(Registered.Key);
		}
	}

	for (const FString& Name : Doomed)
	{
		RegisteredCommands.Remove(Name);
		IConsoleManager::Get().UnregisterConsoleObject(*Name);
	}

	return Doomed.Num();
}

void USCConsoleCommandRegistrySubsystem::PruneStaleCommands()
{
	TArray<FString> Doomed;
	for (const TPair<FString, FWeakObjectPtr>& Registered : RegisteredCommands)
	{
		// IsValid() covers destroyed and garbage-collected alike. An entry with no owner at all cannot
		// occur here - RegisterConsoleCommand refuses unbound callbacks.
		if (!Registered.Value.IsValid())
		{
			Doomed.Add(Registered.Key);
		}
	}

	for (const FString& Name : Doomed)
	{
		RegisteredCommands.Remove(Name);
		IConsoleManager::Get().UnregisterConsoleObject(*Name);
		UE_LOG(LogStillCooking, Verbose,
			TEXT("PruneStaleCommands: '%s' removed - its owner is gone."), *Name);
	}
}

TArray<FString> USCConsoleCommandRegistrySubsystem::GetRegisteredCommandNames() const
{
	TArray<FString> Names;
	RegisteredCommands.GetKeys(Names);
	return Names;
}

void USCConsoleCommandRegistrySubsystem::UnregisterAll()
{
	// Mandatory, not tidiness: a name left behind survives the game instance, so the next PIE session
	// bounces off the duplicate guard and the stale command points at a graph that no longer exists.
	for (const TPair<FString, FWeakObjectPtr>& Registered : RegisteredCommands)
	{
		IConsoleManager::Get().UnregisterConsoleObject(*Registered.Key);
	}

	RegisteredCommands.Reset();
}
