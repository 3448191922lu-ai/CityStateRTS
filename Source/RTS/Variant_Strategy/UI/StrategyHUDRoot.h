#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StrategyHUDModel.h"
#include "StrategyHUDRoot.generated.h"

class AStrategyPlayerController;
class UButton;
class UHorizontalBox;
class UOverlay;
class UProgressBar;
class UTextBlock;
class UVerticalBox;
class UWidget;
class USizeBox;
class UStrategyMinimapWidget;

UCLASS()
class UStrategyHUDRoot : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeForController(AStrategyPlayerController* InController);
	void Refresh();
	void PushNotification(const FString& Message, const FLinearColor& Color);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	virtual void NativeDestruct() override;

private:
	void RefreshResources();
	void RefreshContext();
	void RefreshMatchResult();
	void ApplyViewportScale();
	void RefreshNotifications();
	void HandleFactionNotification(EStrategyFaction Faction, const FString& Message);
	void SetStableText(UTextBlock* TextBlock, FName Key, const FString& Value);
	uint32 BuildCommandSignature(EStrategyHUDContext Context) const;
	void ShowIdleContext();
	void ShowSquadContext();
	void ShowBuildingContext();
	void ShowTownContext();
	void ShowBuildContext();
	UButton* AddCommandButton(const FString& Label, FName HandlerName, bool bEnabled = true,
		const FString& UnavailableReason = FString());
	void ClearCommands();

	UFUNCTION() void HandleMoveClicked();
	UFUNCTION() void HandleAttackMoveClicked();
	UFUNCTION() void HandleStopClicked();
	UFUNCTION() void HandleBuildMenuClicked();
	UFUNCTION() void HandlePauseClicked();
	UFUNCTION() void HandleBuildBarracksClicked();
	UFUNCTION() void HandleBuildArcheryClicked();
	UFUNCTION() void HandleBuildStableClicked();
	UFUNCTION() void HandleBuildHouseClicked();
	UFUNCTION() void HandleBuildTowerClicked();
	UFUNCTION() void HandleBuildWallClicked();
	UFUNCTION() void HandleUpgradeGateClicked();
	UFUNCTION() void HandleTrainInfantryClicked();
	UFUNCTION() void HandleTrainArcherClicked();
	UFUNCTION() void HandleTrainCavalryClicked();
	UFUNCTION() void HandleTradeClicked();
	UFUNCTION() void HandleRecruitmentClicked();
	UFUNCTION() void HandleFortressClicked();
	UFUNCTION() void HandleDowngradeClicked();

	UPROPERTY(Transient)
	TObjectPtr<AStrategyPlayerController> Controller;
	UPROPERTY(Transient)
	TObjectPtr<UWidget> ResourceBar;
	UPROPERTY(Transient)
	TObjectPtr<UWidget> BottomBar;
	UPROPERTY(Transient)
	TObjectPtr<USizeBox> MinimapSize;
	UPROPERTY(Transient)
	TObjectPtr<UStrategyMinimapWidget> Minimap;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ResourceText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ObjectText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailText;
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> ObjectProgress;
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> DetailProgress;
	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> CommandBox;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> NotificationBox;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> NotificationTexts;
	UPROPERTY(Transient)
	TObjectPtr<UOverlay> MatchOverlay;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MatchText;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MatchHelpText;
	uint32 LastCommandSignature = MAX_uint32;
	bool bRebuildCommands = true;
	FStrategyHUDNotificationQueue NotificationQueue;
	FStrategyHUDUpdateCache UpdateCache;
	float ResourceWarningRemaining = 0.0f;
};
