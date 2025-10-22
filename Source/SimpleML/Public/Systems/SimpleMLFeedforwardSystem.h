// Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.
#pragma once

#include "CoreMinimal.h"
#include "EcsSystem.h"
#include "Components/NetworkComponent.h"
#include "SimpleMLFeedforwardSystem.generated.h"

/**
 * ECS System to execute a feedforward pass over FNetworkComponent using its InputValues -> OutputValues.
 * Note: For now this supports standard feedforward layers only.
 */
UCLASS()
class SIMPLEML_API USimpleMLFeedforwardSystem : public UEcsSystem
{
	GENERATED_BODY()
public:
	USimpleMLFeedforwardSystem()
	{
		RegisterComponent<FNetworkComponent>();
	}

	virtual void Update(float DeltaTime = 0.0f) override;
};
