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
                "InputCore",
                "EnhancedInput"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "PropertyEditor",
                "ContentBrowser",
                "AssetTools",
                "UnrealEd",
                "EasyEIBindings",
                "BlueprintGraph",
                "Kismet",
                "GraphEditor",
                "InputBlueprintNodes",
                "InputBlueprintNodes"
            }
        );
    }
}