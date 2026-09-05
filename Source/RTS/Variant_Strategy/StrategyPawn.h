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

	/** 按固定步进旋转斜俯视镜头，保证旋转后仍保持同一俯角。 */
	void RotateCameraYaw(float DeltaYaw);
	float GetCameraYaw() const { return CameraYaw; }

	/** Returns the camera component */
	UCameraComponent* GetCamera() const { return Camera; }

private:
	float CameraYaw = -45.0f;
};
