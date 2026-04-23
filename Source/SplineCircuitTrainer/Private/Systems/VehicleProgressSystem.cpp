//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/VehicleProgressSystem.h"
#include "VehicleComponent.h"
#include "VehicleLibrary.h"
#include "Components/TrainingDataComponent.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "Components/GenomeComponents.h"
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

		// Check if vehicle is progressing correctly along the spline
		bool bIsProgressingCorrectly = true;
		
		// Only perform segment validation when we have a valid previous segment and pass count array
		if (TrainingData.LastSplineSegment >= 0 && TrainingData.SegmentPassCount.Num() > 0)
		{
			// Check if this is a new segment entry
			if (CurrentSegment != TrainingData.LastSplineSegment)
			{
				// If we're moving forward, increment the pass count for that segment
				if (CurrentSegment > TrainingData.LastSplineSegment)
				{
					// Normal forward progression - check if it's the expected next segment
					if (CurrentSegment != TrainingData.LastSplineSegment + 1)
					{
						// We skipped a segment or went backwards, which is incorrect
						bIsProgressingCorrectly = false;
					}
				}
				else if (CurrentSegment < TrainingData.LastSplineSegment)
				{
					// If we're going backward on a closed loop, check if it's because of wraparound
					if (!Spline->IsClosedLoop())
					{
						// For non-closed loops, backward movement is invalid
						bIsProgressingCorrectly = false;
					}
					else
					{
						// For closed loop, check if this is a wraparound (i.e., from last segment back to first)
						int32 NumSegments = TrainingData.SegmentPassCount.Num();
						if (TrainingData.LastSplineSegment == NumSegments - 1 && CurrentSegment == 0)
						{
							// This is valid wraparound from last to first segment
						}
						else
						{
							// Any other backward movement is invalid
							bIsProgressingCorrectly = false;
						}
					}
				}
				
				// Update the pass count for this segment
				if (CurrentSegment >= 0 && CurrentSegment < TrainingData.SegmentPassCount.Num())
				{
					TrainingData.SegmentPassCount[CurrentSegment]++;
				}
			}
			else
			{
				// Same segment, no progression - valid if we're just staying in place or oscillating slightly
				// No change needed to pass count or progress validation
			}
		}
		
		// If vehicle is not progressing correctly, mark it for reset
		if (!bIsProgressingCorrectly)
		{
			entt::registry& Registry = GetRegistry();
			
			// Check if already flagged for reset - avoid duplicate flags
			if (!Registry.all_of<FResetGenomeComponent>(Entity))
			{
				Registry.emplace<FResetGenomeComponent>(Entity, FResetGenomeComponent{ UVehicleLibrary::ReasonIncorrectProgress });
			}
		}

		TrainingData.LastSplineDistance = CurrentSplineDistance;
		TrainingData.LastSplineSegment = CurrentSegment;
	}
}
