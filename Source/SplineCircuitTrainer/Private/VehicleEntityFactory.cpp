//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "VehicleEntityFactory.h"
#include "VehicleTrainerContext.h"
#include "VehicleTrainerConfig.h"
#include "VehicleComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UVehicleEntityFactory::UVehicleEntityFactory()
{
	RegisterComponent<FVehicleComponent>();
}

void UVehicleEntityFactory::Initialize(entt::registry& InRegistry)
{
	Super::Initialize(InRegistry);

	AVehicleTrainerContext* Context = GetTypedOuter<AVehicleTrainerContext>();
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
		
		APawn* NewPawn = World->SpawnActor<APawn>(PawnClass, Context->GetActorLocation(), Context->GetActorRotation(), SpawnParams);

		if (NewPawn)
		{
			// Add VehicleComponent to entity with pawn reference
			InRegistry.emplace<FVehicleComponent>(Entity, NewPawn);
		}
	}
}
