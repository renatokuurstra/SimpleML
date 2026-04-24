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

	// Pre-calculate segment count for normalized distance within segment
	int32 NumPoints = Spline->GetNumberOfSplinePoints();
	int32 NumSegments = NumPoints > 1 ? NumPoints - 1 : 0;

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

		// ---- Segment-based progress validation ----
		// Core invariant: MaxSegmentReached ONLY increases on confirmed forward movement.
		// Backward movement resets all progress tracking, so the car gets zero fitness.

		bool bIsProgressingCorrectly = true;
		bool bHadBackwardMovement = false;

		// Only perform segment validation when we have a valid previous segment and pass count array
		if (TrainingData.LastSplineSegment >= 0 && TrainingData.SegmentPassCount.Num() > 0)
		{
			if (CurrentSegment != TrainingData.LastSplineSegment)
			{
				// Vehicle crossed into a new segment
				if (CurrentSegment > TrainingData.LastSplineSegment)
				{
					// Forward progression - check if it's the expected next segment
					if (CurrentSegment == TrainingData.LastSplineSegment + 1)
					{
						// Valid forward progression: segment N -> N+1
						TrainingData.MaxSegmentReached = FMath::Max(TrainingData.MaxSegmentReached, CurrentSegment);
						TrainingData.SegmentPassCount[CurrentSegment]++;
					}
					else
					{
						// Skipped a segment (e.g., segment 0 -> segment 5 on a closed loop going backward)
						// This is backward movement disguised as "higher segment number"
						bIsProgressingCorrectly = false;
						bHadBackwardMovement = true;
					}
				}
				else // CurrentSegment < TrainingData.LastSplineSegment
				{
					// Segment index decreased - could be valid wraparound or backward movement
					if (Spline->IsClosedLoop())
					{
						// Check if this is a valid wraparound (last segment -> segment 0)
						int32 NumSegs = TrainingData.SegmentPassCount.Num();
						if (TrainingData.LastSplineSegment == NumSegs - 1 && CurrentSegment == 0)
						{
							// Valid wraparound - completed a full lap
							TrainingData.LapsCompleted++;
							TrainingData.MaxSegmentReached = FMath::Max(TrainingData.MaxSegmentReached, NumSegs - 1);
							TrainingData.SegmentPassCount[CurrentSegment]++;
						}
						else
						{
							// Backward movement on closed loop (not a valid wraparound)
							bIsProgressingCorrectly = false;
							bHadBackwardMovement = true;
						}
					}
					else
					{
						// Backward movement on open path - always invalid
						bIsProgressingCorrectly = false;
						bHadBackwardMovement = true;
					}
				}
			}
			// NOTE: When the car stays in the same segment, we do NOT update MaxSegmentReached.
			// MaxSegmentReached only increases on confirmed forward segment transitions.
			// This prevents a car that spawns near the end of the spline from getting high fitness.
		}

		// If vehicle moved backward, zero out all progress tracking
		// This ensures backward-moving cars get zero fitness
		if (bHadBackwardMovement)
		{
			TrainingData.MaxSegmentReached = 0;
			TrainingData.LapsCompleted = 0;
			TrainingData.NormalizedDistanceInSegment = 0.0f;
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

		// Calculate normalized distance within current segment for fitness calculation
		if (NumSegments > 0 && CurrentSegment >= 0 && CurrentSegment < NumSegments)
		{
			// Get the start distance of the current segment
			float SegmentStartDistance = Spline->GetDistanceAlongSplineAtLocation(
				Spline->GetLocationAtSplinePoint(CurrentSegment, ESplineCoordinateSpace::World), ESplineCoordinateSpace::World);
			
			// Get the end distance of the current segment
			int32 NextSegment = FMath::Min(CurrentSegment + 1, NumSegments - 1);
			float SegmentEndDistance = Spline->GetDistanceAlongSplineAtLocation(
				Spline->GetLocationAtSplinePoint(NextSegment, ESplineCoordinateSpace::World), ESplineCoordinateSpace::World);
			
			float SegmentLength = SegmentEndDistance - SegmentStartDistance;
			if (SegmentLength > 0.0f)
			{
				float DistanceInSegment = CurrentSplineDistance - SegmentStartDistance;
				TrainingData.NormalizedDistanceInSegment = FMath::Clamp(DistanceInSegment / SegmentLength, 0.0f, 1.0f);
			}
			else
			{
				TrainingData.NormalizedDistanceInSegment = 0.0f;
			}
		}
		else
		{
			TrainingData.NormalizedDistanceInSegment = 0.0f;
		}

		TrainingData.LastSplineDistance = CurrentSplineDistance;
		TrainingData.LastSplineSegment = CurrentSegment;
	}
}
