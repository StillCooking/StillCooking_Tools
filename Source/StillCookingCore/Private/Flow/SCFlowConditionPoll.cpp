#include "Flow/SCFlowConditionPoll.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/App.h"
#include "SCLogChannels.h"
#include "Stats/Stats.h"

namespace
{
	/**
	 * How long a Wait Until with no deadline may poll before it says so, once. Zero or less disables it.
	 *
	 * 30 seconds, matching SC.Flow.SuspendedLoopWarningSeconds: any threshold responsive enough to be
	 * useful eventually accuses a legitimate long wait, so the warning names the variable to raise.
	 */
	float GSCFlowWaitUntilWarningSeconds = 30.f;

	FAutoConsoleVariableRef CVarSCFlowWaitUntilWarningSeconds(
		TEXT("SC.Flow.WaitUntilWarningSeconds"),
		GSCFlowWaitUntilWarningSeconds,
		TEXT("Seconds a StillCooking|Flow Wait Until with no Timeout may poll before it logs one "
			 "warning. Zero or less disables the warning."));
}

FSCFlowConditionPoll::FSCFlowConditionPoll(UWorld* InWorld, const FSCFlowWaitUntilParams& InParams,
	FSCFlowCondition InCondition, FSCFlowFinished InFinished)
	: FSCFlowTickingTask(InWorld)
	, Params(InParams)
	, Condition(MoveTemp(InCondition))
{
	Finished = MoveTemp(InFinished);
}

TSharedRef<FSCFlowConditionPoll> FSCFlowConditionPoll::Start(UWorld* InWorld,
	const FSCFlowWaitUntilParams& InParams, FSCFlowCondition InCondition, FSCFlowFinished InFinished)
{
	TSharedRef<FSCFlowConditionPoll> Task = MakeShareable(
		new FSCFlowConditionPoll(InWorld, InParams, MoveTemp(InCondition), MoveTemp(InFinished)));

	Task->BeginRun(Task);

	// Polled here, acted on in FlushImmediateCheck(): finishing inside Start() would run the caller's
	// completion path from inside its own construction, before it even holds the reference.
	if (InParams.bCheckImmediately && Task->IsRunning())
	{
		Task->bInCondition = true;
		Task->PendingImmediateResult = Task->Condition ? Task->Condition(Task->Attempt++) : false;
		Task->bInCondition = false;
	}

	return Task;
}

void FSCFlowConditionPoll::Cancel()
{
	// KEEPALIVE RULE - see FSCFlowTickingTask::SelfWhileRunning.
	TSharedPtr<FSCFlowTickingTask> KeepAlive = SelfWhileRunning;

	if (bFinished)
	{
		return;
	}

	// Deferred, not finished: Finish() clears the Condition TFunction, and doing that while the
	// closure's own operator() is on the stack frees it out from under itself.
	if (bInCondition)
	{
		bCancelRequested = true;
		return;
	}

	Finish(ESCFlowFinish::Broken);
}

void FSCFlowConditionPoll::FlushImmediateCheck()
{
	// KEEPALIVE RULE - see FSCFlowTickingTask::SelfWhileRunning.
	TSharedPtr<FSCFlowTickingTask> KeepAlive = SelfWhileRunning;

	if (bFinished || !PendingImmediateResult.IsSet())
	{
		return;
	}

	const bool bSatisfied = PendingImmediateResult.GetValue();
	PendingImmediateResult.Reset();

	if (bCancelRequested)
	{
		bCancelRequested = false;
		Finish(ESCFlowFinish::Broken);
		return;
	}

	if (bSatisfied)
	{
		Finish(ESCFlowFinish::Completed);
	}
}

void FSCFlowConditionPoll::OnFinished(ESCFlowFinish Reason)
{
	Condition = nullptr;
}

void FSCFlowConditionPoll::Poll()
{
	bInCondition = true;
	const bool bSatisfied = Condition ? Condition(Attempt++) : false;
	bInCondition = false;

	// A cancel raised from inside the condition wins over the condition's own answer: the caller
	// decided to stop while being asked, and that decision is later than the answer.
	if (bCancelRequested)
	{
		bCancelRequested = false;
		Finish(ESCFlowFinish::Broken);
		return;
	}

	if (bSatisfied)
	{
		Finish(ESCFlowFinish::Completed);
	}
}

void FSCFlowConditionPoll::WarnIfWaitingTooLong()
{
	// A task with a deadline is not silent - it ends itself and reports why. Nothing to warn about.
	if (bWaitWarningIssued || Params.Timeout > 0.f || GSCFlowWaitUntilWarningSeconds <= 0.f)
	{
		return;
	}

	if (Elapsed < GSCFlowWaitUntilWarningSeconds)
	{
		return;
	}

	bWaitWarningIssued = true;

	// Names the Blueprint pin as well as the C++ type, on purpose: whoever reads this line in PIE is
	// holding a graph, and "Params.Timeout" alone will not lead them to the pin they left at zero.
	UE_LOG(LogStillCooking, Warning,
		TEXT("FSCFlowConditionPoll has polled %d times over %.0f seconds without its condition ")
		TEXT("returning true, and has no deadline to end it (Blueprint: the 'Timeout' pin is 0 or ")
		TEXT("less). It is waiting, not stuck. Set a Timeout, or adjust SC.Flow.WaitUntilWarningSeconds ")
		TEXT("- set it to 0 - if a wait this long is expected here."),
		Attempt, Elapsed);
}

void FSCFlowConditionPoll::Tick(float DeltaTime)
{
	// KEEPALIVE RULE - see FSCFlowTickingTask::SelfWhileRunning. Finish() below clears it.
	TSharedPtr<FSCFlowTickingTask> KeepAlive = SelfWhileRunning;

	if (bFinished)
	{
		return;
	}

	// Same clock selection as FSCFlowDuration::Tick.
	const float Delta = Params.bUseUnscaledTime ? FApp::GetDeltaTime() : DeltaTime;

	// The deadline is tested before the poll gate, not after it: below it the timeout would only be a
	// suggestion, since a condition answering true on the deadline frame would win.
	Elapsed += Delta;
	if (Params.Timeout > 0.f && Elapsed >= Params.Timeout)
	{
		Finish(ESCFlowFinish::TimedOut);
		return;
	}

	WarnIfWaitingTooLong();

	if (Params.PollInterval <= 0.f)
	{
		Poll();
		return;
	}

	Accumulator += Delta;
	if (Accumulator < Params.PollInterval)
	{
		return;
	}

	// Carry the sub-interval remainder rather than resetting, so polling does not drift, and poll
	// exactly once however long the hitch was. Same arithmetic as FSCFlowLoop::Tick().
	Accumulator = FMath::Fmod(Accumulator, Params.PollInterval);
	Poll();
}

TStatId FSCFlowConditionPoll::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FSCFlowConditionPoll, STATGROUP_Tickables);
}
