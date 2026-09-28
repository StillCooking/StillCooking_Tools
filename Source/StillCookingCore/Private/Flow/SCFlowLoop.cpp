#include "Flow/SCFlowLoop.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "SCLogChannels.h"
#include "Stats/Stats.h"

namespace
{
	/**
	 * How long a loop may sit in Suspend before it says so, once. Zero or less disables the warning.
	 *
	 * 30 seconds is a heuristic: a latent body may legitimately stay open for an asset load or a long
	 * Move Component To, so the number sits an order of magnitude above a plausible iteration rather
	 * than at the edge of one, and the warning names the variable to raise.
	 */
	float GSCFlowSuspendedLoopWarningSeconds = 30.f;

	FAutoConsoleVariableRef CVarSCFlowSuspendedLoopWarningSeconds(
		TEXT("SC.Flow.SuspendedLoopWarningSeconds"),
		GSCFlowSuspendedLoopWarningSeconds,
		TEXT("Seconds a suspended StillCooking|Flow loop may wait for Continue() before it logs one "
			 "warning. Zero or less disables the warning."));
}

FSCFlowLoop::FSCFlowLoop(UWorld* InWorld, const FSCFlowLoopParams& InParams,
	FSCFlowLoopBody InBody, FSCFlowFinished InFinished)
	: FSCFlowTickingTask(InWorld)
	, Params(InParams)
	, Body(MoveTemp(InBody))
{
	Finished = MoveTemp(InFinished);
}

TSharedRef<FSCFlowLoop> FSCFlowLoop::Start(UWorld* InWorld, const FSCFlowLoopParams& InParams,
	FSCFlowLoopBody InBody, FSCFlowFinished InFinished)
{
	TSharedRef<FSCFlowLoop> Loop = MakeShareable(
		new FSCFlowLoop(InWorld, InParams, MoveTemp(InBody), MoveTemp(InFinished)));

	if (InWorld == nullptr)
	{
		Loop->BeginRun(Loop);
		return Loop;
	}

	if (InParams.Count <= 0)
	{
		Loop->Finish(ESCFlowFinish::Completed);
		return Loop;
	}

	if (!Loop->BeginRun(Loop))
	{
		return Loop;
	}

	Loop->bSkipDelayForNextIteration = !InParams.bDelayBeforeFirstIteration;

	return Loop;
}

void FSCFlowLoop::OnFinished(ESCFlowFinish Reason)
{
	bSuspended = false;
	Body = nullptr;
}

void FSCFlowLoop::RunBatch()
{
	const int32 Budget = FMath::Max(1, Params.ItemsPerTick);

	for (int32 Ran = 0; Ran < Budget; ++Ran)
	{
		RunIteration();

		// Finished or parked on Continue() - the batch is over either way; only the parked one resumes.
		if (bFinished || bSuspended)
		{
			return;
		}
	}
}

void FSCFlowLoop::RunIteration()
{
	bInBody = true;
	bContinueRequestedDuringBody = false;
	bBreakRequestedDuringBody = false;

	const ESCFlowStep Step = Body ? Body(CurrentIndex) : ESCFlowStep::Break;

	bInBody = false;

	if (Step == ESCFlowStep::Break || bBreakRequestedDuringBody)
	{
		Finish(ESCFlowFinish::Broken);
		return;
	}

	if (Step == ESCFlowStep::Continue || bContinueRequestedDuringBody)
	{
		AdvanceOrComplete();
		return;
	}

	bSuspended = true;
}

void FSCFlowLoop::AdvanceOrComplete()
{
	++CurrentIndex;
	bSuspended = false;
	SuspendedSeconds = 0.f;

	// Accumulator is deliberately not reset here or on resume: the carried remainder is delay time
	// accrued before this iteration started, and discarding it would make the loop drift. A latent
	// body's own duration never lands in it - Tick() early-returns while bSuspended is set.

	if (CurrentIndex >= Params.Count)
	{
		Finish(ESCFlowFinish::Completed);
	}
}

void FSCFlowLoop::Continue()
{
	// KEEPALIVE RULE - see the field comment on SelfWhileRunning; AdvanceOrComplete() can finish the loop.
	TSharedPtr<FSCFlowTickingTask> KeepAlive = SelfWhileRunning;

	if (bFinished)
	{
		return;
	}

	if (bInBody)
	{
		// Recorded, not applied: RunIteration() reconciles it once the body returns. Applying it here
		// would call the body from inside the body.
		bContinueRequestedDuringBody = true;
		return;
	}

	if (!bSuspended)
	{
		// Nothing is waiting on us - the loop is between iterations, counting down the delay.
		return;
	}

	AdvanceOrComplete();
}

void FSCFlowLoop::Break()
{
	// KEEPALIVE RULE - see the field comment on SelfWhileRunning.
	TSharedPtr<FSCFlowTickingTask> KeepAlive = SelfWhileRunning;

	if (bFinished)
	{
		return;
	}

	if (bInBody)
	{
		bBreakRequestedDuringBody = true;
		return;
	}

	Finish(ESCFlowFinish::Broken);
}

void FSCFlowLoop::WarnIfSuspendedTooLong(float DeltaTime)
{
	if (bSuspendWarningIssued || GSCFlowSuspendedLoopWarningSeconds <= 0.f)
	{
		return;
	}

	// Deliberately not Accumulator. Suspension time is not delay time - see the comment in
	// AdvanceOrComplete() - and adding it there would make the next iteration fire early.
	SuspendedSeconds += DeltaTime;
	if (SuspendedSeconds < GSCFlowSuspendedLoopWarningSeconds)
	{
		return;
	}

	bSuspendWarningIssued = true;
	
	UE_LOG(LogStillCooking, Warning,
		TEXT("FSCFlowLoop has been suspended at index %d of %d for %.0f seconds with no Continue() ")
		TEXT("(Blueprint: 'Continue Loop'). The loop is waiting for that call, not stuck, and resumes ")
		TEXT("the moment it arrives. Adjust SC.Flow.SuspendedLoopWarningSeconds, or set it to 0, if a ")
		TEXT("body this long is expected here."),
		CurrentIndex, Params.Count, SuspendedSeconds);
}

void FSCFlowLoop::Tick(float DeltaTime)
{
	// KEEPALIVE RULE - see the field comment on SelfWhileRunning.
	TSharedPtr<FSCFlowTickingTask> KeepAlive = SelfWhileRunning;

	// Ahead of the early return below. Finish() clears bSuspended, so a finished loop cannot get here.
	if (bSuspended)
	{
		WarnIfSuspendedTooLong(DeltaTime);
	}

	if (bFinished || bSuspended)
	{
		return;
	}

	if (bSkipDelayForNextIteration)
	{
		bSkipDelayForNextIteration = false;
		Accumulator = 0.f;
		RunBatch();
		return;
	}

	if (Params.Delay <= 0.f)
	{
		RunBatch();
		return;
	}

	Accumulator += DeltaTime;
	if (Accumulator < Params.Delay)
	{
		return;
	}

	// Carry the sub-interval remainder rather than resetting, so the loop does not drift, and run one
	// batch however long the hitch was. Same arithmetic as USCTickableObject.
	Accumulator = FMath::Fmod(Accumulator, Params.Delay);
	RunBatch();
}

TStatId FSCFlowLoop::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FSCFlowLoop, STATGROUP_Tickables);
}
