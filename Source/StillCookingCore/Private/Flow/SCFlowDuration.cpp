#include "Flow/SCFlowDuration.h"

#include "Engine/World.h"
#include "Misc/App.h"
#include "Stats/Stats.h"

FSCFlowDuration::FSCFlowDuration(UWorld* InWorld, const FSCFlowDurationParams& InParams,
	FSCFlowDurationUpdate InUpdate, FSCFlowAlphaShaper InShaper, FSCFlowFinished InFinished)
	: FSCFlowTickingTask(InWorld)
	, Params(InParams)
	, Update(MoveTemp(InUpdate))
	, Shaper(MoveTemp(InShaper))
{
	Finished = MoveTemp(InFinished);
}

TSharedRef<FSCFlowDuration> FSCFlowDuration::Start(UWorld* InWorld, const FSCFlowDurationParams& InParams,
	FSCFlowDurationUpdate InUpdate, FSCFlowAlphaShaper InShaper, FSCFlowFinished InFinished)
{
	TSharedRef<FSCFlowDuration> Drive = MakeShareable(
		new FSCFlowDuration(InWorld, InParams, MoveTemp(InUpdate), MoveTemp(InShaper), MoveTemp(InFinished)));

	// No Duration <= 0 shortcut here, unlike FSCFlowLoop's Count <= 0: that case still owes the body one
	// update at alpha 1, and a body is never called from Start(). Tick() handles it on the first frame.
	Drive->BeginRun(Drive);

	return Drive;
}

void FSCFlowDuration::Cancel()
{
	// KEEPALIVE RULE - see FSCFlowTickingTask::SelfWhileRunning.
	TSharedPtr<FSCFlowTickingTask> KeepAlive = SelfWhileRunning;

	if (bFinished)
	{
		return;
	}

	Finish(ESCFlowFinish::Broken);
}

float FSCFlowDuration::Shape(float RawAlpha) const
{
	return Shaper ? Shaper(RawAlpha) : RawAlpha;
}

void FSCFlowDuration::OnFinished(ESCFlowFinish Reason)
{
	Update = nullptr;
	Shaper = nullptr;
}

void FSCFlowDuration::Tick(float DeltaTime)
{
	// KEEPALIVE RULE - see FSCFlowTickingTask::SelfWhileRunning. Finish() below clears it.
	TSharedPtr<FSCFlowTickingTask> KeepAlive = SelfWhileRunning;

	if (bFinished)
	{
		return;
	}

	// The engine hands a tickable the world's dilated delta; FApp::GetDeltaTime() is the undilated
	// wall-clock one. Whichever is integrated is also what the body is handed - never one and the other.
	const float Delta = Params.bUseUnscaledTime ? FApp::GetDeltaTime() : DeltaTime;

	Elapsed += Delta;

	const float RawAlpha = (Params.Duration > 0.f)
		? FMath::Clamp(Elapsed / Params.Duration, 0.f, 1.f)
		: 1.f;

	// The update goes out before the completion test, so the frame that reaches or overshoots the
	// duration is still delivered, at alpha exactly 1. Finishing first - the obvious-looking early
	// return - drops that last frame and leaves the interpolation short of its destination.
	if (Update)
	{
		Update(Shape(RawAlpha), Delta);
	}

	if (RawAlpha >= 1.f)
	{
		Finish(ESCFlowFinish::Completed);
	}
}

TStatId FSCFlowDuration::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FSCFlowDuration, STATGROUP_Tickables);
}
