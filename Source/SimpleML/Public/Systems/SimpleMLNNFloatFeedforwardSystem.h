// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "Components/NetworkComponent.h"
#include "Components/NNIOComponents.h"
#include "SimpleMLNNFloatFeedforwardSystem.generated.h"

/**
 * ECS System to execute a feedforward pass over FNetworkComponent using its InputValues -> OutputValues.
 * Note: For now this supports standard feedforward layers only.
 */
UCLASS()
class SIMPLEML_API USimpleMLNNFloatFeedforwardSystem : public UEcsSystem
{
	GENERATED_BODY()
public:
	USimpleMLNNFloatFeedforwardSystem()
	{
		RegisterComponent<FNeuralNetworkFloat>();
		RegisterComponent<FNNInFLoatComp>();
		RegisterComponent<FNNOutFloatComp>();
	}

	virtual void Update(float DeltaTime = 0.0f) override;
};
