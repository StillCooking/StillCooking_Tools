#pragma once

#include "CoreMinimal.h"
#include "Flow/SCFlowDelegates.h"
#include "Flow/SCFlowTypes.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "UObject/WeakObjectPtr.h"
#include "SCFlowForEachIndexWithDelay.generated.h"

class FSCFlowLoop;

/**
 * Blueprint node: runs its body once per index from 0 to Count - 1, waiting Delay seconds between
 * iterations.
 *
 * With Auto Continue set - the default - the loop moves on by itself as soon as the body broadcast
 * returns. Clear it when the body is latent: the loop then waits for an explicit Continue. Break
 * ends the loop early either way; Completed fires as for a full run, and only a C++ handler sees
 * bCompletedFully = false.
 *
 * The node hands itself out on a "Loop" output pin. That pin is not decoration: Continue and Break
 * are called on this object, and without ExposedAsyncProxy the K2Node builds no object pin at all.
 * BlueprintType lets the pin be promoted to a variable, which is how Break gets called from an event
 * other than the loop body.
 */
UCLASS(BlueprintType, meta = (ExposedAsyncProxy = "Loop"))
class STILLCOOKINGCORE_API USCFlowForEachIndexWithDelay : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** Fires once per index, zero-based. Call Continue Loop from it only when Auto Continue is cleared. */
	UPROPERTY(BlueprintAssignable, Category = "StillCooking|Flow")
	FSCFlowIndexSignature LoopBody;

	/**
	 * Fires once, when the loop ends - after the last index or after Break Loop. The Blueprint pin
	 * cannot tell the two apart; a C++ handler receives bCompletedFully, false after a Break.
	 */
	UPROPERTY(BlueprintAssignable, Category = "StillCooking|Flow")
	FSCFlowCompletedSignature Completed;

	/**
	 * Runs LoopBody for every index from 0 to Count - 1, waiting Delay seconds between iterations.
	 * A Count of zero or less completes immediately without a single iteration.
	 *
	 * With bAutoContinue set - the default - the loop moves on by itself as soon as the LoopBody
	 * broadcast returns, so a synchronous body needs no Continue Loop at all. Clear it when the body
	 * is latent (it contains a Delay, a Move Component To, an asset load): the loop then waits for an
	 * explicit Continue Loop, and a body that never sends one parks the loop for good.
	 */
	UFUNCTION(BlueprintCallable, Category = "StillCooking|Flow",
		meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true",
			DisplayName = "For Each Index With Delay", AdvancedDisplay = "bAutoContinue"))
	static USCFlowForEachIndexWithDelay* ForEachIndexWithDelay(UObject* WorldContextObject,
		int32 Count, float Delay = 0.2f, bool bDelayBeforeFirstIteration = false,
		bool bAutoContinue = true);

	/**
	 * Runs LoopBody for every index from 0 to Count - 1, spread across frames: ItemsPerTick
	 * iterations run per tick and the next batch waits for the next tick. A Count of zero or less
	 * completes immediately without a single iteration. An ItemsPerTick below 1 is treated as 1.
	 *
	 * bAutoContinue works as in For Each Index With Delay, except that a suspended body ends the
	 * current batch, so ItemsPerTick has no effect once it is cleared.
	 */
	UFUNCTION(BlueprintCallable, Category = "StillCooking|Flow",
		meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true",
			DisplayName = "For Each Index Per Tick", AdvancedDisplay = "bAutoContinue"))
	static USCFlowForEachIndexWithDelay* ForEachIndexPerTick(UObject* WorldContextObject,
		int32 Count, int32 ItemsPerTick = 1, bool bAutoContinue = true);

	/**
	 * Reports the current iteration as finished and schedules the next one. Does nothing when the
	 * loop is not waiting for it - in Auto Continue mode, or mid-delay between iterations.
	 */
	UFUNCTION(BlueprintCallable, Category = "StillCooking|Flow", meta = (DisplayName = "Continue Loop"))
	void Continue();

	/**
	 * Ends the loop early and fires Completed - the same pin a full run fires. Only a C++ handler
	 * can tell the difference: it receives bCompletedFully = false.
	 */
	UFUNCTION(BlueprintCallable, Category = "StillCooking|Flow", meta = (DisplayName = "Break Loop"))
	void Break();

	//~ Begin UBlueprintAsyncActionBase Interface
	virtual void Activate() override;
	//~ End UBlueprintAsyncActionBase Interface

	//~ Begin UObject Interface
	virtual void BeginDestroy() override;
	//~ End UObject Interface

private:
	void HandleFinished(ESCFlowFinish Reason);

	/** Not a UPROPERTY - the core is not a UObject. It keeps itself alive; this is a handle. */
	TSharedPtr<FSCFlowLoop> Loop;

	TWeakObjectPtr<UObject> WorldContext;
	FSCFlowLoopParams Params;

	/**
	 * Deliberately here and not in FSCFlowLoopParams. The core serves synchronous and latent bodies
	 * through one return value and carries no mode flag; the Blueprint asymmetry - a dynamic
	 * multicast delegate cannot return anything - is paid for once, in this adapter.
	 */
	bool bAutoContinue = true;

	/**
	 * Re-entry guard for Activate(). Deliberately not Loop.IsValid(): a loop that finishes inside
	 * Start() - Count <= 0, or no world - is never stored, so the handle stays empty on a node that
	 * has already run.
	 */
	bool bActivated = false;

	friend struct FSCFlowNodeTestAccess;
};
