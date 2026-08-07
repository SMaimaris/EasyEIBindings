// Copyright Stylianos Maimaris. All Rights Reserved.

using UnrealBuildTool;

public class EasyEIBindingsEditor : ModuleRules
{
	public EasyEIBindingsEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AssetTools",
				"BlueprintGraph",
				"ContentBrowser",
				"EasyEIBindings",
				"EnhancedInput",
				"InputBlueprintNodes",
				"InputCore",
				"PropertyEditor",
				"Slate",
				"SlateCore",
				"UnrealEd",
			}
		);
	}
}
