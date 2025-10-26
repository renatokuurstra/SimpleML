using UnrealBuildTool;

public class GeneticAlgorithm : ModuleRules
{
    public GeneticAlgorithm(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "UEcs"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });
    }
}
