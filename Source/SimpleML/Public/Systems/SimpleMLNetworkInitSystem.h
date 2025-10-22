// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "Components/NetworkComponent.h"
#include "SimpleMLNetworkInitSystem.generated.h"

/**
 * ECS System to initialize neural network weights/biases and IO buffers for entities with FNetworkComponent.
 * The system checks bIsInitialized and, if false, initializes using the component's InitMin/InitMax.
 */
UCLASS()
class SIMPLEML_API USimpleMLNetworkInitSystem : public UEcsSystem
{
	GENERATED_BODY()
public:
	USimpleMLNetworkInitSystem()
	{
		RegisterComponent<FNetworkComponent>();
	}

	virtual void Update(float DeltaTime = 0.0f) override;
};
