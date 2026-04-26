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
			}
		}
	}

	for (auto Entity : View)
	{
		const FTrainingDataComponent& TrainingData = View.get<FTrainingDataComponent>(Entity);
		FFitnessComponent& FitComp = View.get<FFitnessComponent>(Entity);

		if (FitComp.Fitness.Num() > 0)
		{
			int32 FitnessIndex = FitComp.BuiltForFitnessIndex;
			float OldFitness = (FitnessIndex < FitComp.Fitness.Num()) ? FitComp.Fitness[FitnessIndex] : 0.0f;

			// Calculate fitness based on the SUM of SegmentPassCount across all segments.
			//
			// WHY THIS FIX:
			// The previous formula (LapsCompleted * NumSegments + MaxSegmentReached) had two
			// critical flaws that caused fitness to plateau after the first lap:
			//
			//   1. MaxSegmentReached is bounded by (NumSegments - 1). After completing one lap
			//      it reaches its maximum and can never grow again. All post-lap-1 fitness growth
			//      depended entirely on LapsCompleted incrementing correctly.
			//
			//   2. The wraparound detection for LapsCompleted (CurrentSegment < LastSplineSegment
			//      && Delta > 0.0f) is fragile. At the loop boundary the delta calculation can
			//      be slightly off due to spline geometry, causing LapsCompleted to never increment.
			//      Result: fitness stuck at ~8225.50 (2^13 + 0.50) forever.
			//
			// THE FIX - Sum(SegmentPassCount):
			// SegmentPassCount is incremented on EVERY segment entry (both forward progression
			// and valid wraparound). The sum is a naturally monotonically increasing measure of
			// total progress that does NOT depend on detecting the exact wraparound moment.
			// Each lap adds exactly NumSegments to the sum. Fitness only grows, never shrinks.
			//
			// Formula:
			//   - TotalSegmentPasses = Sum(SegmentPassCount[0..N-1])
			//   - EffectiveSegment = TotalSegmentPasses (capped at 30)
			//   - Fitness = 2^EffectiveSegment + NormalizedDistanceInSegment
			//
			// Key properties:
			//   - SegmentPassCount only increases on confirmed forward segment transitions
			//   - Sum naturally grows by NumSegments per lap (no separate lap counter needed)
			//   - NormalizedDistanceInSegment is 0.0-1.0 within the current segment
			//   - Fitness is ALWAYS recalculated from scratch, never accumulated
			//   - Effective segment cap of 30 prevents float overflow (2^30 ~ 1 billion)
			//   - Backward movement triggers a reset (SegmentPassCount zeroed), so backward
			//     movers get low fitness naturally

			float Fitness;

			if (NumSegments > 0)
			{
				int32 TotalSegmentPasses = 0;
				// Calculate total segment passes by summing all segment pass counts.
				// This is the cumulative measure of progress across all laps.
				// Each time the car enters a segment (forward or wraparound), the count increments.
				// After 1 complete lap on a 13-segment spline: sum = 13.
				// After 2 laps: sum = 26. After 3 laps: sum = 39 (capped to 30).
				for (int32 Count : TrainingData.SegmentPassCount)
				{
					TotalSegmentPasses += Count;
				}

				// Base fitness: exponential scaling based on cumulative segment progress
				// 2^N grows quickly: eff 0 = 1, eff 5 = 32, eff 10 = 1024, eff 20 = 1M, eff 30 = 1B
				Fitness = FMath::Pow(2.0f, static_cast<float>(TotalSegmentPasses));

				// Add partial progress within current segment (0.0 to 1.0)
				Fitness += TrainingData.NormalizedDistanceInSegment;
				
				// Sanity check: fitness should not jump by more than 1000 in a single frame.
				// Large jumps indicate SegmentPassCount corruption or false-positive segment detection.
				if (Fitness - OldFitness > 1000.0f)
				{
					UE_LOG(LogTemp, Warning, TEXT("[Fitness] SUSPICIOUS fitness jump: %.2f -> %.2f (delta=%.2f). TotalSegmentPasses=%d, EffectiveSegment=%d"),
						OldFitness, Fitness, Fitness - OldFitness, TotalSegmentPasses, TotalSegmentPasses);
				}
			}
			else
			{
				// Fallback: if no segments available, use distance traveled squared
				// This maintains backward compatibility for edge cases
				Fitness = TrainingData.DistanceTraveled * TrainingData.DistanceTraveled;
			}
			FitComp.Fitness[FitnessIndex] = Fitness;
		}
	}
}
