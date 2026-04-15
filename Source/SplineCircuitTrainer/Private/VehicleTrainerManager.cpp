//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "VehicleTrainerManager.h"
#include "VehicleTrainerContext.h"
#include "Engine/World.h"

AVehicleTrainerManager::AVehicleTrainerManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AVehicleTrainerManager::BeginPlay()
{
	Super::BeginPlay();
	SpawnContexts();
}

void AVehicleTrainerManager::SpawnContexts()
{
	if (!TrainerConfig || !CircuitActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AVehicleTrainerManager] Missing TrainerConfig or CircuitActor. Cannot spawn contexts."));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (int32 i = 0; i < NumContexts; ++i)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Name = FName(*FString::Printf(TEXT("VehicleTrainerContext_%d"), i));
		
		AVehicleTrainerContext* NewContext = World->SpawnActorDeferred<AVehicleTrainerContext>(AVehicleTrainerContext::StaticClass(), FTransform(GetActorRotation(), GetActorLocation()), this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (NewContext)
		{
			NewContext->TrainerConfig = TrainerConfig;
			NewContext->CircuitActor = CircuitActor;
			
			// Assign seed: use ContextSeeds if available, otherwise use index
			if (ContextSeeds.IsValidIndex(i))
			{
				NewContext->RandomSeed = ContextSeeds[i];
			}
			else
			{
				NewContext->RandomSeed = i;
			}

			NewContext->InitializeSystemsFromConfig();
			NewContext->FinishSpawning(FTransform(GetActorRotation(), GetActorLocation()));
			Contexts.Add(NewContext);
		}
	}
}
