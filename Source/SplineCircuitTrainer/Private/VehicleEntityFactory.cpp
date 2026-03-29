//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "VehicleEntityFactory.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "VehicleComponent.h"
#include "Components/NNIOComponents.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UVehicleEntityFactory::UVehicleEntityFactory()
{
	RegisterComponent<FVehicleComponent>();
	RegisterComponent<FNNInFLoatComp>();
	RegisterComponent<FNNOutFloatComp>();
}

void UVehicleEntityFactory::Initialize(AEcsContext* InContext, entt::registry& InRegistry)
{
	Super::Initialize(InContext, InRegistry);

	AVehicleTrainerContext* Context = Cast<AVehicleTrainerContext>(InContext);
	if (!Context || !Context->TrainerConfig || !Context->TrainerConfig->VehiclePawnClass)
	{
		return;
	}

	UWorld* World = Context->GetWorld();
	if (!World)
	{
		return;
	}

	int32 Population = Context->TrainerConfig->Population;
	TSubclassOf<APawn> PawnClass = Context->TrainerConfig->VehiclePawnClass;

	for (int32 i = 0; i < Population; ++i)
	{
		// Create ECS entity
		entt::entity Entity = InRegistry.create();

		// Spawn Pawn
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Context;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		FVector SpawnLocation = Context->GetActorLocation();
		FRotator SpawnRotation = Context->GetActorRotation();

		if (USplineComponent* Spline = Context->GetCircuitSpline())
		{
			SpawnLocation = Spline->GetLocationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
			SpawnRotation = Spline->GetRotationAtDistanceAlongSpline(0.0f, ESplineCoordinateSpace::World);
		}

		APawn* NewPawn = World->SpawnActor<APawn>(PawnClass, SpawnLocation, SpawnRotation, SpawnParams);

		if (NewPawn)
		{
			// Add VehicleComponent to entity with pawn reference
			InRegistry.emplace<FVehicleComponent>(Entity, NewPawn);

			// Add NN Input and Output components
			InRegistry.emplace<FNNInFLoatComp>(Entity);
			InRegistry.emplace<FNNOutFloatComp>(Entity);
		}
	}
}
