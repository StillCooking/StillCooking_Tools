using UnrealBuildTool;

public class StillCookingCoreEditor : ModuleRules
{
	public StillCookingCoreEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// UncookedOnly module: it exists only where Blueprints are compiled, and
		// nothing links against it, so every dependency is Private. The one Public
		// header here is inline, so it needs no export macro either.
		// BlueprintGraph - UK2Node and the K2 schema.
		// KismetCompiler - FKismetCompilerContext, used by ExpandNode.
		// UnrealEd       - Kismet2/BlueprintEditorUtils.h, Kismet2/CompilerResultsLog.h.
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"BlueprintGraph",
			"KismetCompiler",
			"UnrealEd"
		});
	}
}
