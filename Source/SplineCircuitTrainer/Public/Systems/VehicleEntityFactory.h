//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "VehicleEntityFactory.generated.h"

/**
 * UVehicleEntityFactory
 * System that spawns pawns and creates ECS entities for them.
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class SPLINECIRCUITTRAINER_API UVehicleEntityFactory : public UEcsSystem
{
	GENERATED_BODY()

public:
	UVehicleEntityFactory();
	
	virtual void Initialize_Implementation(AEcsContext* InContext) override;
};
