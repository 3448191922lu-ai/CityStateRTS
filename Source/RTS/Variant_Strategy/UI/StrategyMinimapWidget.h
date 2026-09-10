#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StrategyMapDefinition.h"
#include "StrategyTypes.h"
#include "StrategyMinimapWidget.generated.h"

class AStrategyControlPoint;
class AStrategyPlayerController;

struct FStrategyMinimapIcon
{
	TWeakObjectPtr<AActor> TargetActor;
	FVector2D LocalPosition = FVector2D::ZeroVector;
	FLinearColor Color = FLinearColor::White;
	float Radius = 4.0f;
	bool bEnemyTarget = false;
	bool bGarrisoned = false;
};

UCLASS()
class UStrategyMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeForController(AStrategyPlayerController* InController);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
		const FSlateRect& CullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& Event) override;

private:
	void RefreshIcons(const FVector2D& LocalSize);
	void MoveCameraAtPointer(const FGeometry& Geometry, const FPointerEvent& Event);
	AActor* FindVisibleEnemyAt(const FVector2D& LocalPosition) const;
	FVector2D WorldToLocal(const FVector& World, const FVector2D& LocalSize) const;
	FVector2D LocalToWorld(const FVector2D& Local, const FVector2D& LocalSize) const;

	UPROPERTY(Transient)
	TObjectPtr<AStrategyPlayerController> Controller;

	FStrategySkirmishMapDefinition MapDefinition;
	TArray<FStrategyMinimapIcon> Icons;
	TMap<TWeakObjectPtr<AStrategyControlPoint>, EStrategyFaction> ObservedTownOwners;
	float InitialCameraYaw = -45.0f;
	bool bDraggingCamera = false;
};
