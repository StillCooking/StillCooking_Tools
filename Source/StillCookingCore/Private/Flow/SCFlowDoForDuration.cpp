#include "Flow/SCFlowDoForDuration.h"

#include "Curves/CurveFloat.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Flow/SCFlowDuration.h"

USCFlowDoForDuration* USCFlowDoForDuration::DoForDuration(UObject* WorldContextObject,
	float Duration, UCurveFloat* Curve, bool bUseUnscaledTime, bool bTickWhenPaused)
{
	USCFlowDoForDuration* Action = NewObject<USCFlowDoForDuration>();

	Action->WorldContext = WorldContextObject;
	Action->Curve = Curve;
	Action->Params.Duration = Duration;
	Action->Params.bUseUnscaledTime = bUseUnscaledTime;
	Action->Params.bTickWhenPaused = bTickWhenPaused;

	// Correct in a real game, but nothing here depends on it: it silently does nothing when the world
	// has no UGameInstance, which is why the core keeps itself alive.
	Action->RegisterWithGameInstance(WorldContextObject);

	return Action;
}

void USCFlowDoForDuration::Activate()
{
	if (bActivated)
	{
		return;
	}
	bActivated = true;

	// ReturnNull, not LogAndReturnNull: FSCFlowTickingTask::BeginRun already logs this case, and a
	// second engine-generated error would make the null-world test depend on an engine log string.
	UWorld* World = GEngine != nullptr
		? GEngine->GetWorldFromContextObject(WorldContext.Get(), EGetWorldErrorMode::ReturnNull)
		: nullptr;

	TWeakObjectPtr<UCurveFloat> WeakCurve = Curve;

	FSCFlowAlphaShaper Shaper;
	if (WeakCurve.IsValid())
	{
		// Weak on purpose: the UPROPERTY above is what keeps the asset alive, and if this node is
		// collected while the core outlives it by a frame, the lambda must not dereference a corpse.
		Shaper = [WeakCurve](float RawAlpha)
		{
			const UCurveFloat* Resolved = WeakCurve.Get();
			return Resolved != nullptr ? Resolved->GetFloatValue(RawAlpha) : RawAlpha;
		};
	}

	TWeakObjectPtr<USCFlowDoForDuration> WeakThis(this);

	Drive = FSCFlowDuration::Start(World, Params,
		[WeakThis](float Alpha, float DeltaTime)
		{
			if (USCFlowDoForDuration* Self = WeakThis.Get())
			{
				Self->OnUpdate.Broadcast(Alpha, DeltaTime);
			}
		},
		MoveTemp(Shaper),
		[WeakThis](ESCFlowFinish Reason)
		{
			if (USCFlowDoForDuration* Self = WeakThis.Get())
			{
				Self->HandleFinished(Reason);
			}
		});

	if (!Drive->IsRunning())
	{
		// Finished inside Start() - no world. Drop the handle so the node does not look like it is driving.
		Drive.Reset();
	}
}

void USCFlowDoForDuration::Cancel()
{
	if (Drive.IsValid())
	{
		Drive->Cancel();
	}
}

void USCFlowDoForDuration::HandleFinished(ESCFlowFinish Reason)
{
	Drive.Reset();

	// Aborted means the world is being torn down. Broadcasting a gameplay-facing delegate into it is
	// the classic async-node bug; the node just releases itself instead.
	if (Reason == ESCFlowFinish::Aborted)
	{
		SetReadyToDestroy();
		return;
	}

	Completed.Broadcast(Reason == ESCFlowFinish::Completed);
	SetReadyToDestroy();
}

void USCFlowDoForDuration::BeginDestroy()
{
	// The core holds a self-reference, so without this it keeps ticking for the rest of its Duration
	// with nobody listening. Nothing is broadcast from this path: during GC-driven BeginDestroy this
	// object already carries EInternalObjectFlags::Unreachable, so the TWeakObjectPtr the callbacks
	// captured resolves to null and HandleFinished is never entered. Same shape as
	// UCancellableAsyncAction::BeginDestroy, which calls Cancel() for the same reason.
	if (Drive.IsValid())
	{
		Drive->Cancel();
		Drive.Reset();
	}

	Super::BeginDestroy();
}
