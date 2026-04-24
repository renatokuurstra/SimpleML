//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/VehicleResetSystem.h"
#include "VehicleComponent.h"
#include "Components/TrainingDataComponent.h"
#include "Components/GenomeComponents.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "Components/SplineComponent.h"
#include "VehicleLibrary.h"
#include "GameFramework/Pawn.h"

UVehicleResetSystem::UVehicleResetSystem()
{
	RegisterComponent<FVehicleComponent>();
	RegisterComponent<FResetGenomeComponent>();
	RegisterComponent<FTrainingDataComponent>();
	RegisterComponent<FFitnessComponent>();
	RegisterComponent<FEligibleForBreedingTagComponent>();
	RegisterComponent<FUniqueSolutionComponent>();
}

void UVehicleResetSystem::Update_Implementation(float DeltaTime)
{
	AVehicleTrainerContext* TrainerContext = GetTypedContext<AVehicleTrainerContext>();
	if (!TrainerContext || !TrainerContext->TrainerConfig)
	{
		return;
	}

	USplineComponent* Spline = TrainerContext->GetCircuitSpline();
	if (!Spline)
	{
		return;
	}

	auto View = GetView<FVehicleComponent, FResetGenomeComponent, FTrainingDataComponent, FUniqueSolutionComponent>();

	for (auto Entity : View)
	{
		const FVehicleComponent& VehicleComp = View.get<FVehicleComponent>(Entity);
		FTrainingDataComponent& TrainingData = View.get<FTrainingDataComponent>(Entity);
		FUniqueSolutionComponent& UniqueComp = View.get<FUniqueSolutionComponent>(Entity);

		if (VehicleComp.VehiclePawn)
		{
			FVector ResetLocation;
			FRotator ResetRotation;
			
			UVehicleLibrary::GetVehicleSpawnTransform(Spline, TrainerContext->TrainerConfig->SpawnParametricDistance, TrainerContext->TrainerConfig->SpawnVerticalOffset, ResetLocation, ResetRotation);
			UVehicleLibrary::ResetPawnPhysicalState(VehicleComp.VehiclePawn, ResetLocation, ResetRotation);
			
			// Reset training data
			UVehicleLibrary::SetTrainingData(TrainingData, Spline, ResetLocation, GetContext()->GetWorld()->GetTimeSeconds());

			// Also reset segment pass count array (SetTrainingData doesn't touch this)
			TrainingData.SegmentPassCount.Init(0, TrainingData.SegmentPassCount.Num());

			// Reset fitness score
			if (FFitnessComponent* FitComp = GetRegistry().try_get<FFitnessComponent>(Entity))
			{
				for (float& F : FitComp->Fitness)
				{
					F = 0.0f;
				}
			}

			// Assign a new unique ID so that any elite entity still referencing the old ID
			// via SourceId will no longer match this entity. This prevents elites from being
			// updated with fitness data from a completely new "life" of the pooled vehicle.
			UniqueComp.Id = FUniqueSolutionComponent::GenerateNewId();

			// Remove eligibility tag
			GetRegistry().remove<FEligibleForBreedingTagComponent>(Entity);
		}
	}
}
