#pragma once

#include "CoreMinimal.h"
#include "Flow/SCFlowTickingTask.h"
#include "Flow/SCFlowTypes.h"

/**
 * EXPERIMENTAL, like the rest of the StillCooking|Flow C++ layer; the Blueprint node built on it is
 * stable.
 *
 * Polls a condition every PollInterval until it returns true, and gives up on a hard deadline. The
 * deadline is checked BEFORE the poll, so a condition satisfied on the frame the timeout lands does
 * not win: the timeout is meant to be a limit, not a suggestion.
 */
class STILLCOOKINGCORE_API FSCFlowConditionPoll final : public FSCFlowTickingTask
{
public:
	/** A null world, or a world that dies later, finishes the wait as ESCFlowFinish::Aborted. */
	static TSharedRef<FSCFlowConditionPoll> Start(UWorld* InWorld, const FSCFlowWaitUntilParams& InParams,
		FSCFlowCondition InCondition, FSCFlowFinished InFinished);

	/** Ends the wait early; it finishes as ESCFlowFinish::Broken. Idempotent. */
	void Cancel();

	/**
	 * Acts on the answer bCheckImmediately collected inside Start(); a no-op when it is false. Call
	 * this once, immediately after Start() has returned. Skipping it does not merely postpone the
	 * answer: a cancel raised by the Start()-time condition stays pending, so the first tick asks
	 * the condition one more question before Poll() honours it.
	 */
	void FlushImmediateCheck();

private:
	FSCFlowConditionPoll(UWorld* InWorld, const FSCFlowWaitUntilParams& InParams,
		FSCFlowCondition InCondition, FSCFlowFinished InFinished);

	/** Runs the condition and finishes as Completed when it answers true. */
	void Poll();

	/** One warning per instance when a task with no deadline has waited past the CVar threshold. */
	void WarnIfWaitingTooLong();

	//~ Begin FSCFlowTickingTask Interface
	virtual void OnFinished(ESCFlowFinish Reason) override;
	//~ End FSCFlowTickingTask Interface

	//~ Begin FTickableGameObject Interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickableWhenPaused() const override { return Params.bTickWhenPaused; }
	virtual TStatId GetStatId() const override;
	//~ End FTickableGameObject Interface

	FSCFlowWaitUntilParams Params;
	FSCFlowCondition Condition;

	/** Set while the condition is on the stack. Guards bCancelRequested below. */
	bool bInCondition = false;

	/** Cancel() arrived while the condition was running; honoured once it returns. */
	bool bCancelRequested = false;

	/** The answer bCheckImmediately collected in Start(), unset when there was no immediate check. */
	TOptional<bool> PendingImmediateResult;

	/** Zero-based, handed to the condition and incremented once per poll. */
	int32 Attempt = 0;

	/** Seconds since the last poll. Carries its sub-interval remainder so polling does not drift. */
	float Accumulator = 0.f;

	/** Seconds since the task started. Feeds the deadline check and the silent-wait warning. */
	float Elapsed = 0.f;

	bool bWaitWarningIssued = false;
};
