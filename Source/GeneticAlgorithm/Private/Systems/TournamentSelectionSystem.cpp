#include "Systems/TournamentSelectionSystem.h"
#include "Math/UnrealMathUtility.h"

void UTournamentSelectionSystem::Update(float /*DeltaTime*/)
{
    // Basic placeholder implementation: validate fitness data is present.
    // Why: Early guardrails so later selection steps have a sane base.
    auto FitnessArray = GetComponentArray<FFitnessComponent>();
    if (FitnessArray.Num() == 0)
    {
        return; // No fitness data registered in the registry yet.
    }

    const TArray<float>& Fitness = FitnessArray[0].Fitness; // Single owner of fitness scores
    if (Fitness.Num() == 0)
    {
        return;
    }

    // Create a local RNG (deterministic for tests if RandomSeed != 0)
    FRandomStream Rng;
    if (RandomSeed != 0)
    {
        Rng.Initialize(RandomSeed);
    }

    // Perform a single tournament probe as a smoke test of parameters (does not mutate world)
    const int32 PopulationSize = Fitness.Num();
    const int32 K = FMath::Clamp(TournamentSize, 2, FMath::Max(2, PopulationSize));
    int32 BestIndex = INDEX_NONE;
    float BestValue = -FLT_MAX;

    for (int32 i = 0; i < K; ++i)
    {
        const int32 Pick = (RandomSeed != 0) ? Rng.RandRange(0, PopulationSize - 1) : FMath::RandRange(0, PopulationSize - 1);
        const float Value = Fitness[Pick];
        if (Value > BestValue)
        {
            BestValue = Value;
            BestIndex = Pick;
        }
    }

    // Note: We don't output parents yet; this is a minimal placeholder.
}
