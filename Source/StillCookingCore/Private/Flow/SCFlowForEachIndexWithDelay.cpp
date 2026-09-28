#include "Flow/SCFlowForEachIndexWithDelay.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Flow/SCFlowLoop.h"

USCFlowForEachIndexWithDelay* USCFlowForEachIndexWithDelay::ForEachIndexWithDelay(
	UObject* WorldContextObject, int32 Count, float Delay, bool bDelayBeforeFirstIteration,
	bool bAutoContinue)
{
	USCFlowForEachIndexWithDelay* Action = NewObject<USCFlowForEachIndexWithDelay>();
	Action->WorldContext = WorldContextObject;
	Action->Params.Count = Count;
	Action->Params.Delay = Delay;
	Action->Params.bDelayBeforeFirstIteration = bDelayBeforeFirstIteration;
	Action->bAutoContinue = bAutoContinue;
	
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

USCFlowForEachIndexWithDelay* USCFlowForEachIndexWithDelay::ForEachIndexPerTick(
	UObject* WorldContextObject, int32 Count, int32 ItemsPerTick, bool bAutoContinue)
{
	USCFlowForEachIndexWithDelay* Action = NewObject<USCFlowForEachIndexWithDelay>();
	Action->WorldContext = WorldContextObject;
	Action->Params.Count = Count;

	// Delay 0 is not an approximation of "next tick" - it is the branch in FSCFlowLoop::Tick that runs
	// a batch every tick with no accumulator at all. bDelayBeforeFirstIteration is a no-op there.
	Action->Params.Delay = 0.f;
	Action->Params.bDelayBeforeFirstIteration = false;
	Action->Params.ItemsPerTick = FMath::Max(1, ItemsPerTick);
	Action->bAutoContinue = bAutoContinue;

	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void USCFlowForEachIndexWithDelay::Activate()
{
	// Starting here rather than in the factory is load-bearing: the K2Node expansion binds the
	// delegates after the factory returns, so a loop that finished there would broadcast to nobody.
	if (bActivated)
	{
		return;
	}
	bActivated = true;

	UWorld* World = GEngine->GetWorldFromContextObject(
		WorldContext.Get(), EGetWorldErrorMode::LogAndReturnNull);

	TWeakObjectPtr<USCFlowForEachIndexWithDelay> WeakThis(this);

	TSharedRef<FSCFlowLoop> Started = FSCFlowLoop::Start(World, Params,
		[WeakThis](int32 Index)
		{
			USCFlowForEachIndexWithDelay* Strong = WeakThis.Get();
			if (Strong == nullptr)
			{
				return ESCFlowStep::Break;
			}

			Strong->LoopBody.Broadcast(Index);

			// A dynamic multicast delegate has no return value, so the graph cannot answer the
			// loop and the adapter answers for it. A Continue that arrives during the broadcast
			// is reconciled by the core's reentrancy guard, so auto mode plus a still-wired
			// Continue Loop advances one iteration, not two.
			return Strong->bAutoContinue ? ESCFlowStep::Continue : ESCFlowStep::Suspend;
		},
		[WeakThis](ESCFlowFinish Reason)
		{
			if (USCFlowForEachIndexWithDelay* Strong = WeakThis.Get())
			{
				Strong->HandleFinished(Reason);
			}
		});

	// Start() finishes inside itself for Count <= 0 and for a null world, so storing that corpse would
	// make Loop.IsValid() read as "still going".
	if (Started->IsRunning())
	{
		Loop = Started;
	}
}

void USCFlowForEachIndexWithDelay::BeginDestroy()
{
	// A loop parked on Continue() early-returns from Tick() and so never reaches the stale-weak-pointer
	// branch in the body: breaking here is what releases the core, its self-reference and its
	// tickable-registry slot. Nothing is broadcast from this path: during GC-driven BeginDestroy this
	// object already carries EInternalObjectFlags::Unreachable, so the TWeakObjectPtr the callbacks
	// captured resolves to null and HandleFinished is never entered. Same shape as
	// UCancellableAsyncAction::BeginDestroy, which calls Cancel() for the same reason.
	if (Loop.IsValid())
	{
		Loop->Break();
	}

	Super::BeginDestroy();
}

void USCFlowForEachIndexWithDelay::Continue()
{
	if (Loop.IsValid())
	{
		Loop->Continue();
	}
}

void USCFlowForEachIndexWithDelay::Break()
{
	if (Loop.IsValid())
	{
		Loop->Break();
	}
}

void USCFlowForEachIndexWithDelay::HandleFinished(ESCFlowFinish Reason)
{
	Loop.Reset();

	// Aborted means the world is being torn down. Broadcasting into it is how async nodes end up
	// firing at destroyed actors, so this path releases the action silently.
	if (Reason != ESCFlowFinish::Aborted)
	{
		Completed.Broadcast(Reason == ESCFlowFinish::Completed);
	}

	SetReadyToDestroy();
}
