// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "Components/NetworkComponent.h"
#include "SimpleMLNNFloatInitSystem.generated.h"

/**
 * ECS System to initialize neural network weights and biases for entities with FNeuralNetworkFloat.
 * This system has no responsibility over input/output buffers; it only sets up parameters once.
 */
UCLASS()
class SIMPLEML_API USimpleMLNNFloatInitSystem : public UEcsSystem
{
	GENERATED_BODY()
public:
	USimpleMLNNFloatInitSystem()
	{
		RegisterComponent<FNeuralNetworkFloat>();
	}

	virtual void Update(float DeltaTime = 0.0f) override;
};
