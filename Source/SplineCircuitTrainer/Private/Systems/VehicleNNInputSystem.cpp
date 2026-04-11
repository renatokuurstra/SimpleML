//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "Systems/VehicleNNInputSystem.h"
#include "VehicleComponent.h"
#include "Components/NNIOComponents.h"
#include "VehicleTrainerContext.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMathLibrary.h"

UVehicleNNInputSystem::UVehicleNNInputSystem()
{
	RegisterComponent<FVehicleComponent>();
	RegisterComponent<FNNInFLoatComp>();
	RegisterComponent<FNNOutFloatComp>();
}

void UVehicleNNInputSystem::Update_Implementation(float DeltaTime)
{
	AVehicleTrainerContext* TrainerContext = GetTypedContext<AVehicleTrainerContext>();
	if (!TrainerContext)
	{
		return;
	}

	USplineComponent* Spline = TrainerContext->GetCircuitSpline();
	if (!Spline)
	{
		return;
	}

	auto View = GetView<FVehicleComponent, FNNInFLoatComp, FNNOutFloatComp>();

	const int32 TotalExpectedInputs = TrainerContext->TrainerConfig->GetTotalInputCount();
	const int32 RecurrentCount = TrainerContext->TrainerConfig->RecurrentInputCount;

	bool bFirstEntity = true;

	for (auto Entity : View)
	{
		const FVehicleComponent& VehicleComp = View.get<FVehicleComponent>(Entity);
		FNNInFLoatComp& InComp = View.get<FNNInFLoatComp>(Entity);
		const FNNOutFloatComp& OutComp = View.get<FNNOutFloatComp>(Entity);

		if (bFirstEntity)
		{
			if (InComp.Values.Num() < TotalExpectedInputs && !bHasLoggedInputMismatch)
			{
				UE_LOG(LogTemp, Error, TEXT("UVehicleNNInputSystem: Network input mismatch. Expected %d inputs, but got %d."), TotalExpectedInputs, InComp.Values.Num());
				bHasLoggedInputMismatch = true;
			}
			bFirstEntity = false;
		}

		if (!VehicleComp.VehiclePawn || InComp.Values.Num() < TotalExpectedInputs)
		{
			continue;
		}

		APawn* Pawn = VehicleComp.VehiclePawn;
		FVector PawnLocation = Pawn->GetActorLocation();
		FVector PawnForward = Pawn->GetActorForwardVector();

		// 1) Distance to spline (normalized)
		float InputDistance = Spline->GetDistanceAlongSplineAtLocation(PawnLocation, ESplineCoordinateSpace::World);
		FVector ClosestSplineLocation = Spline->GetLocationAtDistanceAlongSpline(InputDistance, ESplineCoordinateSpace::World);
		float DistanceToSpline = FVector::Dist(PawnLocation, ClosestSplineLocation);
		InComp.Values[0] = FMath::Clamp(DistanceToSpline / TrainerContext->TrainerConfig->MaxDistanceNormalization, 0.0f, 1.0f);

		// 2) Dot product current
		FVector SplineTangent = Spline->GetTangentAtDistanceAlongSpline(InputDistance, ESplineCoordinateSpace::World).GetSafeNormal();
		InComp.Values[1] = FVector::DotProduct(PawnForward, SplineTangent);

		// 3 & 4) Future dot products
		int32 InputIndex = 2;
		float SplineLength = Spline->GetSplineLength();
		for (float Offset : TrainerContext->TrainerConfig->FutureDotProductDistances)
		{
			float FutureDistance = InputDistance + Offset;
			// Wrap distance around spline length
			if (Spline->IsClosedLoop())
			{
				FutureDistance = FMath::Fmod(FutureDistance, SplineLength);
				if (FutureDistance < 0.0f) FutureDistance += SplineLength;
			}
			else
			{
				FutureDistance = FMath::Clamp(FutureDistance, 0.0f, SplineLength);
			}

			FVector FutureTangent = Spline->GetTangentAtDistanceAlongSpline(FutureDistance, ESplineCoordinateSpace::World).GetSafeNormal();
			InComp.Values[InputIndex++] = FVector::DotProduct(PawnForward, FutureTangent);
		}

		// 5) Normalized velocity
		float Velocity = Pawn->GetVelocity().Size();
		InComp.Values[InputIndex++] = FMath::Clamp(Velocity / TrainerContext->TrainerConfig->MaxVelocityNormalization, 0.0f, 1.0f);

		// 6) Recurrent inputs
		if (RecurrentCount > 0 && OutComp.Values.Num() >= RecurrentCount)
		{
			int32 OutStartIndex = OutComp.Values.Num() - RecurrentCount;
			for (int32 i = 0; i < RecurrentCount; ++i)
			{
				InComp.Values[InputIndex++] = OutComp.Values[OutStartIndex + i];
			}
		}
	}
}
