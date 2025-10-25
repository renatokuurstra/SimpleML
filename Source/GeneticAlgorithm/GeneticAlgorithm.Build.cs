using UnrealBuildTool;

public class GeneticAlgorithm : ModuleRules
{
    public GeneticAlgorithm(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "UEcs"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
        });
    }
}
