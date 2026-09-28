#pragma once

#include "CoreMinimal.h"
#include "SCFlowDelegates.generated.h"

/**
 * The dynamic delegate types the StillCooking|Flow Blueprint nodes use. Most are multicast and run
 * outward, broadcast by a node onto an exec pin; FSCFlowConditionSignature runs the other way -
 * single-cast with a return value, bound by the graph and called by C++ to ask it a question.
 *
 * They live in a shared header even where only one node uses a type, so that no node includes a
 * sibling's header just to reach a delegate.
 *
 * These are Blueprint-visible types. Renaming one, or changing a parameter, breaks every graph that
 * binds to it and needs a major version bump.
 */

/** Fires once per iteration with the zero-based index. */
UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCFlowIndexSignature, int32, Index);

/** Fires once when a Flow node ends. False means it was stopped early rather than running out. */
UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSCFlowCompletedSignature, bool, bCompletedFully);

/** Fires every frame of a drive with the current alpha and the delta the drive integrated. */
UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSCFlowAlphaSignature, float, Alpha, float, DeltaTime);

/**
 * Asks the graph whether a wait is over, for a zero-based attempt number.
 *
 * The binding target must be a Blueprint FUNCTION returning bool. A custom event cannot declare a
 * return value: it connects to the pin without protest and then fails compilation with
 * "Create Event Signature Error".
 */
UDELEGATE()
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FSCFlowConditionSignature, int32, Attempt);

/** Fires once when a Flow node reaches one of its distinguishable endings. */
UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSCFlowSignalSignature);
