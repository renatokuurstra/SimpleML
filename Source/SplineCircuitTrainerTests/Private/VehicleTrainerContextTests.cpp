//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "CoreMinimal.h"
#include "CQTest.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "TestVehiclePawn.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EcsChainEvents.h"

TEST_CLASS(SplineCircuitTrainer_VehicleTrainerContext_Tests, "SplineCircuitTrainer.VehicleTrainerContext")
{
	TObjectPtr<UWorld> World;

	BEFORE_EACH()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false, FName(TEXT("TrainerTestWorld")));
		ASSERT_THAT(IsNotNull(World, "Transient world should be successfully created"));
	}

	AFTER_EACH()
	{
		if (World)
		{
			World->DestroyWorld(false);
			World = nullptr;
		}
	}

	TEST_METHOD(CanCreateVehicleTrainerContext)
	{
		AVehicleTrainerContext* Trainer = World->SpawnActor<AVehicleTrainerContext>();
		ASSERT_THAT(IsNotNull(Trainer, "VehicleTrainerContext actor should be successfully spawned"));
	}

	TEST_METHOD(SpawnsCorrectNumberOfPawnsOnBeginPlay)
	{
		// 1. Setup config
		UVehicleTrainerConfig* Config = NewObject<UVehicleTrainerConfig>();
		Config->VehiclePawnClass = ATestVehiclePawn::StaticClass();
		Config->Population = 5;
		Config->HiddenLayerSizes = { 16 };
		Config->VehicleOutputCount = 2;

		// 2. Spawn Trainer
		AVehicleTrainerContext* Trainer = World->SpawnActor<AVehicleTrainerContext>();
		Trainer->TrainerConfig = Config;

		// 3. Trigger BeginPlay on Actor
		// This will call AEcsContext::BeginPlay, which calls Initialize on all elements
		// AND then triggers the BeginPlay ECS Event.
		Trainer->DispatchBeginPlay();

		// Log for debugging if I could see logs
		// UE_LOG(LogTemp, Log, TEXT("Pawns spawned: %d"), PawnCount);
		
		// 4. Verify pawn count and type
		int32 PawnCount = 0;
		for (TActorIterator<ATestVehiclePawn> It(World); It; ++It)
		{
			PawnCount++;
		}

		// Use ASSERT_THAT with AreEqual for clear failure message
		ASSERT_THAT(AreEqual(5, PawnCount, "There should be exactly 5 TestVehiclePawns in the world after BeginPlay"));
	}

	TEST_METHOD(PopulationHasDifferentOutputsAfterInitialization)
	{
		// 1. Setup config
		UVehicleTrainerConfig* Config = NewObject<UVehicleTrainerConfig>();
		Config->VehiclePawnClass = ATestVehiclePawn::StaticClass();
		Config->Population = 4;
		Config->NetworkUpdateFrequencyMS = 100.0f; // Enable timer
		Config->HiddenLayerSizes = { 4 };
		Config->VehicleOutputCount = 2;

		// 2. Spawn Trainer
		AVehicleTrainerContext* Trainer = World->SpawnActor<AVehicleTrainerContext>();
		Trainer->TrainerConfig = Config;

		// 3. Initialize
		Trainer->DispatchBeginPlay();

		// 4. Manually trigger network evaluation
		// We need to ensure inputs are set. By default they might be 0.
		// The issue description says "For now I don't initialise inputs(I initialise them with 1.0 I think)"
		// If they are all 1.0 and weights are the same, outputs will be the same.
		
		// Let's trigger evaluation. 
		// Note: VehicleEntityFactory adds FNNInFloatComp and FNNOutFloatComp.
		// SimpleMLNNFloatFeedforwardSystem reads from FNNInFloatComp and writes to FNNOutFloatComp.
		// VehicleNNOutputSystem reads from FNNOutFloatComp and calls ApplyNNOutputs on the pawn.
		
		Trainer->ExecuteEvent(FName("EvaluateNetworks"));

		// 5. Collect outputs from pawns
		TArray<TArray<float>> AllOutputs;
		for (TActorIterator<ATestVehiclePawn> It(World); It; ++It)
		{
			AllOutputs.Add(It->LastOutputs);
		}

		ASSERT_THAT(AreEqual(4, AllOutputs.Num(), "Should have collected outputs from 4 pawns"));

		// 6. Check if at least some are different
		bool bFoundDifference = false;
		for (int32 i = 0; i < AllOutputs.Num(); ++i)
		{
			for (int32 j = i + 1; j < AllOutputs.Num(); ++j)
			{
				if (AllOutputs[i].Num() != AllOutputs[j].Num())
				{
					bFoundDifference = true;
					break;
				}

				for (int32 k = 0; k < AllOutputs[i].Num(); ++k)
				{
					if (!FMath::IsNearlyEqual(AllOutputs[i][k], AllOutputs[j][k], 0.0001f))
					{
						bFoundDifference = true;
						break;
					}
				}
				if (bFoundDifference) break;
			}
			if (bFoundDifference) break;
		}

		ASSERT_THAT(IsTrue(bFoundDifference, "At least some pawns should have different NN outputs after initialization with unique weights"));
	}
};
