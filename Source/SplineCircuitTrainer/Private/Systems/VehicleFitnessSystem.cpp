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
			float Distance = TrainingData.LastSplineDistance * TrainingData.LastSplineDistance;
			FitComp.Fitness[FitComp.BuiltForFitnessIndex] = Distance;
		}
	}
}
