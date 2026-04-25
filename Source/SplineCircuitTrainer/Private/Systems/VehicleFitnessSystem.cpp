//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/VehicleFitnessSystem.h"
#include "Components/TrainingDataComponent.h"
#include "Components/GenomeComponents.h"
#include "VehicleTrainerContext.h"
#include "Components/SplineComponent.h"

UVehicleFitnessSystem::UVehicleFitnessSystem()
{
	RegisterComponent<FTrainingDataComponent>();
	RegisterComponent<FFitnessComponent>();
}

void UVehicleFitnessSystem::Update_Implementation(float DeltaTime)
{
	auto View = GetView<FTrainingDataComponent, FFitnessComponent>();

	// Get total number of segments from the spline (used for fitness scaling)
	int32 NumSegments = 0;
	bool bIsClosedLoop = false;
	{
		AVehicleTrainerContext* TrainerContext = GetTypedContext<AVehicleTrainerContext>();
		if (TrainerContext)
		{
			USplineComponent* Spline = TrainerContext->GetCircuitSpline();
			if (Spline)
			{
				int32 NumPoints = Spline->GetNumberOfSplinePoints();
				// For a closed/looped spline with N points there are N segments (last point connects back to first)
				NumSegments = NumPoints > 1 ? (Spline->IsClosedLoop() ? NumPoints : NumPoints - 1) : 0;
				bIsClosedLoop = Spline->IsClosedLoop();
			}
		}
	}

	for (auto Entity : View)
	{
		const FTrainingDataComponent& TrainingData = View.get<FTrainingDataComponent>(Entity);
		FFitnessComponent& FitComp = View.get<FFitnessComponent>(Entity);

		if (FitComp.Fitness.Num() > 0)
		{
			// Calculate fitness based on segment-based progress tracking.
			// This prevents backward movement from increasing fitness.
			//
			// Formula:
			// - Closed loop: Fitness = Laps * 2^NumSegments + 2^MaxSegmentReached + NormalizedDistanceInSegment
			// - Open path:  Fitness = 2^MaxSegmentReached + NormalizedDistanceInSegment
			//
			// Key properties:
			// - MaxSegmentReached only increases on forward movement (enforced by VehicleProgressSystem)
			// - LapsCompleted only increments on valid forward wraparound
			// - NormalizedDistanceInSegment is 0.0-1.0 within the current segment
			// - Fitness is ALWAYS recalculated, never stacked/accumulated

			float Fitness = 0.0f;

			if (NumSegments > 0)
			{
				// Clamp MaxSegmentReached to valid range
				int32 ClampedMaxSegment = FMath::Clamp(TrainingData.MaxSegmentReached, 0, NumSegments - 1);

				// Base fitness: exponential scaling based on furthest segment reached
				// 2^N grows quickly: segment 0 = 1, segment 5 = 32, segment 10 = 1024
				Fitness = FMath::Pow(2.0f, static_cast<float>(ClampedMaxSegment));

				// Add partial progress within current segment (0.0 to 1.0)
				Fitness += TrainingData.NormalizedDistanceInSegment;

				// For closed loops, add significant bonus for completed laps
				// Each lap adds 2^NumSegments, which is the fitness of completing all segments once
				if (bIsClosedLoop && TrainingData.LapsCompleted > 0)
				{
					float LapBonus = FMath::Pow(2.0f, static_cast<float>(NumSegments));
					Fitness += TrainingData.LapsCompleted * LapBonus;
				}
			}
			else
			{
				// Fallback: if no segments available, use distance traveled squared
				// This maintains backward compatibility for edge cases
				Fitness = TrainingData.DistanceTraveled * TrainingData.DistanceTraveled;
			}

			// Ensure fitness is non-negative
			Fitness = FMath::Max(0.0f, Fitness);

			// Write fitness using the correct population-specific index
			FitComp.Fitness[FitComp.BuiltForFitnessIndex] = Fitness;
		}
	}
}
