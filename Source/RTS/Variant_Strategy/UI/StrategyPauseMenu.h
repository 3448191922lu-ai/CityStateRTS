#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StrategyGameUserSettings.h"
#include "Types/SlateEnums.h"
#include "StrategyPauseMenu.generated.h"

class AStrategyPlayerController;
class UButton;
class UComboBoxString;
class USlider;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;

UENUM()
enum class EStrategyPauseMenuPage : uint8
{
	Pause,
	Settings
};

enum class EStrategyPauseMenuEscapeAction : uint8
{
	CloseMenu,
	DiscardAndReturn
};

struct FStrategyPauseMenuRules
{
	static EStrategyPauseMenuEscapeAction ResolveEscape(EStrategyPauseMenuPage Page);
	static void NormalizeResolutions(TArray<FIntPoint>& Resolutions, const FIntPoint& CurrentResolution);
};

UCLASS()
class UStrategyPauseMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	UStrategyPauseMenu(const FObjectInitializer& ObjectInitializer);
	void InitializeForController(AStrategyPlayerController* InController);
	void OpenPausePage();
	void OpenSettingsPage();
	void HandleEscape();
	EStrategyPauseMenuPage GetCurrentPage() const { return CurrentPage; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent) override;

private:
	UTextBlock* AddText(UVerticalBox* Parent, const FString& Text, int32 Size, const FLinearColor& Color);
	UButton* AddButton(UVerticalBox* Parent, const FString& Text);
	void BuildPausePage();
	void BuildSettingsPage();
	void RefreshSettingsControls();

	UFUNCTION()
	void HandleContinueClicked();
	UFUNCTION()
	void HandleSettingsClicked();
	UFUNCTION()
	void HandleQuitClicked();
	UFUNCTION()
	void HandleApplyClicked();
	UFUNCTION()
	void HandleDiscardClicked();
	UFUNCTION()
	void HandleMasterVolumeChanged(float Value);
	UFUNCTION()
	void HandleWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void HandleResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void HandleQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void HandleMinimapOrientationChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UPROPERTY(Transient)
	TObjectPtr<AStrategyPlayerController> Controller;
	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> PageSwitcher;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> PausePage;
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> SettingsPage;
	UPROPERTY(Transient)
	TObjectPtr<USlider> MasterVolumeSlider;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MasterVolumeValue;
	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> WindowModeCombo;
	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> ResolutionCombo;
	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> QualityCombo;
	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> MinimapOrientationCombo;

	FStrategySettingsDraft Draft;
	EStrategyMinimapOrientation OriginalMinimapOrientation = EStrategyMinimapOrientation::NorthUp;
	EStrategyPauseMenuPage CurrentPage = EStrategyPauseMenuPage::Pause;
	bool bUpdatingControls = false;
};
