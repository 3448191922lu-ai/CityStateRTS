// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyPawn.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "StrategyMapDefinition.h"
#include "StrategySystems.h"

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
	Camera->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
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
	Camera->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
	const FStrategySkirmishMapDefinition MapDefinition = FStrategyMapDefinitions::Resolve(GetWorld()->GetMapName());
	CameraBoundsMin = MapDefinition.CameraMin;
	CameraBoundsMax = MapDefinition.CameraMax;
	SetActorLocation(FVector(-8800.0f, 1800.0f, 4500.0f));
}

void AStrategyPawn::SetZoomModifier(float Value)
{
	// set the ortho width on the camera
	Camera->SetOrthoWidth(Value);
}

void AStrategyPawn::RotateCameraYaw(float DeltaYaw, float ViewportAspectRatio)
{
	const float DesiredYaw = FMath::UnwindDegrees(CameraYaw + DeltaYaw);
	SetActorLocation(FStrategyCameraMovement::GetOrbitCameraLocation(
		GetActorLocation(), CameraPitch, CameraYaw, DesiredYaw));
	CameraYaw = DesiredYaw;
	Camera->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
	ClampToMapBounds(ViewportAspectRatio);
}

void AStrategyPawn::SetGroundFocus(const FVector2D& DesiredFocus, float ViewportAspectRatio)
{
	const FVector ActorLocation = GetActorLocation();
	const FVector CameraForward = Camera->GetForwardVector();
	const float DistanceToGround = ActorLocation.Z / -CameraForward.Z;
	const FVector GroundFocus = ActorLocation + CameraForward * DistanceToGround;
	AddActorWorldOffset(FVector(DesiredFocus.X - GroundFocus.X, DesiredFocus.Y - GroundFocus.Y, 0.0f));
	ClampToMapBounds(ViewportAspectRatio);
}

void AStrategyPawn::ClampToMapBounds(float ViewportAspectRatio)
{
	const FVector ActorLocation = GetActorLocation();
	const FVector CameraForward = Camera->GetForwardVector();
	const float DistanceToGround = ActorLocation.Z / -CameraForward.Z;
	const FVector GroundFocus = ActorLocation + CameraForward * DistanceToGround;
	const FVector2D ViewExtents = FStrategyCameraBoundsRules::GetGroundViewExtents(
		Camera->OrthoWidth, ViewportAspectRatio, FMath::Abs(CameraPitch), CameraYaw);
	const FVector2D ClampedFocus = FStrategyCameraBoundsRules::ClampGroundFocus(
		FVector2D(GroundFocus.X, GroundFocus.Y), CameraBoundsMin, CameraBoundsMax, ViewExtents, CameraBoundsPadding);
	AddActorWorldOffset(FVector(ClampedFocus.X - GroundFocus.X, ClampedFocus.Y - GroundFocus.Y, 0.0f));
}
