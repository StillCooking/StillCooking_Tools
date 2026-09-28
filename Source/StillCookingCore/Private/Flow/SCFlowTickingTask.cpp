#include "Flow/SCFlowTickingTask.h"

#include "Engine/World.h"
#include "SCLogChannels.h"

FSCFlowTickingTask::FSCFlowTickingTask(UWorld* InWorld)
	: World(InWorld)
{
}

FSCFlowTickingTask::~FSCFlowTickingTask()
{
	if (WorldCleanupHandle.IsValid())
	{
		FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
		WorldCleanupHandle.Reset();
	}
}

bool FSCFlowTickingTask::BeginRun(const TSharedRef<FSCFlowTickingTask>& Self)
{
	if (!World.IsValid())
	{
		UE_LOG(LogStillCooking, Error, TEXT("A StillCooking|Flow task was started without a world; it is aborted."));
		Finish(ESCFlowFinish::Aborted);
		return false;
	}

	SelfWhileRunning = Self;
	bRunning = true;
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddRaw(
		&Self.Get(), &FSCFlowTickingTask::HandleWorldCleanup);

	return true;
}

void FSCFlowTickingTask::Finish(ESCFlowFinish Reason)
{
	if (bFinished)
	{
		return;
	}

	bFinished = true;
	bRunning = false;

	// KEEPALIVE RULE - see the field comment. Covers the window between clearing the field and returning.
	TSharedPtr<FSCFlowTickingTask> KeepAlive = SelfWhileRunning;
	SelfWhileRunning.Reset();

	if (WorldCleanupHandle.IsValid())
	{
		FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
		WorldCleanupHandle.Reset();
	}

	OnFinished(Reason);

	FSCFlowFinished FinishedCallback = MoveTemp(Finished);

	if (FinishedCallback)
	{
		FinishedCallback(Reason);
	}
}

void FSCFlowTickingTask::HandleWorldCleanup(UWorld* CleanedWorld, bool bSessionEnded, bool bCleanupResources)
{
	if (CleanedWorld != World.Get())
	{
		return;
	}

	// KEEPALIVE RULE - see the field comment on SelfWhileRunning.
	TSharedPtr<FSCFlowTickingTask> KeepAlive = SelfWhileRunning;

	// Aborted, not Broken: nobody may broadcast a gameplay-facing callback into a world being torn
	// down. The adapter reads this and releases itself silently.
	Finish(ESCFlowFinish::Aborted);
}
