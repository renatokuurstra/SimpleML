// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

using UnrealBuildTool;

public class SimpleMLTests : ModuleRules
{
    public SimpleMLTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "SimpleML",
            "CQTest"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });
    }
}
