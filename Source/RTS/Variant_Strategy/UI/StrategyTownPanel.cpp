#include "StrategyTownPanel.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "StrategyGameState.h"
#include "StrategyPlayerController.h"
#include "StrategySystems.h"
#include "StrategyWorldActors.h"

namespace StrategyTownPanelText
{
	FString Owner(EStrategyFaction Faction)
	{
		switch (Faction)
		{
		case EStrategyFaction::Player: return TEXT("我方");
		case EStrategyFaction::Enemy: return TEXT("敌方");
		default: return TEXT("中立");
		}
	}

	FString Specialization(EStrategyTownSpecialization Value)
	{
		switch (Value)
		{
		case EStrategyTownSpecialization::Trade: return TEXT("贸易");
		case EStrategyTownSpecialization::Recruitment: return TEXT("征募");
		case EStrategyTownSpecialization::Fortress: return TEXT("要塞");
		default: return TEXT("未专精");
		}
	}

	FString State(EStrategyTownDevelopmentState Value)
	{
		switch (Value)
		{
		case EStrategyTownDevelopmentState::Building: return TEXT("建设中");
		case EStrategyTownDevelopmentState::Active: return TEXT("已激活");
		case EStrategyTownDevelopmentState::Downgrading: return TEXT("降级中");
		case EStrategyTownDevelopmentState::DisabledAfterCapture: return TEXT("易手瘫痪");
		default: return TEXT("可选择发展方向");
		}
	}
}

void UStrategyTownPanel::InitializeForController(AStrategyPlayerController* InController)
{
	Controller = InController;
}

UButton* UStrategyTownPanel::AddButton(UHorizontalBox* Parent, const FString& Label)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>();
	Text->SetText(FText::FromString(Label));
	Text->SetJustification(ETextJustify::Center);
	Button->AddChild(Text);
	UHorizontalBoxSlot* ButtonSlot = Parent->AddChildToHorizontalBox(Button);
	ButtonSlot->SetPadding(FMargin(5.0f));
	ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	return Button;
}

TSharedRef<SWidget> UStrategyTownPanel::RebuildWidget()
{
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Root;
	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>();
	PanelSize->SetWidthOverride(760.0f);
	PanelSize->SetHeightOverride(176.0f);
	UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(PanelSize);
	PanelSlot->SetAnchors(FAnchors(0.5f, 1.0f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 1.0f));
	PanelSlot->SetPosition(FVector2D(0.0f, -116.0f));
	PanelSlot->SetAutoSize(FStrategyTownPanelLayoutRules::ShouldUseDesiredSize());

	UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
	Border->SetPadding(FMargin(14.0f, 10.0f));
	Border->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.07f, 0.95f));
	PanelSize->AddChild(Border);
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
	Border->AddChild(Content);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>();
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 22;
	TitleText->SetFont(TitleFont);
	TitleText->SetColorAndOpacity(FLinearColor(1.0f, 0.78f, 0.22f, 1.0f));
	Content->AddChildToVerticalBox(TitleText)->SetHorizontalAlignment(HAlign_Center);
	DetailText = WidgetTree->ConstructWidget<UTextBlock>();
	DetailText->SetJustification(ETextJustify::Center);
	DetailText->SetAutoWrapText(true);
	Content->AddChildToVerticalBox(DetailText)->SetPadding(FMargin(4.0f));

	UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>();
	Content->AddChildToVerticalBox(Actions)->SetPadding(FMargin(0.0f, 4.0f));
	TradeButton = AddButton(Actions, TEXT("贸易  250 金币"));
	RecruitmentButton = AddButton(Actions, TEXT("征募  250 金币"));
	FortressButton = AddButton(Actions, TEXT("要塞  250 金币"));
	DowngradeButton = AddButton(Actions, TEXT("降级  返还 100 金币"));
	TradeButton->OnClicked.AddDynamic(this, &UStrategyTownPanel::ChooseTrade);
	RecruitmentButton->OnClicked.AddDynamic(this, &UStrategyTownPanel::ChooseRecruitment);
	FortressButton->OnClicked.AddDynamic(this, &UStrategyTownPanel::ChooseFortress);
	DowngradeButton->OnClicked.AddDynamic(this, &UStrategyTownPanel::Downgrade);
	return Super::RebuildWidget();
}

void UStrategyTownPanel::ShowTown(AStrategyControlPoint* InTown)
{
	Town = InTown;
	SetVisibility(ESlateVisibility::Visible);
	Refresh();
}

void UStrategyTownPanel::HideTown()
{
	Town = nullptr;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UStrategyTownPanel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Refresh();
}

void UStrategyTownPanel::Refresh()
{
	if (!Town || !Controller || !TitleText)
	{
		return;
	}
	AStrategyGameState* GameState = Controller->GetWorld()->GetGameState<AStrategyGameState>();
	const EStrategyFaction Owner = Town->GetStrategyFaction();
	const bool bVisible = GameState->IsVisibleToFaction(EStrategyFaction::Player, Town->GetActorLocation());
	if (!FStrategyTownVisibilityRules::CanShowPublicDetails(EStrategyFaction::Player, Owner, bVisible))
	{
		HideTown();
		return;
	}

	const FStrategyTownDevelopment& Development = Town->GetTownDevelopment();
	const EStrategyTownSpecialization PublicSpecialization = FStrategyTownVisibilityRules::GetPublicSpecialization(
		EStrategyFaction::Player, Owner, bVisible, Development.Specialization, Development.State);
	TitleText->SetText(FText::FromString(FString::Printf(TEXT("%s城镇 · %s"),
		*StrategyTownPanelText::Owner(Owner), *StrategyTownPanelText::Specialization(PublicSpecialization))));
	FString Details = TEXT("基础：3 金币/秒 + 5 人口");
	if (FStrategyTownVisibilityRules::CanShowLiveDetails(EStrategyFaction::Player, Owner, bVisible))
	{
		const bool bConnected = GameState->IsTownSupplyConnected(Town);
		Details += FString::Printf(TEXT("    补给：%s    状态：%s"), bConnected ? TEXT("已连接") : TEXT("未连接"),
			*StrategyTownPanelText::State(Development.State));
		if (Development.State == EStrategyTownDevelopmentState::Building
			|| Development.State == EStrategyTownDevelopmentState::Downgrading
			|| Development.State == EStrategyTownDevelopmentState::DisabledAfterCapture)
		{
			Details += FString::Printf(TEXT(" %.0f%%"), Town->GetDevelopmentProgress() * 100.0f);
		}
		if (Development.State == EStrategyTownDevelopmentState::Active)
		{
			switch (Development.Specialization)
			{
			case EStrategyTownSpecialization::Trade:
				Details += bConnected ? TEXT("    效果：+6 金币/秒") : TEXT("    效果：+4 金币/秒");
				break;
			case EStrategyTownSpecialization::Recruitment:
				Details += bConnected ? TEXT("    效果：+10 人口，区域训练 -30%") : TEXT("    效果：+10 人口，区域训练 -20%");
				break;
			case EStrategyTownSpecialization::Fortress:
				Details += bConnected ? TEXT("    效果：1500 射程 / 25 伤害 / 15 秒占领") : TEXT("    效果：1200 射程 / 20 伤害 / 15 秒占领");
				break;
			default:
				break;
			}
		}
	}
	DetailText->SetText(FText::FromString(Details));

	const bool bCanChoose = FStrategyTownActionRules::CanChooseSpecialization(
		EStrategyFaction::Player, Owner, Town->IsCapital(), Development.State);
	TradeButton->SetVisibility(bCanChoose ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	RecruitmentButton->SetVisibility(bCanChoose ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	FortressButton->SetVisibility(bCanChoose ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	DowngradeButton->SetVisibility(FStrategyTownActionRules::CanDowngrade(
		EStrategyFaction::Player, Owner, Town->IsCapital(), Development.State)
		? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UStrategyTownPanel::ChooseTrade() { Controller->TrySpecializeSelectedTown(EStrategyTownSpecialization::Trade); }
void UStrategyTownPanel::ChooseRecruitment() { Controller->TrySpecializeSelectedTown(EStrategyTownSpecialization::Recruitment); }
void UStrategyTownPanel::ChooseFortress() { Controller->TrySpecializeSelectedTown(EStrategyTownSpecialization::Fortress); }
void UStrategyTownPanel::Downgrade() { Controller->TryDowngradeSelectedTown(); }
