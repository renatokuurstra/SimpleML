//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/VehicleProgressSystem.h"
#include "VehicleComponent.h"
#include "Components/TrainingDataComponent.h"
#include "VehicleTrainerContext.h"
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
		
		float Delta = CurrentSplineDistance - TrainingData.LastSplineDistance;

		// Handle looped spline wrapping
		if (Spline->IsClosedLoop())
		{
			if (Delta > SplineLength * 0.5f)
			{
				// Pawn moved backward across the start/end point
				Delta -= SplineLength;
			}
			else if (Delta < -SplineLength * 0.5f)
			{
				// Pawn moved forward across the start/end point
				Delta += SplineLength;
			}
		}

		TrainingData.DistanceTraveled += Delta;
		TrainingData.LastSplineDistance = CurrentSplineDistance;
	}
}
