//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/VehicleEntityFactory.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "VehicleComponent.h"
#include "Components/NNIOComponents.h"
#include "Components/SplineComponent.h"
#include "Components/NetworkComponent.h"
#include "Components/GenomeComponents.h"
#include "GameFramework/Pawn.h"
#include "AIController.h"
#include "Engine/World.h"

UVehicleEntityFactory::UVehicleEntityFactory()
{
	RegisterComponent<FVehicleComponent>();
	RegisterComponent<FNNInFLoatComp>();
	RegisterComponent<FNNOutFloatComp>();
	RegisterComponent<FNeuralNetworkFloat>();
	RegisterComponent<FGenomeFloatViewComponent>();
	RegisterComponent<FFitnessComponent>();
}

void UVehicleEntityFactory::Initialize_Implementation(AEcsContext* InContext)
{
	Super::Initialize_Implementation(InContext);
	entt::registry& InRegistry = GetRegistry();

	AVehicleTrainerContext* TrainerContext = Cast<AVehicleTrainerContext>(InContext);
	if (!TrainerContext || !TrainerContext->TrainerConfig || !TrainerContext->TrainerConfig->VehiclePawnClass)
	{
		return;
	}

	UWorld* World = TrainerContext->GetWorld();
	if (!World)
	{
		return;
	}

	int32 Population = TrainerContext->TrainerConfig->Population;
	TSubclassOf<APawn> PawnClass = TrainerContext->TrainerConfig->VehiclePawnClass;

	for (int32 i = 0; i < Population; ++i)
	{
		// Create ECS entity
		entt::entity Entity = InRegistry.create();

		// Spawn Pawn
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = TrainerContext;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		FVector SpawnLocation = TrainerContext->GetActorLocation();
		FRotator SpawnRotation = TrainerContext->GetActorRotation();

		if (USplineComponent* Spline = TrainerContext->GetCircuitSpline())
		{
			SpawnLocation = Spline->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
			SpawnRotation = Spline->GetRotationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
		}

		APawn* NewPawn = World->SpawnActor<APawn>(PawnClass, SpawnLocation, SpawnRotation, SpawnParams);

		if (NewPawn)
		{
			// Spawn AI Controller and possess
			if (AAIController* AIController = World->SpawnActor<AAIController>())
			{
				AIController->Possess(NewPawn);
			}

			// Add VehicleComponent to entity with pawn reference
			InRegistry.emplace<FVehicleComponent>(Entity, NewPawn);

			// Add NN Input and Output components
			FNNInFLoatComp& InComp = InRegistry.emplace<FNNInFLoatComp>(Entity);
			FNNOutFloatComp& OutComp = InRegistry.emplace<FNNOutFloatComp>(Entity);
			FNeuralNetworkFloat& NetComp = InRegistry.emplace<FNeuralNetworkFloat>(Entity);

			// Add Genome and Fitness components
			InRegistry.emplace<FGenomeFloatViewComponent>(Entity);
			FFitnessComponent& FitComp = InRegistry.emplace<FFitnessComponent>(Entity);
			FitComp.Fitness.SetNum(1);
			FitComp.Fitness[0] = 0.0f;
			FitComp.BuiltForFitnessIndex = 0;

	// Initialize Neural Network from config
			TArray<FNeuralNetworkLayerDescriptor> LayerDescriptors = TrainerContext->TrainerConfig->GetNNLayerDescriptors();
			if (LayerDescriptors.Num() >= 2)
			{
				NetComp.Initialize(LayerDescriptors, i);
				
				// Set NN input and output sizes based on descriptors
				InComp.Values.Init(0.5f, TrainerContext->TrainerConfig->GetTotalInputCount());
				OutComp.Values.SetNumZeroed(TrainerContext->TrainerConfig->GetTotalOutputCount());
			}
		}
	}
}
