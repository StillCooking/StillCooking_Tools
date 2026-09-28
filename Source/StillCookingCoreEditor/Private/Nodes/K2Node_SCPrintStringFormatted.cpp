#include "Nodes/K2Node_SCPrintStringFormatted.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_MakeArray.h"
#include "K2Node_MakeStruct.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetTextLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "SCFormatArgumentNames.h"

#define LOCTEXT_NAMESPACE "K2Node_SCPrintStringFormatted"

namespace SCPrintStringFormattedPins
{
	// Pin names are user-visible and, for the fixed pins, part of the node's contract from 0.6.0
	// on - a rename breaks assets that already wire them.
	static const FName Format(TEXT("Format"));
	static const FName PrintToScreen(TEXT("Print To Screen"));
	static const FName PrintToLog(TEXT("Print To Log"));
	static const FName TextColor(TEXT("Text Color"));
	static const FName Duration(TEXT("Duration"));
	static const FName Key(TEXT("Key"));
}

void UK2Node_SCPrintStringFormatted::AllocateDefaultPins()
{
	using namespace SCPrintStringFormattedPins;

	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	UEdGraphPin* FormatPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_String, Format);
	FormatPin->DefaultValue = TEXT("Hello");
	FormatPin->PinToolTip = LOCTEXT("FormatPinTooltip",
		"The string to print. Every {Name} in it grows an input pin called Name.").ToString();

	// Defaults copied from UKismetSystemLibrary::PrintString, which also hides all five behind
	// AdvancedDisplay - so the node opens looking like the engine's.
	UEdGraphPin* PrintToScreenPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, PrintToScreen);
	PrintToScreenPin->DefaultValue = TEXT("true");
	PrintToScreenPin->bAdvancedView = true;

	UEdGraphPin* PrintToLogPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Boolean, PrintToLog);
	PrintToLogPin->DefaultValue = TEXT("true");
	PrintToLogPin->bAdvancedView = true;

	UEdGraphPin* TextColorPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct,
		TBaseStructure<FLinearColor>::Get(), TextColor);
	TextColorPin->DefaultValue = TEXT("(R=0.000000,G=0.660000,B=1.000000,A=1.000000)");
	TextColorPin->bAdvancedView = true;

	UEdGraphPin* DurationPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Real,
		UEdGraphSchema_K2::PC_Double, Duration);
	DurationPin->DefaultValue = TEXT("2.0");
	DurationPin->bAdvancedView = true;

	UEdGraphPin* KeyPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Name, Key);
	KeyPin->DefaultValue = TEXT("None");
	KeyPin->bAdvancedView = true;

	// Restore the argument pins a saved asset carries: without this the node loads with its braces
	// intact and its pins gone.
	for (const FName& ArgumentName : PinNames)
	{
		CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, ArgumentName);
	}

	if (AdvancedPinDisplay == ENodeAdvancedPins::NoPins)
	{
		AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
	}

	Super::AllocateDefaultPins();
}

void UK2Node_SCPrintStringFormatted::PostPlacedNewNode()
{
	Super::PostPlacedNewNode();

	// The node expands to UKismetSystemLibrary::PrintString, which carries meta=(DevelopmentOnly).
	// UK2Node_CallFunction reads that metadata and flips its own enabled state; this node has no
	// target function to read, so it declares the same state directly. The graph editor draws the
	// Development Only banner straight off GetDesiredEnabledState(), and the compiler drops the node
	// from Shipping and Test builds, exactly as it drops the engine's Print String.
	SetEnabledState(ENodeEnabledState::DevelopmentOnly, /*bUserAction=*/ false);
}

void UK2Node_SCPrintStringFormatted::PostLoad()
{
	Super::PostLoad();

	// Fix up a node saved before the state above was set, but never one the author set by hand -
	// HasUserSetTheEnabledState tells the two apart, and stomping a deliberate choice would be worse
	// than a missing banner.
	if (!HasUserSetTheEnabledState() && GetDesiredEnabledState() == ENodeEnabledState::Enabled)
	{
		SetEnabledState(ENodeEnabledState::DevelopmentOnly, /*bUserAction=*/ false);
	}
}

UEdGraphPin* UK2Node_SCPrintStringFormatted::GetFormatPin() const
{
	return FindPinChecked(SCPrintStringFormattedPins::Format, EGPD_Input);
}

UEdGraphPin* UK2Node_SCPrintStringFormatted::FindArgumentPin(const FName ArgumentName) const
{
	UEdGraphPin* const* Found = Pins.FindByPredicate([&ArgumentName](const UEdGraphPin* Pin)
	{
		return Pin->Direction == EGPD_Input && Pin->PinName == ArgumentName;
	});
	return (Found != nullptr && IsArgumentPin(*Found)) ? *Found : nullptr;
}

bool UK2Node_SCPrintStringFormatted::IsArgumentPin(const UEdGraphPin* Pin) const
{
	// Membership in PinNames, not "everything unrecognised": UK2Node_FormatText can treat every
	// non-Format input pin as an argument because it is pure, but this node also owns an exec pin
	// and five option pins, and that rule would delete them.
	return Pin != nullptr && Pin->Direction == EGPD_Input && PinNames.Contains(Pin->PinName);
}

void UK2Node_SCPrintStringFormatted::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	UEdGraphPin* FormatPin = GetFormatPin();
	if (Pin != FormatPin || FormatPin->LinkedTo.Num() > 0)
	{
		return;
	}

	TArray<FName> NewNames;
	SCFormatArgumentNames::Collect(FormatPin->DefaultValue, NewNames);

	for (const FName& ArgumentName : NewNames)
	{
		if (FindArgumentPin(ArgumentName) == nullptr)
		{
			CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, ArgumentName);
		}
	}

	// Remove only argument pins the pattern no longer names - see IsArgumentPin.
	for (auto It = Pins.CreateIterator(); It; ++It)
	{
		UEdGraphPin* CheckPin = *It;
		if (IsArgumentPin(CheckPin) && !NewNames.Contains(CheckPin->PinName))
		{
			CheckPin->MarkAsGarbage();
			It.RemoveCurrent();
		}
	}

	PinNames = MoveTemp(NewNames);

	GetGraph()->NotifyNodeChanged(this);
	FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

void UK2Node_SCPrintStringFormatted::SynchronizeArgumentPinType(UEdGraphPin* Pin)
{
	// An orphaned pin keeps the type it was saved with and is not connectable, so there is nothing
	// to synchronize.
	if (!IsArgumentPin(Pin) || Pin->bOrphanedPin)
	{
		return;
	}

	// Both halves matter: without the reset the pin would keep a stale type and refuse the next,
	// differently typed connection.
	if (Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0] != nullptr)
	{
		Pin->PinType = Pin->LinkedTo[0]->PinType;
	}
	else
	{
		Pin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
		Pin->PinType.PinSubCategory = NAME_None;
		Pin->PinType.PinSubCategoryObject = nullptr;
		Pin->PinType.ContainerType = EPinContainerType::None;
	}
}

void UK2Node_SCPrintStringFormatted::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	SynchronizeArgumentPinType(Pin);
}

void UK2Node_SCPrintStringFormatted::PostReconstructNode()
{
	Super::PostReconstructNode();

	// Reconstruction rebuilds the pins from AllocateDefaultPins, so every argument pin comes back a
	// wildcard. Its links come back by name (see DoPinsMatchForReconstruction) but its type does not:
	// UEdGraphPin::TransferPersistentDataFromOldPin treats PinType as schema-generated, and nothing
	// calls NotifyPinConnectionListChanged during a reconstruction.
	//
	// Not a rare path - FBlueprintEditorUtils::ReconstructAllNodes runs over every node of every
	// Blueprint the compilation manager regenerates on load, so without this pass an argument pin is
	// wildcard-while-wired after each editor start and the node fails to compile until the author
	// replugs it by hand.
	//
	// Re-deriving the type here rather than serializing it is what UK2Node_MakeContainer does, and is
	// why this node reports Low_UsesDependentWildcard: the nodes it borrows types from are
	// reconstructed first.
	for (UEdGraphPin* Pin : Pins)
	{
		SynchronizeArgumentPinType(Pin);
	}
}

bool UK2Node_SCPrintStringFormatted::IsConnectionDisallowed(const UEdGraphPin* MyPin,
	const UEdGraphPin* OtherPin, FString& OutReason) const
{
	// Guarded by IsArgumentPin so the option pins keep their own rules - without it Text Color, a
	// struct, would fall under the argument whitelist and become unconnectable.
	if (IsArgumentPin(MyPin))
	{
		const FName& OtherCategory = OtherPin->PinType.PinCategory;

		bool bIsValidType =
			OtherCategory == UEdGraphSchema_K2::PC_Int ||
			OtherCategory == UEdGraphSchema_K2::PC_Int64 ||
			OtherCategory == UEdGraphSchema_K2::PC_Real ||
			OtherCategory == UEdGraphSchema_K2::PC_Text ||
			OtherCategory == UEdGraphSchema_K2::PC_String ||
			OtherCategory == UEdGraphSchema_K2::PC_Name ||
			OtherCategory == UEdGraphSchema_K2::PC_Boolean ||
			OtherCategory == UEdGraphSchema_K2::PC_Object ||
			OtherCategory == UEdGraphSchema_K2::PC_Wildcard ||
			(OtherCategory == UEdGraphSchema_K2::PC_Byte && !OtherPin->PinType.PinSubCategoryObject.IsValid());

		if (!bIsValidType &&
			(OtherCategory == UEdGraphSchema_K2::PC_Byte || OtherCategory == UEdGraphSchema_K2::PC_Enum))
		{
			static UEnum* TextGenderEnum = FindObjectChecked<UEnum>(nullptr, TEXT("/Script/Engine.ETextGender"),
				EFindObjectFlags::ExactClass);
			bIsValidType = OtherPin->PinType.PinSubCategoryObject == TextGenderEnum;
		}

		if (!bIsValidType)
		{
			OutReason = LOCTEXT("Error_InvalidArgumentType",
				"Format arguments may only be Byte, Integer, Int64, Float, Double, Text, String, Name, Boolean, Object, Wildcard or ETextGender.").ToString();
			return true;
		}
	}

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

UK2Node::ERedirectType UK2Node_SCPrintStringFormatted::DoPinsMatchForReconstruction(const UEdGraphPin* NewPin,
	int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const
{
	// Argument pins are wildcards whose type comes from what they are wired to, so the default
	// type-based matching drops their connections on a reconstruction. Matching them by name is what
	// makes a reopened Blueprint keep its wiring.
	if (IsArgumentPin(OldPin) && NewPin->PinName == OldPin->PinName)
	{
		return ERedirectType_Name;
	}

	return Super::DoPinsMatchForReconstruction(NewPin, NewPinIndex, OldPin, OldPinIndex);
}

FText UK2Node_SCPrintStringFormatted::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "Print String Formatted");
}

FText UK2Node_SCPrintStringFormatted::GetTooltipText() const
{
	return LOCTEXT("NodeTooltip",
		"Prints a string built from a format literal: every {Name} in Format grows an input pin called Name.\n\n"
		"Numeric arguments are formatted using the current culture, so 1000 may print as \"1,000\" or \"1 000\".\n\n"
		"Development only - the call is compiled out of Shipping builds.");
}

void UK2Node_SCPrintStringFormatted::GetMenuActions(FBlueprintActionDatabaseRegistrar& Registrar) const
{
	// IsOpenForRegistration guards against registering the same action twice when the database is
	// rebuilt for a subset of classes.
	UClass* Action = GetClass();
	if (Registrar.IsOpenForRegistration(Action))
	{
		UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create(Action);
		check(Spawner != nullptr);
		Registrar.AddBlueprintAction(Action, Spawner);
	}
}

FText UK2Node_SCPrintStringFormatted::GetMenuCategory() const
{
	return LOCTEXT("MenuCategory", "StillCooking|Debug");
}

namespace
{
	UEdGraphPin* FindSoleOutputPinChecked(UEdGraphNode* Node)
	{
		check(Node != nullptr);
		UEdGraphPin** Found = Node->Pins.FindByPredicate([](const UEdGraphPin* Pin)
		{
			return Pin != nullptr && Pin->Direction == EGPD_Output;
		});
		check(Found != nullptr);
		return *Found;
	}
}

void UK2Node_SCPrintStringFormatted::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	using namespace SCPrintStringFormattedPins;

	Super::ExpandNode(CompilerContext, SourceGraph);

	// The expansion, left to right:
	//
	//   Format (String) -> Conv_StringToText -.
	//                                          +-> KismetTextLibrary::Format -> Conv_TextToString -> PrintString
	//   {Args} -> MakeStruct x N -> MakeArray -
	//
	// After this runs the node itself is gone from the graph; it exists only to wire those together.
	// The detour through FText is why numbers come out formatted by the current culture.

	// One FFormatArgumentData per argument, gathered into the array Format takes.
	UK2Node_MakeArray* MakeArrayNode = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(this, SourceGraph);
	MakeArrayNode->AllocateDefaultPins();
	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeArrayNode, this);
	UEdGraphPin* ArrayOut = MakeArrayNode->GetOutputPin();

	UK2Node_CallFunction* CallFormatFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallFormatFunction->SetFromFunction(UKismetTextLibrary::StaticClass()->FindFunctionByName(
		GET_MEMBER_NAME_CHECKED(UKismetTextLibrary, Format)));
	CallFormatFunction->AllocateDefaultPins();
	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFormatFunction, this);

	ArrayOut->MakeLinkTo(CallFormatFunction->FindPinChecked(TEXT("InArgs")));

	// Settles the array element type, and only works while exactly one pin is connected.
	MakeArrayNode->PinConnectionListChanged(ArrayOut);

	static UScriptStruct* FormatArgumentDataStruct = FindObjectChecked<UScriptStruct>(
		FindObjectChecked<UPackage>(nullptr, TEXT("/Script/Engine")), TEXT("FormatArgumentData"));

	for (int32 ArgIdx = 0; ArgIdx < PinNames.Num(); ++ArgIdx)
	{
		UEdGraphPin* ArgumentPin = FindArgumentPin(PinNames[ArgIdx]);
		if (ArgumentPin == nullptr)
		{
			// PinNames and the pins are kept in step by PinDefaultValueChanged and
			// AllocateDefaultPins; a mismatch means one of those failed, and a compiler
			// error beats dereferencing null.
			CompilerContext.MessageLog.Error(*FText::Format(
				LOCTEXT("Error_MissingArgumentPin", "Print String Formatted: no pin for argument {0}."),
				FText::FromName(PinNames[ArgIdx])).ToString(), this);
			continue;
		}

		UK2Node_MakeStruct* MakeArgument = CompilerContext.SpawnIntermediateNode<UK2Node_MakeStruct>(this, SourceGraph);
		MakeArgument->StructType = FormatArgumentDataStruct;
		MakeArgument->AllocateDefaultPins();
		MakeArgument->bMadeAfterOverridePinRemoval = true;
		CompilerContext.MessageLog.NotifyIntermediateObjectCreation(MakeArgument, this);

		const UEdGraphSchema* StructSchema = MakeArgument->GetSchema();
		UEdGraphPin* ArgumentNamePin = MakeArgument->FindPinChecked(
			GET_MEMBER_NAME_STRING_CHECKED(FFormatArgumentData, ArgumentName));
		UEdGraphPin* ArgumentTypePin = MakeArgument->FindPinChecked(
			GET_MEMBER_NAME_STRING_CHECKED(FFormatArgumentData, ArgumentValueType));
		UEdGraphPin* ArgumentValuePin = MakeArgument->FindPinChecked(
			GET_MEMBER_NAME_STRING_CHECKED(FFormatArgumentData, ArgumentValue));
		UEdGraphPin* ArgumentIntPin = MakeArgument->FindPinChecked(
			GET_MEMBER_NAME_STRING_CHECKED(FFormatArgumentData, ArgumentValueInt));

		StructSchema->TrySetDefaultValue(*ArgumentNamePin, ArgumentPin->PinName.ToString());

		if (ArgumentPin->LinkedTo.Num() > 0)
		{
			const FName& Category = ArgumentPin->PinType.PinCategory;

			// Everything FFormatArgumentData cannot carry natively goes through a ToText node first.
			auto AddConversionNode = [&](const FName FunctionName, const TCHAR* InputPinName)
			{
				StructSchema->TrySetDefaultValue(*ArgumentTypePin, TEXT("Text"));

				UK2Node_CallFunction* ToText = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
				ToText->SetFromFunction(UKismetTextLibrary::StaticClass()->FindFunctionByName(FunctionName));
				ToText->AllocateDefaultPins();
				CompilerContext.MessageLog.NotifyIntermediateObjectCreation(ToText, this);

				CompilerContext.MovePinLinksToIntermediate(*ArgumentPin, *ToText->FindPinChecked(InputPinName));
				ToText->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue)->MakeLinkTo(ArgumentValuePin);
			};

			if (Category == UEdGraphSchema_K2::PC_Int)
			{
				// ArgumentValueInt is an int64, so an int needs a widening cast.
				StructSchema->TrySetDefaultValue(*ArgumentTypePin, TEXT("Int"));

				UK2Node_CallFunction* ToInt64 = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
				ToInt64->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(
					GET_MEMBER_NAME_CHECKED(UKismetMathLibrary, Conv_IntToInt64)));
				ToInt64->AllocateDefaultPins();
				CompilerContext.MessageLog.NotifyIntermediateObjectCreation(ToInt64, this);

				CompilerContext.MovePinLinksToIntermediate(*ArgumentPin, *ToInt64->FindPinChecked(TEXT("InInt")));
				ToInt64->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue)->MakeLinkTo(ArgumentIntPin);
			}
			else if (Category == UEdGraphSchema_K2::PC_Int64)
			{
				StructSchema->TrySetDefaultValue(*ArgumentTypePin, TEXT("Int64"));
				CompilerContext.MovePinLinksToIntermediate(*ArgumentPin, *ArgumentIntPin);
			}
			else if (Category == UEdGraphSchema_K2::PC_Real)
			{
				if (ArgumentPin->PinType.PinSubCategory == UEdGraphSchema_K2::PC_Float)
				{
					StructSchema->TrySetDefaultValue(*ArgumentTypePin, TEXT("Float"));
					CompilerContext.MovePinLinksToIntermediate(*ArgumentPin, *MakeArgument->FindPinChecked(
						GET_MEMBER_NAME_STRING_CHECKED(FFormatArgumentData, ArgumentValueFloat)));
				}
				else
				{
					StructSchema->TrySetDefaultValue(*ArgumentTypePin, TEXT("Double"));
					CompilerContext.MovePinLinksToIntermediate(*ArgumentPin, *MakeArgument->FindPinChecked(
						GET_MEMBER_NAME_STRING_CHECKED(FFormatArgumentData, ArgumentValueDouble)));
				}
			}
			else if (Category == UEdGraphSchema_K2::PC_Text)
			{
				StructSchema->TrySetDefaultValue(*ArgumentTypePin, TEXT("Text"));
				CompilerContext.MovePinLinksToIntermediate(*ArgumentPin, *ArgumentValuePin);
			}
			else if (Category == UEdGraphSchema_K2::PC_Byte && !ArgumentPin->PinType.PinSubCategoryObject.IsValid())
			{
				StructSchema->TrySetDefaultValue(*ArgumentTypePin, TEXT("Int"));

				UK2Node_CallFunction* ByteToInt = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
				ByteToInt->SetFromFunction(UKismetMathLibrary::StaticClass()->FindFunctionByName(
					GET_MEMBER_NAME_CHECKED(UKismetMathLibrary, Conv_ByteToInt64)));
				ByteToInt->AllocateDefaultPins();
				CompilerContext.MessageLog.NotifyIntermediateObjectCreation(ByteToInt, this);

				CompilerContext.MovePinLinksToIntermediate(*ArgumentPin, *ByteToInt->FindPinChecked(TEXT("InByte")));
				ByteToInt->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue)->MakeLinkTo(ArgumentIntPin);
			}
			else if (Category == UEdGraphSchema_K2::PC_Byte || Category == UEdGraphSchema_K2::PC_Enum)
			{
				static UEnum* TextGenderEnum = FindObjectChecked<UEnum>(nullptr, TEXT("/Script/Engine.ETextGender"),
					EFindObjectFlags::ExactClass);
				if (ArgumentPin->PinType.PinSubCategoryObject == TextGenderEnum)
				{
					StructSchema->TrySetDefaultValue(*ArgumentTypePin, TEXT("Gender"));
					CompilerContext.MovePinLinksToIntermediate(*ArgumentPin, *MakeArgument->FindPinChecked(
						GET_MEMBER_NAME_STRING_CHECKED(FFormatArgumentData, ArgumentValueGender)));
				}
			}
			else if (Category == UEdGraphSchema_K2::PC_Boolean)
			{
				AddConversionNode(GET_MEMBER_NAME_CHECKED(UKismetTextLibrary, Conv_BoolToText), TEXT("InBool"));
			}
			else if (Category == UEdGraphSchema_K2::PC_Name)
			{
				AddConversionNode(GET_MEMBER_NAME_CHECKED(UKismetTextLibrary, Conv_NameToText), TEXT("InName"));
			}
			else if (Category == UEdGraphSchema_K2::PC_String)
			{
				AddConversionNode(GET_MEMBER_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText), TEXT("InString"));
			}
			else if (Category == UEdGraphSchema_K2::PC_Object)
			{
				AddConversionNode(GET_MEMBER_NAME_CHECKED(UKismetTextLibrary, Conv_ObjectToText), TEXT("InObj"));
			}
			else
			{
				CompilerContext.MessageLog.Error(*FText::Format(
					LOCTEXT("Error_UnexpectedPinType", "Print String Formatted: pin {0} has an unexpected type: {1}"),
					FText::FromName(PinNames[ArgIdx]), FText::FromName(Category)).ToString(), this);
			}
		}
		else
		{
			// A brace whose pin is left unwired prints as empty, not as the brace itself - the same
			// answer UK2Node_FormatText gives.
			StructSchema->TrySetDefaultValue(*ArgumentTypePin, TEXT("Text"));
			StructSchema->TrySetDefaultText(*ArgumentValuePin, FText::GetEmpty());
		}

		// Make Array starts with one input pin already allocated.
		if (ArgIdx > 0)
		{
			MakeArrayNode->AddInputPin();
		}
		UEdGraphPin* ElementPin = MakeArrayNode->FindPinChecked(FString::Printf(TEXT("[%d]"), ArgIdx));
		FindSoleOutputPinChecked(MakeArgument)->MakeLinkTo(ElementPin);
	}

	// The Format pin is a String and Format takes an FText, so unlike FormatText this node needs a
	// conversion in front of the pattern. MovePinLinksToIntermediate carries the pin's literal as
	// well as any connection.
	UK2Node_CallFunction* PatternToText = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	PatternToText->SetFromFunction(UKismetTextLibrary::StaticClass()->FindFunctionByName(
		GET_MEMBER_NAME_CHECKED(UKismetTextLibrary, Conv_StringToText)));
	PatternToText->AllocateDefaultPins();
	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(PatternToText, this);

	CompilerContext.MovePinLinksToIntermediate(*GetFormatPin(), *PatternToText->FindPinChecked(TEXT("InString")));
	PatternToText->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue)->MakeLinkTo(
		CallFormatFunction->FindPinChecked(TEXT("InPattern")));

	// Back out of FText, because PrintString takes an FString.
	UK2Node_CallFunction* ResultToString = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	ResultToString->SetFromFunction(UKismetTextLibrary::StaticClass()->FindFunctionByName(
		GET_MEMBER_NAME_CHECKED(UKismetTextLibrary, Conv_TextToString)));
	ResultToString->AllocateDefaultPins();
	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(ResultToString, this);

	CallFormatFunction->GetReturnValuePin()->MakeLinkTo(ResultToString->FindPinChecked(TEXT("InText")));

	UK2Node_CallFunction* CallPrintString = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallPrintString->SetFromFunction(UKismetSystemLibrary::StaticClass()->FindFunctionByName(
		GET_MEMBER_NAME_CHECKED(UKismetSystemLibrary, PrintString)));
	CallPrintString->AllocateDefaultPins();
	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallPrintString, this);

	ResultToString->FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue)->MakeLinkTo(
		CallPrintString->FindPinChecked(TEXT("InString")));

	// The option pins carry across by pin object, not by name - ours read "Print To Screen" where
	// the function parameter is "bPrintToScreen".
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(PrintToScreen, EGPD_Input),
		*CallPrintString->FindPinChecked(TEXT("bPrintToScreen")));
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(PrintToLog, EGPD_Input),
		*CallPrintString->FindPinChecked(TEXT("bPrintToLog")));
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(TextColor, EGPD_Input),
		*CallPrintString->FindPinChecked(TEXT("TextColor")));
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(Duration, EGPD_Input),
		*CallPrintString->FindPinChecked(TEXT("Duration")));
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(Key, EGPD_Input),
		*CallPrintString->FindPinChecked(TEXT("Key")));

	// WorldContextObject is deliberately left alone: PrintString carries
	// CallableWithoutWorldContext, so the schema resolves it.
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(UEdGraphSchema_K2::PN_Execute, EGPD_Input),
		*CallPrintString->GetExecPin());
	CompilerContext.MovePinLinksToIntermediate(*FindPinChecked(UEdGraphSchema_K2::PN_Then, EGPD_Output),
		*CallPrintString->GetThenPin());

	BreakAllNodeLinks();
}

#undef LOCTEXT_NAMESPACE
