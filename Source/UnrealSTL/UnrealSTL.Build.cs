// Copyright 2022, Roberto De Ioris.

using UnrealBuildTool;

public class UnrealSTL : ModuleRules
{
    public UnrealSTL(ReadOnlyTargetRules Target) : base(Target)
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
                "RenderCore",
                "RHI"
            }
            );
    }
}
