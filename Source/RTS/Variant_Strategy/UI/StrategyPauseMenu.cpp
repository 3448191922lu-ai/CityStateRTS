#include "StrategyPauseMenu.h"

#include "StrategyPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ComboBoxString.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetSystemLibrary.h"

UStrategyPauseMenu::UStrategyPauseMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

EStrategyPauseMenuEscapeAction FStrategyPauseMenuRules::ResolveEscape(EStrategyPauseMenuPage Page)
{
	return Page == EStrategyPauseMenuPage::Settings
		? EStrategyPauseMenuEscapeAction::DiscardAndReturn
		: EStrategyPauseMenuEscapeAction::CloseMenu;
}

void FStrategyPauseMenuRules::NormalizeResolutions(TArray<FIntPoint>& Resolutions, const FIntPoint& CurrentResolution)
{
	Resolutions.AddUnique(CurrentResolution);
	for (int32 Index = Resolutions.Num() - 1; Index >= 0; --Index)
	{
		if (Resolutions[Index].X <= 0 || Resolutions[Index].Y <= 0)
		{
			Resolutions.RemoveAt(Index);
		}
	}
	for (int32 Index = Resolutions.Num() - 1; Index >= 0; --Index)
	{
		for (int32 Earlier = 0; Earlier < Index; ++Earlier)
		{
			if (Resolutions[Earlier] == Resolutions[Index])
			{
				Resolutions.RemoveAt(Index);
				break;
			}
		}
	}
	Resolutions.Sort([](const FIntPoint& Left, const FIntPoint& Right)
	{
		return static_cast<int64>(Left.X) * Left.Y < static_cast<int64>(Right.X) * Right.Y;
	});
}

void UStrategyPauseMenu::InitializeForController(AStrategyPlayerController* InController)
{
	Controller = InController;
}

UTextBlock* UStrategyPauseMenu::AddText(UVerticalBox* Parent, const FString& Text, int32 Size, const FLinearColor& Color)
{
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
	Label->SetText(FText::FromString(Text));
	Label->SetColorAndOpacity(Color);
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = Size;
	Label->SetFont(Font);
	Label->SetJustification(ETextJustify::Center);
	UVerticalBoxSlot* LabelSlot = Parent->AddChildToVerticalBox(Label);
	LabelSlot->SetPadding(FMargin(8.0f));
	LabelSlot->SetHorizontalAlignment(HAlign_Fill);
	return Label;
}

UButton* UStrategyPauseMenu::AddButton(UVerticalBox* Parent, const FString& Text)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
	Label->SetText(FText::FromString(Text));
	Label->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Label->GetFont();
	Font.Size = 23;
	Label->SetFont(Font);
	Button->AddChild(Label);
	UVerticalBoxSlot* ButtonSlot = Parent->AddChildToVerticalBox(Button);
	ButtonSlot->SetPadding(FMargin(18.0f, 7.0f));
	ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
	return Button;
}

void UStrategyPauseMenu::BuildPausePage()
{
	PausePage = WidgetTree->ConstructWidget<UVerticalBox>();
	PageSwitcher->AddChild(PausePage);
	AddText(PausePage, TEXT("城邦争霸"), 34, FLinearColor(1.0f, 0.78f, 0.22f, 1.0f));

	UButton* ContinueButton = AddButton(PausePage, TEXT("继续游戏"));
	UButton* SettingsButton = AddButton(PausePage, TEXT("设置"));
	UButton* QuitButton = AddButton(PausePage, TEXT("退出游戏"));
	ContinueButton->OnClicked.AddDynamic(this, &UStrategyPauseMenu::HandleContinueClicked);
	SettingsButton->OnClicked.AddDynamic(this, &UStrategyPauseMenu::HandleSettingsClicked);
	QuitButton->OnClicked.AddDynamic(this, &UStrategyPauseMenu::HandleQuitClicked);
}

void UStrategyPauseMenu::BuildSettingsPage()
{
	SettingsPage = WidgetTree->ConstructWidget<UVerticalBox>();
	PageSwitcher->AddChild(SettingsPage);
	AddText(SettingsPage, TEXT("设置"), 32, FLinearColor(1.0f, 0.78f, 0.22f, 1.0f));

	auto AddSettingRow = [this](const FString& LabelText, UWidget* Control)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		UVerticalBoxSlot* RowSlot = SettingsPage->AddChildToVerticalBox(Row);
		RowSlot->SetPadding(FMargin(16.0f, 8.0f));

		USizeBox* LabelSize = WidgetTree->ConstructWidget<USizeBox>();
		LabelSize->SetWidthOverride(140.0f);
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetText(FText::FromString(LabelText));
		FSlateFontInfo Font = Label->GetFont();
		Font.Size = 20;
		Label->SetFont(Font);
		LabelSize->AddChild(Label);
		Row->AddChildToHorizontalBox(LabelSize);

		UHorizontalBoxSlot* ControlSlot = Row->AddChildToHorizontalBox(Control);
		ControlSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ControlSlot->SetVerticalAlignment(VAlign_Center);
	};

	UHorizontalBox* VolumeControls = WidgetTree->ConstructWidget<UHorizontalBox>();
	MasterVolumeSlider = WidgetTree->ConstructWidget<USlider>();
	MasterVolumeSlider->SetStepSize(0.05f);
	MasterVolumeSlider->OnValueChanged.AddDynamic(this, &UStrategyPauseMenu::HandleMasterVolumeChanged);
	UHorizontalBoxSlot* SliderSlot = VolumeControls->AddChildToHorizontalBox(MasterVolumeSlider);
	SliderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	MasterVolumeValue = WidgetTree->ConstructWidget<UTextBlock>();
	USizeBox* VolumeValueSize = WidgetTree->ConstructWidget<USizeBox>();
	VolumeValueSize->SetWidthOverride(70.0f);
	VolumeValueSize->AddChild(MasterVolumeValue);
	VolumeControls->AddChildToHorizontalBox(VolumeValueSize);
	AddSettingRow(TEXT("总音量"), VolumeControls);

	WindowModeCombo = WidgetTree->ConstructWidget<UComboBoxString>();
	WindowModeCombo->AddOption(TEXT("窗口"));
	WindowModeCombo->AddOption(TEXT("无边框全屏"));
	WindowModeCombo->AddOption(TEXT("独占全屏"));
	WindowModeCombo->OnSelectionChanged.AddDynamic(this, &UStrategyPauseMenu::HandleWindowModeChanged);
	AddSettingRow(TEXT("窗口模式"), WindowModeCombo);

	ResolutionCombo = WidgetTree->ConstructWidget<UComboBoxString>();
	ResolutionCombo->OnSelectionChanged.AddDynamic(this, &UStrategyPauseMenu::HandleResolutionChanged);
	AddSettingRow(TEXT("分辨率"), ResolutionCombo);

	QualityCombo = WidgetTree->ConstructWidget<UComboBoxString>();
	QualityCombo->AddOption(TEXT("低"));
	QualityCombo->AddOption(TEXT("中"));
	QualityCombo->AddOption(TEXT("高"));
	QualityCombo->AddOption(TEXT("史诗"));
	QualityCombo->OnSelectionChanged.AddDynamic(this, &UStrategyPauseMenu::HandleQualityChanged);
	AddSettingRow(TEXT("画质预设"), QualityCombo);

	MinimapOrientationCombo = WidgetTree->ConstructWidget<UComboBoxString>();
	MinimapOrientationCombo->AddOption(TEXT("固定北向"));
	MinimapOrientationCombo->AddOption(TEXT("跟随摄像机"));
	MinimapOrientationCombo->OnSelectionChanged.AddDynamic(this, &UStrategyPauseMenu::HandleMinimapOrientationChanged);
	AddSettingRow(TEXT("小地图方向"), MinimapOrientationCombo);

	UButton* ApplyButton = AddButton(SettingsPage, TEXT("应用并返回"));
	UButton* DiscardButton = AddButton(SettingsPage, TEXT("放弃并返回"));
	ApplyButton->OnClicked.AddDynamic(this, &UStrategyPauseMenu::HandleApplyClicked);
	DiscardButton->OnClicked.AddDynamic(this, &UStrategyPauseMenu::HandleDiscardClicked);
}

TSharedRef<SWidget> UStrategyPauseMenu::RebuildWidget()
{
	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>();
	WidgetTree->RootWidget = Root;

	UBorder* Shade = WidgetTree->ConstructWidget<UBorder>();
	Shade->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.58f));
	UOverlaySlot* ShadeSlot = Root->AddChildToOverlay(Shade);
	ShadeSlot->SetHorizontalAlignment(HAlign_Fill);
	ShadeSlot->SetVerticalAlignment(VAlign_Fill);

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(560.0f);
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetPadding(FMargin(24.0f));
	Panel->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.07f, 0.97f));
	PanelSize->AddChild(Panel);
	UOverlaySlot* PanelSlot = Root->AddChildToOverlay(PanelSize);
	PanelSlot->SetHorizontalAlignment(HAlign_Center);
	PanelSlot->SetVerticalAlignment(VAlign_Center);

	PageSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>();
	Panel->AddChild(PageSwitcher);
	BuildPausePage();
	BuildSettingsPage();
	OpenPausePage();
	return Super::RebuildWidget();
}

void UStrategyPauseMenu::OpenPausePage()
{
	CurrentPage = EStrategyPauseMenuPage::Pause;
	if (PageSwitcher)
	{
		PageSwitcher->SetActiveWidget(PausePage);
	}
}

void UStrategyPauseMenu::OpenSettingsPage()
{
	Draft = UStrategyGameUserSettings::Get()->MakeDraft();
	OriginalMinimapOrientation = Draft.MinimapOrientation;
	RefreshSettingsControls();
	CurrentPage = EStrategyPauseMenuPage::Settings;
	PageSwitcher->SetActiveWidget(SettingsPage);
}

void UStrategyPauseMenu::RefreshSettingsControls()
{
	bUpdatingControls = true;
	MasterVolumeSlider->SetValue(Draft.MasterVolume);
	MasterVolumeValue->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Draft.MasterVolume * 100.0f))));
	WindowModeCombo->SetSelectedIndex(FStrategySettingsRules::WindowModeValueToIndex(Draft.WindowMode));
	QualityCombo->SetSelectedIndex(FMath::Clamp(Draft.OverallQuality, 0, 3));
	MinimapOrientationCombo->SetSelectedIndex(FStrategySettingsRules::MinimapOrientationToIndex(Draft.MinimapOrientation));

	TArray<FIntPoint> Resolutions;
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);
	FStrategyPauseMenuRules::NormalizeResolutions(Resolutions, Draft.Resolution);
	ResolutionCombo->ClearOptions();
	for (const FIntPoint& Resolution : Resolutions)
	{
		ResolutionCombo->AddOption(FStrategySettingsRules::FormatResolution(Resolution));
	}
	ResolutionCombo->SetSelectedOption(FStrategySettingsRules::FormatResolution(Draft.Resolution));
	bUpdatingControls = false;
}

void UStrategyPauseMenu::HandleEscape()
{
	if (FStrategyPauseMenuRules::ResolveEscape(CurrentPage) == EStrategyPauseMenuEscapeAction::DiscardAndReturn)
	{
		HandleDiscardClicked();
	}
	else
	{
		Controller->ClosePauseMenu();
	}
}

FReply UStrategyPauseMenu::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey() == EKeys::Escape)
	{
		HandleEscape();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(Geometry, KeyEvent);
}

void UStrategyPauseMenu::HandleContinueClicked()
{
	Controller->ClosePauseMenu();
}

void UStrategyPauseMenu::HandleSettingsClicked()
{
	OpenSettingsPage();
}

void UStrategyPauseMenu::HandleQuitClicked()
{
	Controller->QuitFromPauseMenu();
}

void UStrategyPauseMenu::HandleApplyClicked()
{
	UStrategyGameUserSettings::Get()->ApplyDraft(Draft);
	OpenPausePage();
}

void UStrategyPauseMenu::HandleDiscardClicked()
{
	UStrategyGameUserSettings::Get()->SetMinimapOrientation(OriginalMinimapOrientation);
	Draft = UStrategyGameUserSettings::Get()->MakeDraft();
	OpenPausePage();
}

void UStrategyPauseMenu::HandleMasterVolumeChanged(float Value)
{
	if (!bUpdatingControls)
	{
		Draft.MasterVolume = FStrategySettingsRules::SnapMasterVolume(Value);
		MasterVolumeSlider->SetValue(Draft.MasterVolume);
		MasterVolumeValue->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Draft.MasterVolume * 100.0f))));
	}
}

void UStrategyPauseMenu::HandleWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (!bUpdatingControls)
	{
		Draft.WindowMode = FStrategySettingsRules::WindowModeIndexToValue(WindowModeCombo->GetSelectedIndex());
	}
}

void UStrategyPauseMenu::HandleResolutionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (!bUpdatingControls)
	{
		FStrategySettingsRules::ParseResolution(SelectedItem, Draft.Resolution);
	}
}

void UStrategyPauseMenu::HandleQualityChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (!bUpdatingControls)
	{
		Draft.OverallQuality = FStrategySettingsRules::QualityIndexToLevel(QualityCombo->GetSelectedIndex());
	}
}

void UStrategyPauseMenu::HandleMinimapOrientationChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (!bUpdatingControls)
	{
		Draft.MinimapOrientation = FStrategySettingsRules::MinimapOrientationFromIndex(
			MinimapOrientationCombo->GetSelectedIndex());
		UStrategyGameUserSettings::Get()->SetMinimapOrientation(Draft.MinimapOrientation);
	}
}
