#pragma once

#include "CoreMinimal.h"
#include "Internationalization/Text.h"

/**
 * Argument names referenced by a format pattern, for nodes that grow a pin per brace. Kept out of
 * the node because this is the only part of the feature headless automation can reach - a UK2Node's
 * pins exist only inside an editor. Header-only so the test module can use it without an export
 * macro on StillCookingCoreEditor.
 */
namespace SCFormatArgumentNames
{
	/**
	 * Collects the argument names a format pattern references, in first-appearance order.
	 *
	 * @param Pattern   The format literal, e.g. TEXT("{Index} of {Count}").
	 * @param OutNames  Reset first, then filled. Empty for a pattern that references no arguments,
	 *                  including a half-typed one - the engine yields nothing until the brace closes.
	 */
	inline void Collect(const FString& Pattern, TArray<FName>& OutNames)
	{
		// Load-bearing: the engine APPENDS, and returns early without touching the array when the
		// pattern compiles to anything but a Complex expression - so a pattern that lost its last
		// brace would otherwise keep the names from the previous keystroke.
		OutNames.Reset();

		TArray<FString> Parameters;
		FText::GetFormatPatternParameters(FTextFormat::FromString(Pattern), Parameters);

		OutNames.Reserve(Parameters.Num());
		for (const FString& Parameter : Parameters)
		{
			// AddUnique is not redundant with the engine's own de-duplication: that one is
			// case-SENSITIVE (TextFormatter.cpp), so it hands back both {Name} and {name}. FName
			// compares case-insensitively and a graph cannot hold two pins differing only in case,
			// so the variants collapse here, keeping the first spelling as UK2Node_FormatText does.
			// Consequence, inherited from that node: only the kept spelling gets an argument and the
			// other brace prints literally, because FFormatArgumentData::ArgumentName is an FString
			// matched case-sensitively.
			OutNames.AddUnique(FName(*Parameter));
		}
	}
}
