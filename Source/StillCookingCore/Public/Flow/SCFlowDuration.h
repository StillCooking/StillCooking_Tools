#pragma once

#include "CoreMinimal.h"
#include "Flow/SCFlowTickingTask.h"
#include "Flow/SCFlowTypes.h"

/**
 * EXPERIMENTAL, like the rest of the StillCooking|Flow C++ layer; the Blueprint node built on it is
 * stable.
 *
 * Drives a normalised alpha from 0 to 1 over a duration, calling an update every tick, and
 * guarantees a final update at raw progress exactly 1 - the hand-written version of this loses its
 * last frame and leaves the interpolation short of its destination.
 *
 * The guarantee attaches to RAW progress, not to the delivered alpha: with a shaper set, the last
 * update carries Shape(1), including an overshoot that never returns to 1.
 */
class STILLCOOKINGCORE_API FSCFlowDuration final : public FSCFlowTickingTask
{
public:
	/**
	 * Starts a drive on the given world. A null world, or a world that dies later, finishes it as
	 * ESCFlowFinish::Aborted. An empty Shaper means the alpha is the raw progress.
	 *
	 * The update is never called synchronously from Start(); the first one lands on the first tick,
	 * and therefore never at alpha 0.
	 */
	static TSharedRef<FSCFlowDuration> Start(UWorld* InWorld, const FSCFlowDurationParams& InParams,
		FSCFlowDurationUpdate InUpdate, FSCFlowAlphaShaper InShaper, FSCFlowFinished InFinished);

	/** Ends the drive early; it finishes as ESCFlowFinish::Broken. Idempotent. */
	void Cancel();

private:
	FSCFlowDuration(UWorld* InWorld, const FSCFlowDurationParams& InParams,
		FSCFlowDurationUpdate InUpdate, FSCFlowAlphaShaper InShaper, FSCFlowFinished InFinished);

	/** Never clamps the result - see FSCFlowAlphaShaper. */
	float Shape(float RawAlpha) const;

#pragma region FSCFlowTickingTask Interface
	virtual void OnFinished(ESCFlowFinish Reason) override;
#pragma endregion

#pragma region FTickableGameObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickableWhenPaused() const override { return Params.bTickWhenPaused; }
	virtual TStatId GetStatId() const override;
#pragma endregion

	FSCFlowDurationParams Params;
	FSCFlowDurationUpdate Update;
	FSCFlowAlphaShaper Shaper;

	/** Seconds integrated so far. Never reset; the drive runs once. */
	float Elapsed = 0.f;
};
