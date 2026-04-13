//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/EliteSelectionBaseSystem.h"

UEliteSelectionBaseSystem::UEliteSelectionBaseSystem()
{
	RegisterComponent<FFitnessComponent>();
	RegisterComponent<FEliteTagComponent>();
	RegisterComponent<FEligibleForBreedingTagComponent>();
}

void UEliteSelectionBaseSystem::Update_Implementation(float DeltaTime)
{
	ApplySelection();
}
