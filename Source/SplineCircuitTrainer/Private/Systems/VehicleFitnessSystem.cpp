//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/VehicleFitnessSystem.h"
#include "Components/TrainingDataComponent.h"
#include "Components/GenomeComponents.h"

UVehicleFitnessSystem::UVehicleFitnessSystem()
{
	RegisterComponent<FTrainingDataComponent>();
	RegisterComponent<FFitnessComponent>();
}

void UVehicleFitnessSystem::Update_Implementation(float DeltaTime)
{
	auto View = GetView<FTrainingDataComponent, FFitnessComponent>();

	for (auto Entity : View)
	{
		const FTrainingDataComponent& TrainingData = View.get<FTrainingDataComponent>(Entity);
		FFitnessComponent& FitComp = View.get<FFitnessComponent>(Entity);

		if (FitComp.Fitness.Num() > 0)
		{
			float Distance = TrainingData.DistanceTraveled;
			// Add the cube of DistanceTraveled to the fitness.
			FitComp.Fitness[0] += (Distance * Distance * Distance);

			// Mark for reset if fitness is negative.
			if (FitComp.Fitness[0] < 0.0f)
			{
				GetRegistry().emplace_or_replace<FResetGenomeComponent>(Entity);
			}
		}
	}
}
