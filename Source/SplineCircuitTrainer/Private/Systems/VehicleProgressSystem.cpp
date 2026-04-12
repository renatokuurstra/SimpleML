//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/VehicleProgressSystem.h"
#include "VehicleComponent.h"
#include "Components/TrainingDataComponent.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Pawn.h"

UVehicleProgressSystem::UVehicleProgressSystem()
{
	RegisterComponent<FVehicleComponent>();
	RegisterComponent<FTrainingDataComponent>();
}

void UVehicleProgressSystem::Update_Implementation(float DeltaTime)
{
	AVehicleTrainerContext* TrainerContext = GetTypedContext<AVehicleTrainerContext>();
	if (!TrainerContext)
	{
		return;
	}

	USplineComponent* Spline = TrainerContext->GetCircuitSpline();
	if (!Spline)
	{
		return;
	}

	float SplineLength = Spline->GetSplineLength();
	if (SplineLength <= 0.0f)
	{
		return;
	}

	float MinProgress = 0.0f;
	if (TrainerContext->TrainerConfig)
	{
		MinProgress = TrainerContext->TrainerConfig->MinimumProgressBetweenEvaluations;
	}

	auto View = GetView<FVehicleComponent, FTrainingDataComponent>();

	for (auto Entity : View)
	{
		const FVehicleComponent& VehicleComp = View.get<FVehicleComponent>(Entity);
		FTrainingDataComponent& TrainingData = View.get<FTrainingDataComponent>(Entity);

		if (!VehicleComp.VehiclePawn)
		{
			continue;
		}

		APawn* Pawn = VehicleComp.VehiclePawn;
		FVector PawnLocation = Pawn->GetActorLocation();

		float CurrentSplineDistance = Spline->GetDistanceAlongSplineAtLocation(PawnLocation, ESplineCoordinateSpace::World);
		float CurrentInputKey = Spline->FindInputKeyClosestToWorldLocation(PawnLocation);
		int32 CurrentSegment = FMath::FloorToInt(CurrentInputKey);

		float Delta = CurrentSplineDistance - TrainingData.LastSplineDistance;

		// Handle looped spline wrapping
		if (Spline->IsClosedLoop())
		{
			// If distance jump is more than half the spline length, assume a wrap
			if (Delta > SplineLength * 0.5f)
			{
				Delta -= SplineLength;
			}
			else if (Delta < -SplineLength * 0.5f)
			{
				Delta += SplineLength;
			}
		}

		// Teleport / bad-read guard
		// If the delta is still larger than a reasonable threshold (e.g., 20% of spline length in one frame),
		// treat it as a discontinuity and reset the tracking state.
		const float TeleportThreshold = SplineLength * 0.2f;
		if (FMath::Abs(Delta) > TeleportThreshold)
		{
			TrainingData.LastSplineDistance = CurrentSplineDistance;
			TrainingData.LastSplineSegment = CurrentSegment;
			continue;
		}

		TrainingData.DistanceTraveled += Delta;
		TrainingData.LastDistanceDelta = Delta;

		if (Delta > MinProgress)
		{
			TrainingData.TimeSinceLastProgress = 0.0f;
		}
		else
		{
			TrainingData.TimeSinceLastProgress += DeltaTime;
		}

		TrainingData.LastSplineDistance = CurrentSplineDistance;
		TrainingData.LastSplineSegment = CurrentSegment;
	}
}
