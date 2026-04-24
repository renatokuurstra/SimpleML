//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "CoreMinimal.h"
#include "CQTest.h"
#include "Systems/VehicleFitnessSystem.h"
#include "Components/TrainingDataComponent.h"
#include "Components/GenomeComponents.h"
#include "EcsContext.h"
#include "SplineTestActor.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"


TEST_CLASS(SplineCircuitTrainer_VehicleFitnessSystem_Tests, "SplineCircuitTrainer.VehicleFitnessSystem")
{
	TObjectPtr<UWorld> World;
	TObjectPtr<AVehicleTrainerContext> Context;
	TObjectPtr<ASplineTestActor> SplineActor;
	TObjectPtr<UVehicleFitnessSystem> System;
	TObjectPtr<UVehicleTrainerConfig> Config;

	BEFORE_EACH()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("FitnessTestWorld")));
		ASSERT_THAT(IsNotNull(World, "Transient world should be successfully created"));

		Context = World->SpawnActor<AVehicleTrainerContext>();
		ASSERT_THAT(IsNotNull(Context, "VehicleTrainerContext should be successfully spawned"));

		Config = NewObject<UVehicleTrainerConfig>();
		Context->TrainerConfig = Config;

		SplineActor = World->SpawnActor<ASplineTestActor>();
		ASSERT_THAT(IsNotNull(SplineActor, "SplineActor should be successfully spawned"));

		Context->CircuitActor = SplineActor;

		// Setup a spline with 6 points = 5 segments
		SplineActor->SplineComponent->ClearSplinePoints();
		SplineActor->SplineComponent->AddSplinePoint(FVector(0, 0, 0), ESplineCoordinateSpace::World);
		SplineActor->SplineComponent->AddSplinePoint(FVector(200, 0, 0), ESplineCoordinateSpace::World);
		SplineActor->SplineComponent->AddSplinePoint(FVector(400, 0, 0), ESplineCoordinateSpace::World);
		SplineActor->SplineComponent->AddSplinePoint(FVector(600, 0, 0), ESplineCoordinateSpace::World);
		SplineActor->SplineComponent->AddSplinePoint(FVector(800, 0, 0), ESplineCoordinateSpace::World);
		SplineActor->SplineComponent->AddSplinePoint(FVector(1000, 0, 0), ESplineCoordinateSpace::World);
		SplineActor->SplineComponent->SetClosedLoop(false);
		SplineActor->SplineComponent->UpdateSpline();

		System = NewObject<UVehicleFitnessSystem>();
		ASSERT_THAT(IsNotNull(System, "VehicleFitnessSystem should be successfully created"));
		System->Initialize(Context);
	}

	AFTER_EACH()
	{
		if (World)
		{
			World->DestroyWorld(false);
			World = nullptr;
		}
	}

	TEST_METHOD(CalculatesFitnessBasedOnMaxSegmentReached)
	{
		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();

		FTrainingDataComponent& TrainingData = Registry.emplace<FTrainingDataComponent>(Entity);
		FFitnessComponent& FitComp = Registry.emplace<FFitnessComponent>(Entity);
		FitComp.Fitness.SetNum(1);
		FitComp.Fitness[0] = 0.0f;

		// Vehicle reached segment 3 with 50% progress within that segment
		TrainingData.MaxSegmentReached = 3;
		TrainingData.NormalizedDistanceInSegment = 0.5f;
		TrainingData.LapsCompleted = 0;

		// Run system update
		System->Update(0.1f);

		// Expected fitness: 2^3 + 0.5 = 8.5
		ASSERT_THAT(IsNear(8.5f, FitComp.Fitness[0], 0.1f, "Fitness should be 2^MaxSegmentReached + NormalizedDistance"));

		// Run again - fitness should be recalculated, NOT accumulated
		System->Update(0.1f);
		ASSERT_THAT(IsNear(8.5f, FitComp.Fitness[0], 0.1f, "Fitness should be recalculated, not accumulated"));
	}

	TEST_METHOD(FitnessNeverStacks)
	{
		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();

		FTrainingDataComponent& TrainingData = Registry.emplace<FTrainingDataComponent>(Entity);
		FFitnessComponent& FitComp = Registry.emplace<FFitnessComponent>(Entity);
		FitComp.Fitness.SetNum(1);
		FitComp.Fitness[0] = 0.0f;

		TrainingData.MaxSegmentReached = 2;
		TrainingData.NormalizedDistanceInSegment = 0.25f;

		// First update
		System->Update(0.1f);
		float FirstFitness = FitComp.Fitness[0];
		// Expected: 2^2 + 0.25 = 4.25
		ASSERT_THAT(IsNear(4.25f, FirstFitness, 0.1f, "First fitness calculation"));

		// Second update with same data - should be identical
		System->Update(0.1f);
		ASSERT_THAT(IsNear(FirstFitness, FitComp.Fitness[0], 0.01f, "Fitness must not stack on repeated updates"));

		// Third update - still same
		System->Update(0.1f);
		ASSERT_THAT(IsNear(FirstFitness, FitComp.Fitness[0], 0.01f, "Fitness must not stack on third update"));
	}

	TEST_METHOD(HandlesMultipleEntitiesWithDifferentProgress)
	{
		entt::registry& Registry = Context->GetRegistry();

		// Entity 1: reached segment 1, 75% through
		entt::entity E1 = Registry.create();
		FTrainingDataComponent& Data1 = Registry.emplace<FTrainingDataComponent>(E1);
		Data1.MaxSegmentReached = 1;
		Data1.NormalizedDistanceInSegment = 0.75f;
		Data1.LapsCompleted = 0;
		Registry.emplace<FFitnessComponent>(E1).Fitness.Add(0.0f);

		// Entity 2: reached segment 4, 25% through
		entt::entity E2 = Registry.create();
		FTrainingDataComponent& Data2 = Registry.emplace<FTrainingDataComponent>(E2);
		Data2.MaxSegmentReached = 4;
		Data2.NormalizedDistanceInSegment = 0.25f;
		Data2.LapsCompleted = 0;
		Registry.emplace<FFitnessComponent>(E2).Fitness.Add(0.0f);

		System->Update(0.1f);

		// E1: 2^1 + 0.75 = 2.75
		ASSERT_THAT(IsNear(2.75f, Registry.get<FFitnessComponent>(E1).Fitness[0], 0.1f, "E1 fitness should be 2.75"));

		// E2: 2^4 + 0.25 = 16.25
		ASSERT_THAT(IsNear(16.25f, Registry.get<FFitnessComponent>(E2).Fitness[0], 0.1f, "E2 fitness should be 16.25"));

		// E2 should have higher fitness than E1 (further progress)
		ASSERT_THAT(IsTrue(Registry.get<FFitnessComponent>(E2).Fitness[0] > Registry.get<FFitnessComponent>(E1).Fitness[0],
			"Entity with higher segment progress should have higher fitness"));
	}

	TEST_METHOD(ClosedLoopAddsLapBonus)
	{
		// Make spline a closed loop
		SplineActor->SplineComponent->SetClosedLoop(true);
		SplineActor->SplineComponent->UpdateSpline();

		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();

		FTrainingDataComponent& TrainingData = Registry.emplace<FTrainingDataComponent>(Entity);
		FFitnessComponent& FitComp = Registry.emplace<FFitnessComponent>(Entity);
		FitComp.Fitness.SetNum(1);
		FitComp.Fitness[0] = 0.0f;

		// Vehicle has completed 2 laps, currently at segment 1
		TrainingData.MaxSegmentReached = 1;
		TrainingData.NormalizedDistanceInSegment = 0.5f;
		TrainingData.LapsCompleted = 2;

		System->Update(0.1f);

		// Expected: 2^1 + 0.5 + 2 * (2^5) = 2.5 + 64 = 66.5
		// (5 segments, so lap bonus is 2^5 = 32 per lap)
		ASSERT_THAT(IsNear(66.5f, FitComp.Fitness[0], 0.5f, "Fitness should include lap bonus for closed loop"));
	}

	TEST_METHOD(ZeroProgressYieldsMinimalFitness)
	{
		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();

		FTrainingDataComponent& TrainingData = Registry.emplace<FTrainingDataComponent>(Entity);
		FFitnessComponent& FitComp = Registry.emplace<FFitnessComponent>(Entity);
		FitComp.Fitness.SetNum(1);
		FitComp.Fitness[0] = 0.0f;

		// Vehicle hasn't moved at all (MaxSegmentReached=0, NormalizedDistance=0)
		TrainingData.MaxSegmentReached = 0;
		TrainingData.NormalizedDistanceInSegment = 0.0f;
		TrainingData.LapsCompleted = 0;

		System->Update(0.1f);

		// Expected: 2^0 + 0 = 1.0 (minimum fitness for being at start)
		ASSERT_THAT(IsNear(1.0f, FitComp.Fitness[0], 0.1f, "Vehicle at start should have minimal fitness"));
	}

	TEST_METHOD(BackwardMovementResultsInZeroFitness)
	{
		// This is the critical test: when VehicleProgressSystem detects backward movement,
		// it zeroes MaxSegmentReached, LapsCompleted, and NormalizedDistanceInSegment.
		// The fitness system should then calculate near-zero fitness.
		entt::registry& Registry = Context->GetRegistry();
		entt::entity Entity = Registry.create();

		FTrainingDataComponent& TrainingData = Registry.emplace<FTrainingDataComponent>(Entity);
		FFitnessComponent& FitComp = Registry.emplace<FFitnessComponent>(Entity);
		FitComp.Fitness.SetNum(1);
		FitComp.Fitness[0] = 0.0f;

		// Simulate what happens after VehicleProgressSystem detects backward movement:
		// all progress fields are zeroed
		TrainingData.MaxSegmentReached = 0;
		TrainingData.NormalizedDistanceInSegment = 0.0f;
		TrainingData.LapsCompleted = 0;

		System->Update(0.1f);

		// Fitness should be minimal (2^0 + 0 = 1), not inflated
		// This ensures backward-moving cars don't get selected as elites
		ASSERT_THAT(IsNear(1.0f, FitComp.Fitness[0], 0.1f, "Backward-moving car should have minimal fitness"));
	}

	TEST_METHOD(ForwardMovingCarOutscoresBackwardCar)
	{
		// Simulates the exact bug scenario: one car moves forward, another goes backward
		entt::registry& Registry = Context->GetRegistry();

		// Forward car: reached segment 2
		entt::entity ForwardCar = Registry.create();
		FTrainingDataComponent& ForwardData = Registry.emplace<FTrainingDataComponent>(ForwardCar);
		ForwardData.MaxSegmentReached = 2;
		ForwardData.NormalizedDistanceInSegment = 0.5f;
		ForwardData.LapsCompleted = 0;
		FFitnessComponent& ForwardFit = Registry.emplace<FFitnessComponent>(ForwardCar);
		ForwardFit.Fitness.Add(0.0f);

		// Backward car: progress zeroed by VehicleProgressSystem
		entt::entity BackwardCar = Registry.create();
		FTrainingDataComponent& BackwardData = Registry.emplace<FTrainingDataComponent>(BackwardCar);
		BackwardData.MaxSegmentReached = 0;
		BackwardData.NormalizedDistanceInSegment = 0.0f;
		BackwardData.LapsCompleted = 0;
		FFitnessComponent& BackwardFit = Registry.emplace<FFitnessComponent>(BackwardCar);
		BackwardFit.Fitness.Add(0.0f);

		System->Update(0.1f);

		// Forward car: 2^2 + 0.5 = 4.5
		ASSERT_THAT(IsNear(4.5f, ForwardFit.Fitness[0], 0.1f, "Forward car fitness"));

		// Backward car: 2^0 + 0 = 1.0
		ASSERT_THAT(IsNear(1.0f, BackwardFit.Fitness[0], 0.1f, "Backward car fitness"));

		// CRITICAL: forward car must have significantly higher fitness
		ASSERT_THAT(IsTrue(ForwardFit.Fitness[0] > BackwardFit.Fitness[0] * 2,
			"Forward-moving car must have significantly higher fitness than backward-moving car"));
	}
};
