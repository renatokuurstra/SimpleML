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
            "UEcs",
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "CoreUObject",
            "Engine",
        });

        // Expose Eigen headers publicly so other modules depending on SimpleML can include them
        string ThirdPartyDir = Path.Combine(ModuleDirectory, "..", "..", "ThirdParty");
        string EigenIncludeDir = Path.GetFullPath(Path.Combine(ThirdPartyDir, "Eigen"));

        if (Directory.Exists(EigenIncludeDir))
        {
            PublicIncludePaths.Add(EigenIncludeDir);
            PublicSystemIncludePaths.Add(EigenIncludeDir);
        }
        else
        {
            // Still add the path so that once user drops headers, no Build.cs change is needed
            PublicIncludePaths.Add(EigenIncludeDir);
            PublicSystemIncludePaths.Add(EigenIncludeDir);
        }

        // Safer defaults for using Eigen with MSVC/Unreal
        PublicDefinitions.AddRange(new string[]
        {
            "NOMINMAX",
            "EIGEN_MPL2_ONLY=1",
#if UE_BUILD_SHIPPING
            "EIGEN_NO_DEBUG",
#endif
            // Avoid potential alignment issues with custom allocators / MSVC under Unreal
            "EIGEN_DONT_ALIGN_STATICALLY=1",
            "EIGEN_MAX_ALIGN_BYTES=0"
        });
    }
}
