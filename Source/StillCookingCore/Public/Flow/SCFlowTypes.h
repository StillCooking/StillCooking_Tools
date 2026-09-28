#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"

/**
 * What a loop body reports when it returns. A synchronous body always returns Continue; a latent
 * body returns Suspend and owes the loop a Continue() or Break() once it is done.
 */
enum class ESCFlowStep : uint8
{
	/** The iteration is finished; run the next one after the delay. */
	Continue,
	/** The iteration is still in flight; the loop waits for an explicit Continue() or Break(). */
	Suspend,
	/** Stop the loop; it finishes as ESCFlowFinish::Broken. */
	Break
};

/** How a StillCooking|Flow task ended - a loop, a duration drive, or a condition poll. */
enum class ESCFlowFinish : uint8
{
	/** Every iteration ran, the drive reached its duration, or the condition returned true. */
	Completed,
	/** Break() or Cancel() was called, or a loop body returned ESCFlowStep::Break. */
	Broken,
	/**
	 * The world went away underneath the task. Nothing gameplay-facing may be broadcast for this
	 * reason - that is how async nodes end up firing at destroyed actors.
	 */
	Aborted,
	/** A deadline expired. Only FSCFlowConditionPoll has one, so only it produces this. */
	TimedOut
};

/** Start-up parameters for FSCFlowLoop. */
struct FSCFlowLoopParams
{
	/** Number of iterations. Zero or less finishes immediately as Completed, without iterating. */
	int32 Count = 0;

	/** Seconds between the end of one iteration and the start of the next. Zero or less means every tick. */
	float Delay = 0.f;

	/** When set, the first iteration also waits a full Delay instead of running on the first tick. */
	bool bDelayBeforeFirstIteration = false;

	/**
	 * Iterations to run in a single tick; values below 1 are treated as 1. A batch ends the moment
	 * the loop finishes or parks, so a latent body - one that returns ESCFlowStep::Suspend - runs at
	 * most one iteration per tick whatever this says. Combining it with a Delay above zero means
	 * "run this many, then wait"; no Blueprint node exposes that combination.
	 */
	int32 ItemsPerTick = 1;
};

/**
 * Called once per iteration with the zero-based index; see ESCFlowStep for the return contract.
 *
 * The body must not synchronously tear down the world the loop runs on. Everything else that can
 * end the loop from inside the body is deferred until it returns, but world cleanup finishes with
 * no deferral - and finishing clears this TFunction while its own operator() is on the stack.
 */
using FSCFlowLoopBody = TFunction<ESCFlowStep(int32 Index)>;

/** Called exactly once, when the task ends, for any reason. */
using FSCFlowFinished = TFunction<void(ESCFlowFinish Reason)>;

/** Start-up parameters for FSCFlowDuration. */
struct FSCFlowDurationParams
{
	/** Seconds the drive lasts. Zero or less delivers one update at alpha 1 and completes. */
	float Duration = 1.f;

	/**
	 * Integrate real time instead of the world's dilated time, so slow motion does not stretch the
	 * drive. Independent of bTickWhenPaused: a task that does not tick while paused has no delta to
	 * rescale, whatever base it asked for.
	 */
	bool bUseUnscaledTime = false;

	/** Keep driving while the game is paused. Without this the drive freezes and resumes on unpause. */
	bool bTickWhenPaused = false;
};

/**
 * Shapes the raw 0..1 progress into the alpha the body is handed. Empty means the identity.
 *
 * A TFunction rather than a UCurveFloat, so the core stays usable from C++ with any easing function
 * and the strong asset reference lives on the Blueprint adapter, where UPROPERTY is available. The
 * result is never clamped: an ease that overshoots past 1 and settles back is legitimate to author.
 */
using FSCFlowAlphaShaper = TFunction<float(float RawAlpha)>;

/**
 * Called every tick of the drive with the shaped alpha and the delta the driver integrated.
 *
 * Unlike FSCFlowLoopBody, nothing here is deferred: cancelling the drive from inside Update -
 * directly, or through a Blueprint node's own Cancel() - clears this TFunction while its own
 * operator() is on the stack. Reading a capture after the statement that cancels is a use-after-free.
 */
using FSCFlowDurationUpdate = TFunction<void(float Alpha, float DeltaTime)>;

/** Start-up parameters for FSCFlowConditionPoll. */
struct FSCFlowWaitUntilParams
{
	/** Seconds between polls. Zero or less polls every tick. */
	float PollInterval = 0.f;

	/** Seconds before the task gives up. Zero or less waits without a deadline. */
	float Timeout = 0.f;

	/** Poll once inside Start(), before the first tick. The answer is not acted on until flushed. */
	bool bCheckImmediately = false;

	/** Integrate real time instead of the world's dilated time. See FSCFlowDurationParams. */
	bool bUseUnscaledTime = false;

	/** Keep polling while the game is paused. Without this the task freezes and resumes on unpause. */
	bool bTickWhenPaused = false;
};

/**
 * Answers "are we there yet" for a zero-based attempt number. Returning true ends the wait.
 *
 * A cancel raised from inside this function is deferred until the call has returned, so the closure
 * is never freed while its own operator() is on the stack - the opposite of FSCFlowDurationUpdate.
 * World teardown is the one route that is not covered, as it is for FSCFlowLoopBody.
 */
using FSCFlowCondition = TFunction<bool(int32 Attempt)>;
