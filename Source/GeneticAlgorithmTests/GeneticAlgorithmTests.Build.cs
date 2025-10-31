// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

using UnrealBuildTool;

public class GeneticAlgorithmTests : ModuleRules
{
    public GeneticAlgorithmTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UEcs",
            "GeneticAlgorithm"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });
    }
}
