//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/VehicleResetFlagSystem.h"
#include "VehicleComponent.h"
#include "Components/TrainingDataComponent.h"
#include "Components/GenomeComponents.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"
#include "VehicleLibrary.h"

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
	float MinAverageVelocity = TrainerContext->TrainerConfig->MinAverageVelocity;
	float MinAgeForReset = TrainerContext->TrainerConfig->MinAgeForReset;
	float NoProgressTimeout = TrainerContext->TrainerConfig->NoProgressTimeout;
	float CurrentTime = TrainerContext->GetWorld()->GetTimeSeconds();

	auto View = GetView<FVehicleComponent, FTrainingDataComponent>();
	entt::registry& Registry = GetRegistry();

	for (auto Entity : View)
	{
		// Skip if already flagged for reset
		if (Registry.all_of<FResetGenomeComponent>(Entity))
		{
			UE_LOG(LogTemp, Warning, TEXT("Skipping entity %d because it is already flagged for reset"), Entity);
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
			Registry.emplace<FResetGenomeComponent>(Entity, FResetGenomeComponent{ UVehicleLibrary::ReasonTooFarFromSpline });
			if (TrainerContext->TrainerConfig->bDebugInfo)
			{
				DrawDebugPoint(GetContext()->GetWorld(), PawnLocation, 50.0f, FColor::Red, false, 3.0f, 0);
			}
			continue;
		}

		// 2. Check for average progress during lifespan
		float Age = CurrentTime - TrainingData.CreationTime;
		if (Age >= MinAgeForReset)
		{
			float AverageVelocity = TrainingData.DistanceTraveled / Age;
			if (AverageVelocity < MinAverageVelocity)
			{
				Registry.emplace<FResetGenomeComponent>(Entity, FResetGenomeComponent{ UVehicleLibrary::ReasonTooSlow });
				if (TrainerContext->TrainerConfig->bDebugInfo)
				{
					DrawDebugPoint(GetContext()->GetWorld(), PawnLocation, 50.0f, FColor::Blue, false, 3.0f, 0);
				}
				continue;
			}
		}

		// 3. Check for progress timeout
		if (TrainingData.TimeSinceLastProgress > NoProgressTimeout)
		{
			Registry.emplace<FResetGenomeComponent>(Entity, FResetGenomeComponent{ UVehicleLibrary::ReasonNoProgress });
			if (TrainerContext->TrainerConfig->bDebugInfo)
			{
				DrawDebugPoint(GetContext()->GetWorld(), PawnLocation, 50.0f, FColor::Yellow, false, 3.0f, 0);
			}
		}
	}
}
