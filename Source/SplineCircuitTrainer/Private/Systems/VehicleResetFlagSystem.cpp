//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/VehicleResetFlagSystem.h"
#include "VehicleComponent.h"
#include "Components/TrainingDataComponent.h"
#include "Components/GenomeComponents.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Pawn.h"

UVehicleResetFlagSystem::UVehicleResetFlagSystem()
{
	RegisterComponent<FVehicleComponent>();
	RegisterComponent<FTrainingDataComponent>();
}

void UVehicleResetFlagSystem::Update_Implementation(float DeltaTime)
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

	float MaxDistThreshold = TrainerContext->TrainerConfig->MaxSplineDistanceThreshold;
	float NoProgressTimeout = TrainerContext->TrainerConfig->NoProgressTimeout;
	float MinProgress = TrainerContext->TrainerConfig->MinimumProgressBetweenEvaluations;

	auto View = GetView<FVehicleComponent, FTrainingDataComponent>();
	entt::registry& Registry = GetRegistry();

	for (auto Entity : View)
	{
		// Skip if already flagged for reset
		if (Registry.all_of<FResetGenomeComponent>(Entity))
		{
			continue;
		}

		const FVehicleComponent& VehicleComp = View.get<FVehicleComponent>(Entity);
		FTrainingDataComponent& TrainingData = View.get<FTrainingDataComponent>(Entity);

		if (!VehicleComp.VehiclePawn)
		{
			continue;
		}

		FVector PawnLocation = VehicleComp.VehiclePawn->GetActorLocation();

		// 1. Check distance from spline
		FVector ClosestPoint = Spline->FindLocationClosestToWorldLocation(PawnLocation, ESplineCoordinateSpace::World);
		float DistanceFromSpline = FVector::Dist(PawnLocation, ClosestPoint);

		if (DistanceFromSpline > MaxDistThreshold)
		{
			Registry.emplace<FResetGenomeComponent>(Entity);
			continue;
		}

		// 2. Check for positive progress
		if (TrainingData.DistanceTraveled > TrainingData.MaxDistanceTraveled + MinProgress)
		{
			TrainingData.MaxDistanceTraveled = TrainingData.DistanceTraveled;
			TrainingData.TimeSinceLastProgress = 0.0f;
		}
		else
		{
			TrainingData.TimeSinceLastProgress += DeltaTime;
		}

		if (TrainingData.TimeSinceLastProgress >= NoProgressTimeout)
		{
			Registry.emplace<FResetGenomeComponent>(Entity);
		}
	}
}
