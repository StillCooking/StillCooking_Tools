#include "Flow/SCFlowWaitUntil.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Flow/SCFlowConditionPoll.h"
#include "SCLogChannels.h"

USCFlowWaitUntil* USCFlowWaitUntil::WaitUntil(UObject* WorldContextObject,
	FSCFlowConditionSignature Condition, float PollInterval, float Timeout, bool bCheckImmediately,
	bool bUseUnscaledTime, bool bTickWhenPaused)
{
	USCFlowWaitUntil* Action = NewObject<USCFlowWaitUntil>();

	Action->WorldContext = WorldContextObject;
	Action->Condition = Condition;
	Action->Params.PollInterval = PollInterval;
	Action->Params.Timeout = Timeout;
	Action->Params.bCheckImmediately = bCheckImmediately;
	Action->Params.bUseUnscaledTime = bUseUnscaledTime;
	Action->Params.bTickWhenPaused = bTickWhenPaused;

	// Correct in a real game, but nothing here depends on it: it silently does nothing when the world
	// has no UGameInstance, which is why the core keeps itself alive.
	Action->RegisterWithGameInstance(WorldContextObject);

	return Action;
}

bool USCFlowWaitUntil::EnsureConditionBound()
{
	if (Condition.IsBound())
	{
		return true;
	}

	// Logs once per node in every reachable case: the mid-wait caller Cancels the task right after this
	// returns false, which stops further polling. The gap is the Start()-time immediate check, where
	// Task is not yet assigned and no Cancel() follows - to log twice the condition would have to fall
	// unbound between Activate()'s own check and Start().
	UE_LOG(LogStillCooking, Error,
		TEXT("Wait Until has no callable Condition - the pin was never bound, or the object it was ")
		TEXT("bound to has been destroyed. A condition that can never answer has run out of time at ")
		TEXT("zero, so the node takes On Timed Out. Bind Condition through Create Event to a ")
		TEXT("Blueprint FUNCTION of the shape (int32 Attempt) -> bool; a custom event cannot return ")
		TEXT("a value and will fail to compile."));

	return false;
}

void USCFlowWaitUntil::Activate()
{
	if (bActivated)
	{
		return;
	}
	bActivated = true;

	if (!EnsureConditionBound())
	{
		OnTimedOut.Broadcast();
		SetReadyToDestroy();
		return;
	}

	// ReturnNull, not LogAndReturnNull: FSCFlowTickingTask::BeginRun already logs a clear,
	// node-agnostic error for a null world. Same choice as USCFlowDoForDuration.
	UWorld* World = GEngine != nullptr
		? GEngine->GetWorldFromContextObject(WorldContext.Get(), EGetWorldErrorMode::ReturnNull)
		: nullptr;

	TWeakObjectPtr<USCFlowWaitUntil> WeakThis(this);

	Task = FSCFlowConditionPoll::Start(World, Params,
		[WeakThis](int32 Attempt)
		{
			USCFlowWaitUntil* Self = WeakThis.Get();
			if (Self == nullptr)
			{
				return false;
			}

			if (!Self->EnsureConditionBound())
			{
				// The target died mid-wait; end here rather than poll a dead delegate to the deadline.
				// Cancel() is the only deferred exit the core offers, and this is not a cancellation, so
				// bConditionLost re-labels the resulting Broken as On Timed Out in HandleFinished.
				//
				// The flag is set only where a Cancel() actually follows: with Task unassigned it would
				// stay true with no Broken ever coming, and a later genuine Cancel() would misreport.
				if (Self->Task.IsValid())
				{
					Self->bConditionLost = true;
					Self->Task->Cancel();
				}
				return false;
			}

			return Self->Condition.Execute(Attempt);
		},
		[WeakThis](ESCFlowFinish Reason)
		{
			if (USCFlowWaitUntil* Self = WeakThis.Get())
			{
				Self->HandleFinished(Reason);
			}
		});

	// Must follow Start() rather than be folded into it: finishing inside Start() would run this
	// node's completion path from inside its own construction.
	Task->FlushImmediateCheck();

	if (Task.IsValid() && !Task->IsRunning())
	{
		// Finished already - no world, or satisfied immediately. Drop the handle so the node does
		// not look like it is waiting.
		Task.Reset();
	}
}

void USCFlowWaitUntil::Cancel()
{
	if (Task.IsValid())
	{
		Task->Cancel();
	}
}

void USCFlowWaitUntil::HandleFinished(ESCFlowFinish Reason)
{
	Task.Reset();

	switch (Reason)
	{
	case ESCFlowFinish::Completed:
		OnSatisfied.Broadcast();
		break;

	case ESCFlowFinish::TimedOut:
		OnTimedOut.Broadcast();
		break;

	case ESCFlowFinish::Broken:
		// A wait ended because its condition became uncallable is not a cancellation, whatever exit
		// the core had to use to get there. One rule for both unbound cases - see EnsureConditionBound.
		if (bConditionLost)
		{
			OnTimedOut.Broadcast();
		}
		else
		{
			OnCancelled.Broadcast();
		}
		break;

	case ESCFlowFinish::Aborted:
		// The world is being torn down. Broadcasting a gameplay-facing delegate into it is the
		// classic async-node bug; the node just releases itself instead.
		break;
	}

	SetReadyToDestroy();
}

void USCFlowWaitUntil::BeginDestroy()
{
	// The core holds a self-reference and outlives this node otherwise. Nothing is broadcast from
	// this path: during GC-driven BeginDestroy this object already carries Unreachable, so the weak
	// pointer the callbacks captured resolves to null. Same shape as USCFlowDoForDuration.
	if (Task.IsValid())
	{
		Task->Cancel();
		Task.Reset();
	}

	Super::BeginDestroy();
}
