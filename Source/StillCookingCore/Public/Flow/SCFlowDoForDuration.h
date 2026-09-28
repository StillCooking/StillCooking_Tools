#pragma once

#include "CoreMinimal.h"
#include "Flow/SCFlowDelegates.h"
#include "Flow/SCFlowTypes.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "UObject/WeakObjectPtr.h"
#include "SCFlowDoForDuration.generated.h"

class FSCFlowDuration;
class UCurveFloat;

/**
 * Blueprint node: drives an alpha from 0 to 1 over Duration seconds, firing On Update every frame,
 * then Completed.
 *
 * The last On Update is GUARANTEED to land at the end of the drive - with no curve, at an alpha of
 * exactly 1 - so an interpolation does not stop just short of its destination. With a curve it
 * carries the curve's value at time 1, whatever the curve's author drew there.
 */
UCLASS(BlueprintType, meta = (ExposedAsyncProxy = "Action"))
class STILLCOOKINGCORE_API USCFlowDoForDuration : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** Delta Time is the delta the drive integrated, so a body can integrate against the same clock. */
	UPROPERTY(BlueprintAssignable, Category = "StillCooking|Flow")
	FSCFlowAlphaSignature OnUpdate;

	/**
	 * Fires once, when the drive ends - after Duration or after Cancel. The Blueprint pin cannot
	 * tell the two apart; a C++ handler receives bCompletedFully, false after a Cancel.
	 */
	UPROPERTY(BlueprintAssignable, Category = "StillCooking|Flow")
	FSCFlowCompletedSignature Completed;

	/**
	 * Fires On Update every frame for Duration seconds with an alpha running 0 to 1, then Completed.
	 * The final update is guaranteed to land at the end of the drive. A Duration of zero or less
	 * delivers a single update at alpha 1 and completes immediately.
	 *
	 * Curve, when supplied, shapes the alpha: the drive's raw 0..1 progress is looked up on it and
	 * the result is what On Update receives. An overshooting ease is honoured rather than clamped.
	 *
	 */
	UFUNCTION(BlueprintCallable, Category = "StillCooking|Flow",
		meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true",
			DisplayName = "Do For Duration",
			AdvancedDisplay = "bUseUnscaledTime,bTickWhenPaused"))
	static USCFlowDoForDuration* DoForDuration(UObject* WorldContextObject,
		float Duration = 1.f, UCurveFloat* Curve = nullptr,
		bool bUseUnscaledTime = false, bool bTickWhenPaused = false);

	/**
	 * Ends the drive early and fires Completed - the same pin a full run fires. Only a C++ handler
	 * can tell the difference: it receives bCompletedFully = false.
	 */
	UFUNCTION(BlueprintCallable, Category = "StillCooking|Flow", meta = (DisplayName = "Cancel"))
	void Cancel();

	//~ Begin UBlueprintAsyncActionBase Interface
	virtual void Activate() override;
	//~ End UBlueprintAsyncActionBase Interface

	//~ Begin UObject Interface
	virtual void BeginDestroy() override;
	//~ End UObject Interface

private:
	void HandleFinished(ESCFlowFinish Reason);

	/** Not a UPROPERTY - the core is not a UObject. It keeps itself alive; this is a handle. */
	TSharedPtr<FSCFlowDuration> Drive;

	TWeakObjectPtr<UObject> WorldContext;
	FSCFlowDurationParams Params;

	/**
	 * The core takes a shaper TFunction and knows nothing about assets, so this UPROPERTY is the only
	 * thing keeping the curve alive for the drive's lifetime. The shaper the node builds captures a
	 * weak pointer, so a node collected first degrades the lambda to the identity rather than
	 * dereferencing a dead asset.
	 */
	UPROPERTY()
	TObjectPtr<UCurveFloat> Curve;

	/**
	 * Re-entry guard for Activate(). Deliberately not Drive.IsValid(): a drive that finishes inside
	 * Start() - no world - is never stored, so the handle stays empty on a node that has already run.
	 */
	bool bActivated = false;

	friend struct FSCFlowDurationNodeTestAccess;
};
