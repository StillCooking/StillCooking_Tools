#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SCConsoleCommandRegistrySubsystem.generated.h"

/** Blueprint-side body of a console command. Args are the raw, unparsed console tokens. */
DECLARE_DYNAMIC_DELEGATE_OneParam(FSCBlueprintConsoleCommand, const TArray<FString>&, Args);

/**
 * Lets Blueprints define console commands on two paths: registered at runtime from a Blueprint child
 * of this class (dynamic), or declared by a C++ stub at module load that forwards to OnStaticCommand
 * (static). Both need a game world to execute. Commands carry ECVF_Cheat and every registration path
 * is compiled out of Shipping.
 *
 * ShouldCreateSubsystem yields whenever a derived class exists, so the most-derived class is the one
 * that runs. A subclass that is not meant to be the registry - a test double, for instance - stands
 * the real one down in every session its module loads.
 *
 * @see README.md for usage and examples.
 */
UCLASS(Blueprintable, meta = (DisplayName = "SC Console Command Registry"))
class STILLCOOKINGCORE_API USCConsoleCommandRegistrySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** The live registry for a world - this class, or a Blueprint child if one exists. Null without a game world. */
	UFUNCTION(BlueprintPure, Category = "StillCooking|Console",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Get Console Command Registry"))
	static USCConsoleCommandRegistrySubsystem* Resolve(const UObject* WorldContextObject);

	/**
	 * Registers a console command whose body is a Blueprint event (dynamic path).
	 * False when the name is empty or already taken, the callback is unbound, or the build is Shipping.
	 */
	UFUNCTION(BlueprintCallable, Category = "StillCooking|Console", meta = (AdvancedDisplay = "Help"))
	bool RegisterConsoleCommand(const FString& Name, const FString& Help,
		const FSCBlueprintConsoleCommand& Callback);

	/** Removes a command registered through RegisterConsoleCommand. False if this registry never owned that name. */
	UFUNCTION(BlueprintCallable, Category = "StillCooking|Console")
	bool UnregisterConsoleCommand(const FString& Name);

	/**
	 * Removes every command registered on behalf of Owner and returns how many were removed.
	 * Pairs with the registration site - register in BeginPlay, call this in EndPlay.
	 */
	UFUNCTION(BlueprintCallable, Category = "StillCooking|Console")
	int32 UnregisterCommandsFor(const UObject* Owner);

	/** Names this registry currently owns. Dynamic path only - static commands belong to no registry. */
	UFUNCTION(BlueprintPure, Category = "StillCooking|Console")
	TArray<FString> GetRegisteredCommandNames() const;

	/**
	 * Static path entry point, called from a console command stub. Raises OnStaticCommand on the
	 * registry for World. False - with a warning naming the command - when there is no game world,
	 * and in Shipping, where this path does not exist.
	 */
	static bool DispatchStaticCommand(UWorld* World, FName CommandId, const TArray<FString>& Args);

	/** Static path body. One event for every statically registered command - switch on CommandId. */
	UFUNCTION(BlueprintImplementableEvent, Category = "StillCooking|Console")
	void OnStaticCommand(FName CommandId, const TArray<FString>& Args);

	/** Dynamic path entry point. Call RegisterConsoleCommand from here. Not called in Shipping. */
	UFUNCTION(BlueprintImplementableEvent, Category = "StillCooking|Console")
	void OnRegisterCommands();

	//~ Begin UGameInstanceSubsystem Interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End UGameInstanceSubsystem Interface

private:
	void UnregisterAll();

	/** Drops commands whose owner has been destroyed. Runs on every registration, so the table heals itself. */
	void PruneStaleCommands();

	/**
	 * Command name to the object that owns its body. Names rather than console object pointers, and a
	 * weak owner, so the registry neither dangles nor keeps an actor alive. Emptied in Deinitialize.
	 */
	TMap<FString, FWeakObjectPtr> RegisteredCommands;
};
