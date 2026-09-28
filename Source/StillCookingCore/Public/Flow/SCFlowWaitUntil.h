#pragma once

#include "CoreMinimal.h"
#include "Flow/SCFlowDelegates.h"
#include "Flow/SCFlowTypes.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "UObject/WeakObjectPtr.h"
#include "SCFlowWaitUntil.generated.h"

class FSCFlowConditionPoll;

/**
 * Blueprint node: polls Condition every Poll Interval until it returns true, then fires On
 * Satisfied. Gives up on Timeout with On Timed Out, or on Cancel with On Cancelled. Placeable on an
 * event graph only, not inside a function graph.
 *
 * The node hands itself out on an "Action" output pin. That pin is not decoration: Cancel is called
 * on this object, and without ExposedAsyncProxy the K2Node builds no object pin at all, leaving the
 * graph holding no reference to the thing it has to call.
 */
UCLASS(BlueprintType, meta = (ExposedAsyncProxy = "Action"))
class STILLCOOKINGCORE_API USCFlowWaitUntil : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "StillCooking|Flow")
	FSCFlowSignalSignature OnSatisfied;

	/** Also fires when Condition becomes uncallable: a question nobody can answer is out of time at zero. */
	UPROPERTY(BlueprintAssignable, Category = "StillCooking|Flow")
	FSCFlowSignalSignature OnTimedOut;

	/** Fires once, when Cancel ends the wait. Not fired when the world is torn down. */
	UPROPERTY(BlueprintAssignable, Category = "StillCooking|Flow")
	FSCFlowSignalSignature OnCancelled;

	/**
	 * Polls Condition every Poll Interval until it returns true, then fires On Satisfied.
	 *
	 * Condition must be bound through Create Event to a Blueprint FUNCTION of the shape
	 * (int32 Attempt) -> bool, Attempt being zero-based. A custom event will connect and then fail
	 * to compile, because an event cannot declare a return value.
	 *
	 * Timeout of zero or less waits without a deadline - and a wait that long logs one warning
	 * naming SC.Flow.WaitUntilWarningSeconds. Check Immediately polls once before the first tick, so
	 * a condition that is already true fires On Satisfied in the same frame. Use Unscaled Time
	 * ignores time dilation; Tick When Paused keeps polling while the game is paused, and without it
	 * the deadline stops advancing along with the node.
	 *
	 * If Poll Interval is at least Timeout, the deadline lands before the first poll is due, so the
	 * condition is never asked at all and the node reports On Timed Out.
	 */
	UFUNCTION(BlueprintCallable, Category = "StillCooking|Flow",
		meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true",
			DisplayName = "Wait Until",
			AdvancedDisplay = "bUseUnscaledTime,bTickWhenPaused"))
	static USCFlowWaitUntil* WaitUntil(UObject* WorldContextObject,
		FSCFlowConditionSignature Condition,
		float PollInterval = 0.1f, float Timeout = 0.f, bool bCheckImmediately = false,
		bool bUseUnscaledTime = false, bool bTickWhenPaused = false);

	/** Ends the wait early; On Cancelled fires. */
	UFUNCTION(BlueprintCallable, Category = "StillCooking|Flow", meta = (DisplayName = "Cancel"))
	void Cancel();

#pragma region UBlueprintAsyncActionBase
	virtual void Activate() override;
#pragma endregion

#pragma region UObjectInterface
	virtual void BeginDestroy() override;
#pragma endregion

private:
	void HandleFinished(ESCFlowFinish Reason);

	/** True while Condition is bound to something callable; logs once and gives up when it is not. */
	bool EnsureConditionBound();

	/** Not a UPROPERTY - the core is not a UObject. It keeps itself alive; this is a handle. */
	TSharedPtr<FSCFlowConditionPoll> Task;

	/**
	 * A dynamic delegate holds a WEAK reference to its target, so this can fall unbound mid-wait when
	 * the bound actor is destroyed - hence EnsureConditionBound() on every call, not only at Activate().
	 */
	UPROPERTY()
	FSCFlowConditionSignature Condition;

	TWeakObjectPtr<UObject> WorldContext;
	FSCFlowWaitUntilParams Params;

	/** Re-entry guard for Activate(). Deliberately not Task.IsValid() - see USCFlowDoForDuration. */
	bool bActivated = false;

	/**
	 * The condition became uncallable mid-wait. The core's only deferred exit is Cancel(), which
	 * finishes as Broken; this re-labels that ending as On Timed Out, because nobody cancelled
	 * anything - the question simply stopped being answerable.
	 */
	bool bConditionLost = false;

	friend struct FSCFlowWaitUntilNodeTestAccess;
};
