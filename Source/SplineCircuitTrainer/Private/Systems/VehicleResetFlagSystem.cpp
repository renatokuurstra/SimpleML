//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/VehicleResetFlagSystem.h"
#include "VehicleComponent.h"
#include "Components/TrainingDataComponent.h"
#include "Components/GenomeComponents.h"
#include "Components/EliteComponents.h"
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

	// Gather elite SourceIds to check if a car is an elite
	TSet<int64> EliteSourceIds;
	auto EliteView = Registry.view<FEliteTagComponent, FUniqueSolutionComponent>();
	for (auto EliteEntity : EliteView)
	{
		EliteSourceIds.Add(EliteView.get<FUniqueSolutionComponent>(EliteEntity).SourceId);
	}

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

		auto DrawDeadDebug = [&]()
		{
			if (TrainerContext->TrainerConfig->bDebugInfo)
			{
				FColor Color = FColor::White;
				float Duration = 3.0f;
				if (Registry.all_of<FResetGenomeComponent>(Entity))
				{
					FName Reason = Registry.get<FResetGenomeComponent>(Entity).ReasonForReset;
					if (Reason == UVehicleLibrary::ReasonTooFarFromSpline) Color = FColor::Red;
					else if (Reason == UVehicleLibrary::ReasonTooSlow) Color = FColor::Blue;
					else if (Reason == UVehicleLibrary::ReasonNoProgress) Color = FColor::Yellow;
					else if (Reason == UVehicleLibrary::ReasonIncorrectProgress) Color = FColor::White; // Use white for incorrect progress
				}

				DrawDebugPoint(GetContext()->GetWorld(), PawnLocation, 20.0f, Color, false, Duration, 0);

				if (Registry.all_of<FUniqueSolutionComponent>(Entity))
				{
					int64 MyId = Registry.get<FUniqueSolutionComponent>(Entity).Id;
					if (EliteSourceIds.Contains(MyId))
					{
						DrawDebugPoint(GetContext()->GetWorld(), PawnLocation, 20.0f, FColor::Emerald, false, 20.0f, 0);
					}
				}
			}
		};

		// 1. Check distance from spline
		FVector ClosestPoint = Spline->FindLocationClosestToWorldLocation(PawnLocation, ESplineCoordinateSpace::World);
		float DistanceFromSpline = FVector::Dist(PawnLocation, ClosestPoint);

		if (DistanceFromSpline > MaxDistThreshold)
		{
			Registry.emplace<FResetGenomeComponent>(Entity, FResetGenomeComponent{ UVehicleLibrary::ReasonTooFarFromSpline });
			DrawDeadDebug();
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
				DrawDeadDebug();
				continue;
			}
		}

		// 3. Check for progress timeout
		if (TrainingData.TimeSinceLastProgress > NoProgressTimeout)
		{
			Registry.emplace<FResetGenomeComponent>(Entity, FResetGenomeComponent{ UVehicleLibrary::ReasonNoProgress });
			DrawDeadDebug();
		}
	}
}
