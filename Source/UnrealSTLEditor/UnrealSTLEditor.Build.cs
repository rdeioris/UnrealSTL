// Copyright 2022, Roberto De Ioris.

using UnrealBuildTool;

public class UnrealSTLEditor : ModuleRules
{
    public UnrealSTLEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "UnrealSTL",
                "UnrealEd",
                "MeshDescription",
                "StaticMeshDescription"
            }
            );
    }
}
