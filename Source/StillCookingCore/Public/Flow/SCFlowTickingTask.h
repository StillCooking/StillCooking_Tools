#pragma once

#include "CoreMinimal.h"
#include "Flow/SCFlowTypes.h"
#include "Tickable.h"
#include "UObject/WeakObjectPtr.h"

/**
 * EXPERIMENTAL. The shape of this class may change without a major version bump until the
 * StillCooking|Flow family has settled; the Blueprint nodes built on it are stable.
 *
 * The part every Flow core has in common: a world it is bound to, a self-reference that keeps it
 * alive while it runs, and exactly one way to end. Subclasses add what they are actually about -
 * iteration and delay for FSCFlowLoop, elapsed time and an alpha for FSCFlowDuration.
 */
class STILLCOOKINGCORE_API FSCFlowTickingTask : public FTickableGameObject
{
public:
	virtual ~FSCFlowTickingTask() override;

	FSCFlowTickingTask(const FSCFlowTickingTask&) = delete;
	FSCFlowTickingTask& operator=(const FSCFlowTickingTask&) = delete;

	bool IsRunning() const { return bRunning; }

protected:
	explicit FSCFlowTickingTask(UWorld* InWorld);

	/**
	 * Binds the task to its world, takes the self-reference and starts it running. Returns false
	 * when there is no world, having logged and finished the task as Aborted - the caller should
	 * return immediately.
	 */
	bool BeginRun(const TSharedRef<FSCFlowTickingTask>& Self);

	/** The single exit point. Idempotent: the second call does nothing. */
	void Finish(ESCFlowFinish Reason);

	/**
	 * Called from Finish() exactly once, after bRunning and the self-reference have been cleared
	 * and before the finished callback runs. Subclasses release whatever they own - a body or an
	 * update TFunction - so nothing outlives the task.
	 */
	virtual void OnFinished(ESCFlowFinish Reason) {}

	/** Moved out and cleared by Finish() before it is invoked. */
	FSCFlowFinished Finished;

	TWeakObjectPtr<UWorld> World;

	/**
	 * THE KEEPALIVE RULE. This is what keeps the task alive; Finish() clears it, which is very often
	 * the last reference in existence. Never clear it - directly or by calling Finish() - without a
	 * local TSharedPtr copy taken first, or the object dies in the middle of its own method. Every
	 * entry point that can reach Finish() (Tick, HandleWorldCleanup, and whatever the subclass adds
	 * - Continue, Break, Cancel) takes that copy before the first branch that can reach it.
	 */
	TSharedPtr<FSCFlowTickingTask> SelfWhileRunning;

	bool bRunning = false;
	bool bFinished = false;

#pragma region FTickableGameObject Interface
	virtual bool IsTickable() const override { return bRunning && World.IsValid(); }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
	/** Subclasses that want to run in a paused game override this; the default matches the engine's. */
	virtual bool IsTickableWhenPaused() const override { return false; }
	virtual UWorld* GetTickableGameObjectWorld() const override { return World.Get(); }
#pragma endregion

private:
	void HandleWorldCleanup(UWorld* CleanedWorld, bool bSessionEnded, bool bCleanupResources);

	FDelegateHandle WorldCleanupHandle;
};
