// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyPawn.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

AStrategyPawn::AStrategyPawn()
{
 	PrimaryActorTick.bCanEverTick = true;

	// create the root
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// create the camera
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(RootComponent);

	// create the movement component
	FloatingPawnMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Floating Pawn Movement"));

	// configure the camera
	Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
	Camera->OrthoWidth = 7000.0f;
	CameraYaw = -45.0f;
	Camera->SetRelativeRotation(FRotator(-60.0f, CameraYaw, 0.0f));
	Camera->AutoPlaneShift = 1.0f;
	Camera->bUpdateOrthoPlanes = false;

	// configure the movement comp
	FloatingPawnMovement->bConstrainToPlane = true;
	FloatingPawnMovement->SetPlaneConstraintNormal(FVector::UpVector);
	FloatingPawnMovement->SetPlaneConstraintOrigin(FVector::UpVector * 4500.0f);
	FloatingPawnMovement->MaxSpeed = 2500.0f;
}

void AStrategyPawn::BeginPlay()
{
	Super::BeginPlay();
	Camera->ProjectionMode = ECameraProjectionMode::Orthographic;
	Camera->SetOrthoWidth(7000.0f);
	CameraYaw = -45.0f;
	Camera->SetRelativeRotation(FRotator(-60.0f, CameraYaw, 0.0f));
	SetActorLocation(FVector(-8800.0f, 1800.0f, 4500.0f));
}

void AStrategyPawn::SetZoomModifier(float Value)
{
	// set the ortho width on the camera
	Camera->SetOrthoWidth(Value);
}

void AStrategyPawn::RotateCameraYaw(float DeltaYaw)
{
	CameraYaw = FMath::UnwindDegrees(CameraYaw + DeltaYaw);
	Camera->SetRelativeRotation(FRotator(-60.0f, CameraYaw, 0.0f));
}
