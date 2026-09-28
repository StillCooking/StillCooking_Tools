#pragma once

#include "CoreMinimal.h"
#include "Flow/SCFlowTickingTask.h"
#include "Flow/SCFlowTypes.h"
#include "UObject/WeakObjectPtr.h"

/**
 * EXPERIMENTAL. The shape of this class is justified by one node so far and may change without a
 * major version bump until a second StillCooking|Flow node validates it; the Blueprint nodes built
 * on top of it are stable.
 *
 * Runs a body once per index, waiting a delay between iterations. The loop holds a shared reference
 * to itself for as long as it runs, so the caller may drop the TSharedRef returned by Start()
 * immediately. The body is never called synchronously from Start(); the first iteration lands on
 * the first tick.
 */
class STILLCOOKINGCORE_API FSCFlowLoop final : public FSCFlowTickingTask
{
public:
	/**
	 * A null world, or a world that dies later, finishes the loop as ESCFlowFinish::Aborted.
	 * Count <= 0 finishes as Completed without a single iteration.
	 */
	static TSharedRef<FSCFlowLoop> Start(UWorld* InWorld, const FSCFlowLoopParams& InParams,
		FSCFlowLoopBody InBody, FSCFlowFinished InFinished);

	FSCFlowLoop(const FSCFlowLoop&) = delete;
	FSCFlowLoop& operator=(const FSCFlowLoop&) = delete;

	/**
	 * Reports a suspended iteration as finished and schedules the next one. Called from inside the
	 * body, the request is recorded and applied once the body returns, so a body that continues
	 * synchronously cannot recurse into the loop.
	 */
	void Continue();

	/** Ends the loop as ESCFlowFinish::Broken. Recorded rather than applied when called from the body. */
	void Break();

private:
	FSCFlowLoop(UWorld* InWorld, const FSCFlowLoopParams& InParams,
		FSCFlowLoopBody InBody, FSCFlowFinished InFinished);

	/**
	 * Runs up to Params.ItemsPerTick iterations back to back, stopping the moment the loop finishes
	 * or parks in Suspend. It spins between body calls and never inside one, so the reentrancy
	 * guards on RunIteration() keep their exact current meaning.
	 */
	void RunBatch();

	/** Runs the body once and applies whatever it - or a reentrant call - asked for. */
	void RunIteration();

	/** Moves to the next index, or finishes as Completed when there is none. */
	void AdvanceOrComplete();

	/**
	 * Accumulates suspended time and logs one warning once it passes the threshold. A parked loop is
	 * measurable at all only because IsTickable() stays true while the loop runs, so a loop waiting
	 * on Continue() keeps receiving ticks; Tick() calls this before its early return on bSuspended.
	 */
	void WarnIfSuspendedTooLong(float DeltaTime);

#pragma region SCFlowTickingTasks
	virtual void OnFinished(ESCFlowFinish Reason) override;
#pragma endregion

#pragma region FTickableGameObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
#pragma endregion

	FSCFlowLoopParams Params;
	FSCFlowLoopBody Body;

	/** Seconds toward the next iteration. Carries the sub-interval remainder; never catches up. */
	float Accumulator = 0.f;

	int32 CurrentIndex = 0;

	/** Set while waiting for an explicit Continue() after a body returned Suspend. */
	bool bSuspended = false;

	/** Seconds spent parked in Suspend. Reset every time the loop resumes, not when it warns. */
	float SuspendedSeconds = 0.f;

	/**
	 * Once per loop, not once per suspension: a loop that parks, warns, resumes and parks again stays
	 * silent the second time. A warning that repeats is a warning people learn to filter out.
	 */
	bool bSuspendWarningIssued = false;

	/** Consumed by the first tick so the first iteration does not wait for the delay. */
	bool bSkipDelayForNextIteration = false;

	/** Reentrancy guard: true for the duration of the body call. */
	bool bInBody = false;
	bool bContinueRequestedDuringBody = false;
	bool bBreakRequestedDuringBody = false;
};
