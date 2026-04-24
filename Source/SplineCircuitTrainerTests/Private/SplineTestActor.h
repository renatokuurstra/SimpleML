#pragma once
#include "Components/SplineComponent.h"
#include "SplineTestActor.generated.h"

// Helper actor with a spline component
UCLASS()
class ASplineTestActor : public AActor
{
	GENERATED_BODY()
	
public:
	ASplineTestActor()
	{
		SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
		SetRootComponent(SplineComponent);
	}

	UPROPERTY()
	USplineComponent* SplineComponent;
};