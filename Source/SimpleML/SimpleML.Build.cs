// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
using UnrealBuildTool;
using System.IO;

public class SimpleML : ModuleRules
{
    public SimpleML(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "CoreUObject",
            "Engine",
        });

        // Expose Eigen headers publicly so other modules depending on SimpleML can include them
        string ThirdPartyDir = Path.Combine(ModuleDirectory, "..", "..", "ThirdParty");
        string EigenIncludeDir = Path.GetFullPath(Path.Combine(ThirdPartyDir, "Eigen", "include"));

        if (Directory.Exists(EigenIncludeDir))
        {
            PublicIncludePaths.Add(EigenIncludeDir);
            PublicSystemIncludePaths.Add(EigenIncludeDir);
            bEnableUndefinedIdentifierWarnings = false; // Eigen uses heavy templates; keep warnings manageable
        }
        else
        {
            // Still add the path so that once user drops headers, no Build.cs change is needed
            PublicIncludePaths.Add(EigenIncludeDir);
            PublicSystemIncludePaths.Add(EigenIncludeDir);
        }
    }
}
