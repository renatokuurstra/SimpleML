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
				// Calculate fitness based on cumulative segment progress across all laps.
				// This prevents backward movement from increasing fitness AND ensures fitness
				// keeps growing as the car completes more laps (no plateau after first lap).
				//
				// Formula:
				// - EffectiveSegment = LapsCompleted * NumSegments + MaxSegmentReached (capped at 30)
				// - Fitness = 2^EffectiveSegment + NormalizedDistanceInSegment
				//
				// Key properties:
				// - MaxSegmentReached only increases on forward movement (enforced by VehicleProgressSystem)
				// - LapsCompleted only increments on valid forward wraparound
				// - NormalizedDistanceInSegment is 0.0-1.0 within the current segment
				// - Fitness is ALWAYS recalculated, never stacked/accumulated
				// - Effective segment cap of 30 prevents float overflow (2^30 ~ 1 billion)

			float Fitness = 0.0f;

		if (NumSegments > 0)
		{
			// Clamp MaxSegmentReached to valid range for the current lap
			int32 ClampedMaxSegment = FMath::Clamp(TrainingData.MaxSegmentReached, 0, NumSegments - 1);

			// Calculate cumulative effective segment across all laps.
			// This ensures fitness keeps growing exponentially as the car completes more laps,
			// instead of plateauing after the first lap.
			// Example (10 segments): segment 5, lap 0 -> eff=5; segment 5, lap 1 -> eff=15; segment 5, lap 2 -> eff=25
			// Cap at 30 to prevent float overflow (2^30 ~ 1 billion, well within float precision).
			int32 EffectiveSegment = TrainingData.LapsCompleted * NumSegments + ClampedMaxSegment;
			EffectiveSegment = FMath::Min(EffectiveSegment, 30);

			// Base fitness: exponential scaling based on cumulative segment progress
			// 2^N grows quickly: eff 0 = 1, eff 5 = 32, eff 10 = 1024, eff 20 = 1M, eff 30 = 1B
			Fitness = FMath::Pow(2.0f, static_cast<float>(EffectiveSegment));

			// Add partial progress within current segment (0.0 to 1.0)
			Fitness += TrainingData.NormalizedDistanceInSegment;
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
