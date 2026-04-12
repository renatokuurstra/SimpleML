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

	auto View = GetView<FVehicleComponent, FResetGenomeComponent, FTrainingDataComponent>();

	for (auto Entity : View)
	{
		const FVehicleComponent& VehicleComp = View.get<FVehicleComponent>(Entity);
		FTrainingDataComponent& TrainingData = View.get<FTrainingDataComponent>(Entity);

		if (VehicleComp.VehiclePawn)
		{
			FVector ResetLocation;
			FRotator ResetRotation;
			
			UVehicleLibrary::GetVehicleSpawnTransform(Spline, TrainerContext->TrainerConfig->SpawnParametricDistance, TrainerContext->TrainerConfig->SpawnVerticalOffset, ResetLocation, ResetRotation);
			UVehicleLibrary::ResetPawnPhysicalState(VehicleComp.VehiclePawn, ResetLocation, ResetRotation);
			
			// Reset training data
			UVehicleLibrary::SetTrainingData(TrainingData, Spline, ResetLocation, GetContext()->GetWorld()->GetTimeSeconds());
		}
	}
}
