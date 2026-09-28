#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "UObject/Object.h"
#include "UObject/WeakObjectPtr.h"
#include "SCTickableObject.generated.h"

/**
 * Base class for a UObject that ticks once per frame.
 *
 * Ownership contract: construct with an outer whose chain resolves a UWorld, hold the object in a
 * UPROPERTY(TObjectPtr<>) (it never roots itself), call Initialize() after construction and
 * Shutdown() before dropping the reference - Shutdown() is the only path that runs subclass teardown.
 *
 * Extend by overriding ReceiveTick_Implementation in C++, or the Tick event in Blueprint.
 *
 * @see https://stillcooking.dev/en/topics/unreal-engine/gameplay-framework/tickable-uobject/
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class STILLCOOKINGCORE_API USCTickableObject : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	USCTickableObject();

	/** Starting tick intent. EnableTick()/DisableTick() overwrite it at any time; Shutdown() restores it. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "StillCooking|Tickable")
	bool bStartTickingOnInitialize = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "StillCooking|Tickable")
	bool bTickWhenPaused = false;

	/** Checked by the engine before the paused and game-world rules, so it applies regardless of bTickWhenPaused. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "StillCooking|Tickable")
	bool bTickInEditor = false;

	/** Resolves the world, marks the object initialized and applies the tick intent. Never call from a constructor. */
	UFUNCTION(BlueprintCallable, Category = "StillCooking|Tickable")
	void Initialize();

	/** Stops ticking, runs ReceiveShutdown and clears the initialized state. Idempotent. */
	UFUNCTION(BlueprintCallable, Category = "StillCooking|Tickable")
	void Shutdown();

	UFUNCTION(BlueprintPure, Category = "StillCooking|Tickable")
	bool IsInitialized() const { return bInitialized; }

	/** Takes effect at the next tick pass. Before Initialize() it only records the intent. */
	UFUNCTION(BlueprintCallable, Category = "StillCooking|Tickable")
	void EnableTick();

	/** Unregisters from the tickable array rather than leaving the object polled every frame. */
	UFUNCTION(BlueprintCallable, Category = "StillCooking|Tickable")
	void DisableTick();

	UFUNCTION(BlueprintPure, Category = "StillCooking|Tickable")
	bool IsTickEnabled() const { return bTickEnabled; }

	/** Seconds between ticks; negative values are clamped to 0. The only supported way to change the interval. */
	UFUNCTION(BlueprintCallable, Category = "StillCooking|Tickable")
	void SetTickInterval(float NewTickInterval);

	UFUNCTION(BlueprintPure, Category = "StillCooking|Tickable")
	float GetTickInterval() const { return TickInterval; }

	/** The cached world after Initialize(), the outer chain's world before it. */
	virtual UWorld* GetWorld() const override;

#if WITH_EDITOR
	/**
	 * Keeps Blueprint subclasses from growing a forced World Context pin. Returns true directly because
	 * UObject::ImplementsGetWorld() probes the CDO, where CachedWorld is empty.
	 */
	virtual bool ImplementsGetWorld() const override { return true; }
#endif

protected:
	/** DeltaTime is the whole interval just consumed, not the wall-clock gap since the previous call. */
	UFUNCTION(BlueprintNativeEvent, DisplayName = "Tick", Category = "StillCooking|Tickable")
	void ReceiveTick(float DeltaTime);
	virtual void ReceiveTick_Implementation(float DeltaTime);

	UFUNCTION(BlueprintNativeEvent, DisplayName = "Initialize", Category = "StillCooking|Tickable")
	void ReceiveInitialize();
	virtual void ReceiveInitialize_Implementation();

	UFUNCTION(BlueprintNativeEvent, DisplayName = "Shutdown", Category = "StillCooking|Tickable")
	void ReceiveShutdown();
	virtual void ReceiveShutdown_Implementation();

	// The engine calls the interface below through UObject / FTickableGameObject pointers, so keeping it
	// off this class's public surface costs nothing.

#pragma region UObject Interface
	//~ Begin UObject Interface
	virtual void PostInitProperties() override;

	/** Stops ticking and reports an owner that never called Shutdown(). Does NOT run subclass teardown. */
	virtual void BeginDestroy() override;
	//~ End UObject Interface
#pragma endregion

#pragma region FTickableGameObject
	//~ Begin FTickableGameObject Interface
	virtual void Tick(float DeltaTime) override final;
	virtual ETickableTickType GetTickableTickType() const override;

	/**
	 * Final: this is where the dead-world guard lives, and an override that forgets Super would put the
	 * object back on the global tick pass with a torn-down world. Gate ticking with EnableTick() instead.
	 */
	virtual bool IsTickable() const override final;

	virtual bool IsTickableWhenPaused() const override final { return bTickWhenPaused; }
	virtual bool IsTickableInEditor() const override final { return bTickInEditor; }
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual TStatId GetStatId() const override;
	//~ End FTickableGameObject Interface
#pragma endregion

private:
	/** Registers or unregisters for ticking to match bTickEnabled. Only meaningful once initialized. */
	void ApplyTickIntent();

	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	/** 0 means every frame. Private so a direct write cannot skip the accumulator restart in SetTickInterval(). */
	UPROPERTY(EditDefaultsOnly, Category = "StillCooking|Tickable", meta = (ClampMin = "0.0", Units = "s"))
	float TickInterval = 0.f;

	TWeakObjectPtr<UWorld> CachedWorld;
	FDelegateHandle WorldCleanupHandle;
	float TimeSinceLastTick = 0.f;
	bool bInitialized = false;
	bool bTickEnabled = false;
};
