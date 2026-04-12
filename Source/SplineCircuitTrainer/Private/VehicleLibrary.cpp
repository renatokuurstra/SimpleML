//Copyright (c) 2025 Renato Kuurstra. Licensed under the MIT License. See LICENSE file in the project root for details.

#include "VehicleLibrary.h"
#include "Components/SplineComponent.h"
#include "Components/TrainingDataComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"

void UVehicleLibrary::GetVehicleSpawnTransform(const USplineComponent* Spline, float Distance, float VerticalOffset, FVector& OutLocation, FRotator& OutRotation)
{
	if (!Spline) return;

	OutLocation = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
	OutRotation = Spline->GetRotationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);

	if (VerticalOffset != 0.0f)
	{
		FVector UpVector = Spline->GetUpVectorAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
		OutLocation += UpVector * VerticalOffset;
	}
}

void UVehicleLibrary::ResetPawnPhysicalState(APawn* Pawn, const FVector& Location, const FRotator& Rotation)
{
	if (!Pawn) return;

	Pawn->SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::ResetPhysics);

	if (UPawnMovementComponent* Movement = Pawn->GetMovementComponent())
	{
		Movement->StopMovementImmediately();
	}

	// Also reset primitive component physics if applicable
	if (UPrimitiveComponent* RootPrim = Cast<UPrimitiveComponent>(Pawn->GetRootComponent()))
	{
		RootPrim->SetPhysicsLinearVelocity(FVector::ZeroVector);
		RootPrim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	}
}

void UVehicleLibrary::SetTrainingData(FTrainingDataComponent& OutTrainingData, const USplineComponent* Spline, const FVector& Location, float CreationTime)
{
	OutTrainingData.DistanceTraveled = 0.0f;
	OutTrainingData.MaxDistanceTraveled = 0.0f;
	OutTrainingData.TimeSinceLastProgress = 0.0f;
	OutTrainingData.CreationTime = CreationTime;

	if (Spline)
	{
		OutTrainingData.LastSplineDistance = Spline->GetDistanceAlongSplineAtLocation(Location, ESplineCoordinateSpace::World);
		float InputKey = Spline->FindInputKeyClosestToWorldLocation(Location);
		OutTrainingData.LastSplineSegment = FMath::FloorToInt(InputKey);
	}
	else
	{
		OutTrainingData.LastSplineDistance = 0.0f;
		OutTrainingData.LastSplineSegment = 0;
	}
}
