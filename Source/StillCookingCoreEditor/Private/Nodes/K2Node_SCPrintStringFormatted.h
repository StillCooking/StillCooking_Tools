#pragma once

#include "CoreMinimal.h"
#include "K2Node.h"
#include "K2Node_SCPrintStringFormatted.generated.h"

class FBlueprintActionDatabaseRegistrar;
class FKismetCompilerContext;
class UEdGraph;
class UEdGraphPin;

/**
 * Print String whose argument pins are generated from the braces in its format literal: type
 * "{Index}" and the node grows a wildcard pin called Index.
 *
 * Modelled on UK2Node_FormatText rather than derived from it - that class is UCLASS(MinimalAPI), so
 * it exports neither its constructor nor its virtuals and a subclass in another module does not
 * link. Kept in Private because its contract runs through the graph and asset serialization rather
 * than through a header; reflection finds it by path either way.
 *
 * @note Numeric arguments are formatted by the current culture, so 1000 may print as "1,000" or
 *       "1 000".
 */
UCLASS()
class UK2Node_SCPrintStringFormatted : public UK2Node
{
	GENERATED_BODY()

public:
	//~ Begin UEdGraphNode
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual void PostPlacedNewNode() override;
	virtual void PostLoad() override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	//~ End UEdGraphNode

	//~ Begin UK2Node
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& Registrar) const override;
	virtual FText GetMenuCategory() const override;
	virtual bool IsNodePure() const override { return false; }
	virtual bool NodeCausesStructuralBlueprintChange() const override { return true; }
	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const override;
	virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex,
		const UEdGraphPin* OldPin, int32 OldPinIndex) const override;
	virtual int32 GetNodeRefreshPriority() const override { return EBaseNodeRefreshPriority::Low_UsesDependentWildcard; }
	virtual void PostReconstructNode() override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	//~ End UK2Node

	/** The format literal. Input, String, and the pin whose default value grows the argument pins. */
	UEdGraphPin* GetFormatPin() const;

	/** The argument pin carrying this name, or nullptr when the pattern does not reference it. */
	UEdGraphPin* FindArgumentPin(const FName ArgumentName) const;

private:
	bool IsArgumentPin(const UEdGraphPin* Pin) const;

	/**
	 * Gives an argument pin the type of whatever it is wired to, and takes it back to wildcard when
	 * it is wired to nothing. Does nothing to any other pin.
	 */
	void SynchronizeArgumentPinType(UEdGraphPin* Pin);

	/**
	 * Argument names in pin order, serialized so the argument pins survive a reload and a
	 * reconstruction. Written into every consumer Blueprint that uses the node: renaming it, or
	 * renaming this class, orphans the node in assets that already exist.
	 */
	UPROPERTY()
	TArray<FName> PinNames;
};
