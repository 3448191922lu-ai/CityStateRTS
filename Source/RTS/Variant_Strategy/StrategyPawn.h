// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "StrategyPawn.generated.h"

class UCameraComponent;
class UFloatingPawnMovement;

/**
 *  Simple pawn that implements a top-down camera perspective for a strategy game.
 *  Units are indirectly controlled by other means.
 */
UCLASS()
class AStrategyPawn : public APawn
{
	GENERATED_BODY()

	/** Camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* Camera;

	/** Movement Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UFloatingPawnMovement* FloatingPawnMovement;

public:

	/** Constructor */
	AStrategyPawn();
	virtual void BeginPlay() override;

public:

	/** Sets the camera zoom modifier value */
	void SetZoomModifier(float Value);

	/** 围绕当前地面视野中心旋转斜俯视镜头，并保持同一俯角。 */
	void RotateCameraYaw(float DeltaYaw, float ViewportAspectRatio);
	float GetCameraYaw() const { return CameraYaw; }
	void SetGroundFocus(const FVector2D& DesiredFocus, float ViewportAspectRatio);
	void ClampToMapBounds(float ViewportAspectRatio);

	/** Returns the camera component */
	UCameraComponent* GetCamera() const { return Camera; }

private:
	static constexpr float CameraPitch = -53.0f;
	static constexpr float CameraBoundsPadding = 200.0f;
	float CameraYaw = -45.0f;
	FVector2D CameraBoundsMin = FVector2D(-11000.0f, -9000.0f);
	FVector2D CameraBoundsMax = FVector2D(11000.0f, 9000.0f);
};
