//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

using UnrealBuildTool;

public class SplineCircuitTrainerTests : ModuleRules
{
    public SplineCircuitTrainerTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UEcs",
            "SplineCircuitTrainer",
            "SimpleML",
            "SimpleMLInterfaces",
            "CQTest"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });
    }
}
