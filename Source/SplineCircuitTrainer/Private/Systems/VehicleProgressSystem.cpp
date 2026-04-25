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
	// For a closed/looped spline with N points there are N segments (last point connects back to first)
	// For an open spline with N points there are N-1 segments
	int32 NumPoints = Spline->GetNumberOfSplinePoints();
	int32 NumSegments = NumPoints > 1 ? (Spline->IsClosedLoop() ? NumPoints : NumPoints - 1) : 0;

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

		// If vehicle moved backward, penalize progress tracking but allow recovery.
		// Setting MaxSegmentReached to the current segment (not 0) prevents a permanent
		// trap: if the car is at segment N and we zero it, the car can never recover
		// unless it advances to segment N+1 — which may be impossible near the spline end.
		if (bHadBackwardMovement)
		{
			TrainingData.MaxSegmentReached = CurrentSegment;
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

		// Calculate normalized distance within current segment for fitness calculation.
		// Use the fractional part of the input key — this is the exact fraction of
		// progress within the current segment. The previous approach using
		// GetDistanceAlongSplineAtLocation was broken on closed loops because it finds
		// the *closest* point on the spline, which can be a different segment entirely.
		if (NumSegments > 0 && CurrentSegment >= 0 && CurrentSegment < NumSegments)
		{
			TrainingData.NormalizedDistanceInSegment = FMath::Clamp(
				CurrentInputKey - static_cast<float>(CurrentSegment), 0.0f, 1.0f);
		}
		else
		{
			TrainingData.NormalizedDistanceInSegment = 0.0f;
		}

		TrainingData.LastSplineDistance = CurrentSplineDistance;
		TrainingData.LastSplineSegment = CurrentSegment;
	}
}
