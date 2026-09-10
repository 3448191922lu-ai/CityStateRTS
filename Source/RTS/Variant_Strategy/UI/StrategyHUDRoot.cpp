#include "StrategyHUDRoot.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "StrategyGameState.h"
#include "StrategyPlayerController.h"
#include "StrategySystems.h"
#include "StrategyWorldActors.h"
#include "StrategyMinimapModel.h"
#include "StrategyMinimapWidget.h"
#include "Brushes/SlateColorBrush.h"

namespace StrategyHUDText
{
	FString Unit(EStrategyUnitType Type)
	{
		switch (Type)
		{
		case EStrategyUnitType::Infantry: return TEXT("步兵");
		case EStrategyUnitType::Archer: return TEXT("弓兵");
		case EStrategyUnitType::Cavalry: return TEXT("骑兵");
		default: return TEXT("小队");
		}
	}

	FString Building(EStrategyBuildingType Type)
	{
		switch (Type)
		{
		case EStrategyBuildingType::Barracks: return TEXT("兵营");
		case EStrategyBuildingType::ArcheryRange: return TEXT("靶场");
		case EStrategyBuildingType::Stable: return TEXT("马厩");
		case EStrategyBuildingType::House: return TEXT("民居");
		case EStrategyBuildingType::Tower: return TEXT("箭塔");
		case EStrategyBuildingType::Wall: return TEXT("城墙");
		case EStrategyBuildingType::Gate: return TEXT("城门");
		case EStrategyBuildingType::Capital: return TEXT("主城");
		default: return TEXT("建筑");
		}
	}

	FString Owner(EStrategyFaction Faction)
	{
		return Faction == EStrategyFaction::Player ? TEXT("我方")
			: Faction == EStrategyFaction::Enemy ? TEXT("敌方") : TEXT("中立");
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

	FString DevelopmentState(EStrategyTownDevelopmentState Value)
	{
		switch (Value)
		{
		case EStrategyTownDevelopmentState::Building: return TEXT("建设中");
		case EStrategyTownDevelopmentState::Active: return TEXT("已激活");
		case EStrategyTownDevelopmentState::Downgrading: return TEXT("降级中");
		case EStrategyTownDevelopmentState::DisabledAfterCapture: return TEXT("等待驻军重启");
		default: return TEXT("可选择专精");
		}
	}
}

namespace StrategyHUDStyle
{
	const FLinearColor Panel(0.035f, 0.09f, 0.13f, 0.96f);
	const FLinearColor Divider(0.25f, 0.36f, 0.42f, 1.0f);
	const FLinearColor Gold(0.84f, 0.65f, 0.26f, 1.0f);
	const FLinearColor Text(0.92f, 0.94f, 0.91f, 1.0f);
	const FLinearColor Secondary(0.65f, 0.72f, 0.75f, 1.0f);
}

namespace
{
	UTextBlock* MakeText(UWidgetTree* Tree, int32 Size, const FLinearColor& Color)
	{
		UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(Color);
		Text->SetAutoWrapText(true);
		return Text;
	}

	UBorder* MakePanel(UWidgetTree* Tree, const FMargin& Padding)
	{
		UBorder* Border = Tree->ConstructWidget<UBorder>();
		Border->SetPadding(Padding);
		Border->SetBrushColor(StrategyHUDStyle::Panel);
		return Border;
	}

	void SetFill(UHorizontalBoxSlot* Slot, float Value)
	{
		FSlateChildSize Size;
		Size.SizeRule = ESlateSizeRule::Fill;
		Size.Value = Value;
		Slot->SetSize(Size);
	}
}

void UStrategyHUDRoot::InitializeForController(AStrategyPlayerController* InController)
{
	Controller = InController;
	Controller->GetWorld()->GetGameState<AStrategyGameState>()->OnFactionNotification().AddUObject(
		this, &UStrategyHUDRoot::HandleFactionNotification);
	Refresh();
}

TSharedRef<SWidget> UStrategyHUDRoot::RebuildWidget()
{
	UpdateCache.Reset();
	LastCommandSignature = MAX_uint32;
	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Root;

	USizeBox* ResourceSize = WidgetTree->ConstructWidget<USizeBox>();
	ResourceSize->SetWidthOverride(620.0f);
	ResourceSize->SetHeightOverride(54.0f);
	UCanvasPanelSlot* ResourceSlot = Root->AddChildToCanvas(ResourceSize);
	ResourceSlot->SetAnchors(FAnchors(0.5f, 0.0f));
	ResourceSlot->SetAlignment(FVector2D(0.5f, 0.0f));
	ResourceSlot->SetPosition(FVector2D::ZeroVector);
	ResourceSlot->SetAutoSize(true);
	ResourceBar = ResourceSize;
	ResourceSize->SetRenderTransformPivot(FVector2D(0.5f, 0.0f));

	UBorder* ResourceBorder = MakePanel(WidgetTree, FMargin(16.0f, 8.0f));
	ResourceSize->AddChild(ResourceBorder);
	ResourceText = MakeText(WidgetTree, 18, StrategyHUDStyle::Text);
	// 动态数字使用固定左起点，避免字宽变化时整行重新居中跳动。
	ResourceText->SetAutoWrapText(false);
	ResourceText->SetJustification(ETextJustify::Left);
	ResourceBorder->AddChild(ResourceText);

	USizeBox* NoticeSize = WidgetTree->ConstructWidget<USizeBox>();
	NoticeSize->SetWidthOverride(560.0f);
	NoticeSize->SetHeightOverride(110.0f);
	UCanvasPanelSlot* NoticeSlot = Root->AddChildToCanvas(NoticeSize);
	NoticeSlot->SetAnchors(FAnchors(0.5f, 0.0f));
	NoticeSlot->SetAlignment(FVector2D(0.5f, 0.0f));
	NoticeSlot->SetPosition(FVector2D(0.0f, 62.0f));
	NoticeSlot->SetAutoSize(true);
	NotificationBox = WidgetTree->ConstructWidget<UVerticalBox>();
	NoticeSize->AddChild(NotificationBox);
	NotificationTexts.Reset();
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UTextBlock* Notice = MakeText(WidgetTree, 17, StrategyHUDStyle::Text);
		Notice->SetJustification(ETextJustify::Center);
		Notice->SetVisibility(ESlateVisibility::Collapsed);
		NotificationBox->AddChildToVerticalBox(Notice)->SetPadding(FMargin(0.0f, 2.0f));
		NotificationTexts.Add(Notice);
	}

	UBorder* BottomBorder = MakePanel(WidgetTree, FMargin(0.0f));
	BottomBorder->SetRenderTransformPivot(FVector2D(0.5f, 1.0f));
	UCanvasPanelSlot* BottomSlot = Root->AddChildToCanvas(BottomBorder);
	BottomSlot->SetAnchors(FAnchors(0.0f, 1.0f, 1.0f, 1.0f));
	BottomSlot->SetOffsets(FMargin(20.0f, -124.0f, -20.0f, 104.0f));
	BottomBar = BottomBorder;

	UHorizontalBox* Columns = WidgetTree->ConstructWidget<UHorizontalBox>();
	BottomBorder->AddChild(Columns);

	UBorder* ObjectBorder = MakePanel(WidgetTree, FMargin(14.0f, 10.0f));
	ObjectBorder->SetBrushColor(FLinearColor(0.04f, 0.11f, 0.16f, 1.0f));
	UHorizontalBoxSlot* ObjectSlot = Columns->AddChildToHorizontalBox(ObjectBorder);
	SetFill(ObjectSlot, 0.24f);
	ObjectText = MakeText(WidgetTree, 17, StrategyHUDStyle::Text);
	UVerticalBox* ObjectColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	ObjectBorder->AddChild(ObjectColumn);
	ObjectColumn->AddChildToVerticalBox(ObjectText);
	ObjectProgress = WidgetTree->ConstructWidget<UProgressBar>();
	ObjectColumn->AddChildToVerticalBox(ObjectProgress)->SetPadding(FMargin(0.0f, 7.0f, 0.0f, 0.0f));

	UBorder* DetailBorder = MakePanel(WidgetTree, FMargin(14.0f, 10.0f));
	DetailBorder->SetBrushColor(FLinearColor(0.035f, 0.09f, 0.13f, 1.0f));
	UHorizontalBoxSlot* DetailSlot = Columns->AddChildToHorizontalBox(DetailBorder);
	SetFill(DetailSlot, 0.44f);
	DetailText = MakeText(WidgetTree, 16, StrategyHUDStyle::Secondary);
	UVerticalBox* DetailColumn = WidgetTree->ConstructWidget<UVerticalBox>();
	DetailBorder->AddChild(DetailColumn);
	DetailColumn->AddChildToVerticalBox(DetailText);
	DetailProgress = WidgetTree->ConstructWidget<UProgressBar>();
	DetailColumn->AddChildToVerticalBox(DetailProgress)->SetPadding(FMargin(0.0f, 7.0f, 0.0f, 0.0f));

	UBorder* CommandBorder = MakePanel(WidgetTree, FMargin(10.0f, 8.0f));
	CommandBorder->SetBrushColor(FLinearColor(0.04f, 0.11f, 0.16f, 1.0f));
	UHorizontalBoxSlot* CommandSlot = Columns->AddChildToHorizontalBox(CommandBorder);
	SetFill(CommandSlot, 0.32f);
	CommandBox = WidgetTree->ConstructWidget<UHorizontalBox>();
	CommandBorder->AddChild(CommandBox);

	MinimapSize = WidgetTree->ConstructWidget<USizeBox>();
	MinimapSize->SetWidthOverride(300.0f);
	MinimapSize->SetHeightOverride(240.0f);
	UCanvasPanelSlot* MinimapSlot = Root->AddChildToCanvas(MinimapSize);
	MinimapSlot->SetAnchors(FAnchors(1.0f, 1.0f));
	MinimapSlot->SetAlignment(FVector2D(1.0f, 1.0f));
	MinimapSlot->SetPosition(FVector2D(-20.0f, -140.0f));
	MinimapSlot->SetAutoSize(true);
	Minimap = WidgetTree->ConstructWidget<UStrategyMinimapWidget>();
	Minimap->InitializeForController(Controller);
	MinimapSize->AddChild(Minimap);

	MatchOverlay = WidgetTree->ConstructWidget<UOverlay>();
	UCanvasPanelSlot* MatchSlot = Root->AddChildToCanvas(MatchOverlay);
	MatchSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	MatchSlot->SetOffsets(FMargin(0.0f));
	UBorder* MatchShade = WidgetTree->ConstructWidget<UBorder>();
	MatchShade->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.78f));
	MatchOverlay->AddChildToOverlay(MatchShade);
	UVerticalBox* MatchContent = WidgetTree->ConstructWidget<UVerticalBox>();
	UOverlaySlot* MatchContentSlot = MatchOverlay->AddChildToOverlay(MatchContent);
	MatchContentSlot->SetHorizontalAlignment(HAlign_Center);
	MatchContentSlot->SetVerticalAlignment(VAlign_Center);
	MatchText = MakeText(WidgetTree, 44, StrategyHUDStyle::Gold);
	MatchText->SetJustification(ETextJustify::Center);
	MatchContent->AddChildToVerticalBox(MatchText);
	MatchHelpText = MakeText(WidgetTree, 20, StrategyHUDStyle::Text);
	MatchHelpText->SetJustification(ETextJustify::Center);
	MatchHelpText->SetText(FText::FromString(TEXT("R 重新开始    Q 退出")));
	MatchContent->AddChildToVerticalBox(MatchHelpText)->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
	MatchOverlay->SetVisibility(ESlateVisibility::Collapsed);

	Refresh();
	return Super::RebuildWidget();
}

void UStrategyHUDRoot::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	ResourceWarningRemaining = FMath::Max(0.0f, ResourceWarningRemaining - DeltaSeconds);
	NotificationQueue.Update(DeltaSeconds);
	Refresh();
	RefreshNotifications();
}

void UStrategyHUDRoot::NativeDestruct()
{
	if (Controller && Controller->GetWorld())
	{
		if (AStrategyGameState* State = Controller->GetWorld()->GetGameState<AStrategyGameState>())
		{
			State->OnFactionNotification().RemoveAll(this);
		}
	}
	Super::NativeDestruct();
}

void UStrategyHUDRoot::Refresh()
{
	if (!Controller || !ResourceText)
	{
		return;
	}
	RefreshResources();
	RefreshContext();
	RefreshMatchResult();
	ApplyViewportScale();
}

void UStrategyHUDRoot::PushNotification(const FString& Message, const FLinearColor& Color)
{
	NotificationQueue.Push(Message, Color);
	if (Message.Contains(TEXT("金币")) || Message.Contains(TEXT("人口")))
	{
		ResourceWarningRemaining = 0.2f;
	}
	RefreshNotifications();
}

void UStrategyHUDRoot::HandleFactionNotification(EStrategyFaction Faction, const FString& Message)
{
	if (Faction == EStrategyFaction::Player)
	{
		PushNotification(Message, StrategyHUDStyle::Gold);
	}
}

void UStrategyHUDRoot::RefreshNotifications()
{
	if (NotificationTexts.Num() != 3)
	{
		return;
	}
	const TArray<FStrategyHUDNotification>& Items = NotificationQueue.GetItems();
	for (int32 Index = 0; Index < NotificationTexts.Num(); ++Index)
	{
		UTextBlock* Notice = NotificationTexts[Index];
		if (!Items.IsValidIndex(Index))
		{
			if (Notice->GetVisibility() != ESlateVisibility::Collapsed)
			{
				Notice->SetVisibility(ESlateVisibility::Collapsed);
			}
			continue;
		}
		const FStrategyHUDNotification& Item = Items[Index];
		SetStableText(Notice, FName(*FString::Printf(TEXT("Notice%d"), Index)), Item.Message);
		Notice->SetColorAndOpacity(Item.Color.CopyWithNewOpacity(FMath::Min(1.0f, Item.RemainingSeconds)));
		if (Notice->GetVisibility() != ESlateVisibility::HitTestInvisible)
		{
			Notice->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}

void UStrategyHUDRoot::SetStableText(UTextBlock* TextBlock, FName Key, const FString& Value)
{
	if (UpdateCache.AcceptText(Key, Value))
	{
		TextBlock->SetText(FText::FromString(Value));
	}
}

void UStrategyHUDRoot::RefreshResources()
{
	AStrategyGameState* State = Controller->GetWorld()->GetGameState<AStrategyGameState>();
	const FStrategyFactionState& Faction = State->GetFactionState(EStrategyFaction::Player);
	int32 TownCount = 0;
	for (const AStrategyControlPoint* Point : State->GetControlPoints())
	{
		TownCount += IsValid(Point) && !Point->IsCapital();
	}
	const FString ResourceValue = FString::Printf(
		TEXT("金币 %.0f  +%.0f/秒    人口 %d+%d/%d    城镇 %d/%d"),
		Faction.Gold, Faction.IncomePerSecond, Faction.UsedPopulation, Faction.ReservedPopulation,
		Faction.PopulationCap, FMath::Max(0, Faction.OwnedPoints - 1), TownCount);
	SetStableText(ResourceText, TEXT("Resource"), ResourceValue);
	const float WarningState = ResourceWarningRemaining > 0.0f ? 1.0f : 0.0f;
	if (UpdateCache.AcceptScalar(TEXT("ResourceWarning"), WarningState))
	{
		ResourceText->SetColorAndOpacity(WarningState > 0.0f
			? FLinearColor(0.93f, 0.34f, 0.28f, 1.0f) : StrategyHUDStyle::Text);
	}
}

void UStrategyHUDRoot::RefreshContext()
{
	FStrategyHUDContextInputs Inputs;
	Inputs.bBuildMode = Controller->IsBuildMenuOpen();
	Inputs.bHasBuilding = IsValid(Controller->GetSelectedBuilding());
	Inputs.SelectedSquadCount = Controller->GetSelectedSquads().Num();
	if (AStrategyControlPoint* Town = Controller->GetSelectedControlPoint())
	{
		AStrategyGameState* State = Controller->GetWorld()->GetGameState<AStrategyGameState>();
		Inputs.bHasTown = FStrategyTownVisibilityRules::CanShowPublicDetails(EStrategyFaction::Player,
			Town->GetStrategyFaction(), State->IsVisibleToFaction(EStrategyFaction::Player, Town->GetActorLocation()));
	}

	const EStrategyHUDContext Context = FStrategyHUDLayoutRules::ResolveContext(Inputs);
	const uint32 CommandSignature = BuildCommandSignature(Context);
	bRebuildCommands = CommandSignature != LastCommandSignature;
	LastCommandSignature = CommandSignature;
	switch (Context)
	{
	case EStrategyHUDContext::Build: ShowBuildContext(); break;
	case EStrategyHUDContext::Town: ShowTownContext(); break;
	case EStrategyHUDContext::Building: ShowBuildingContext(); break;
	case EStrategyHUDContext::Squad: ShowSquadContext(); break;
	default: ShowIdleContext(); break;
	}
}

uint32 UStrategyHUDRoot::BuildCommandSignature(EStrategyHUDContext Context) const
{
	AStrategyGameState* State = Controller->GetWorld()->GetGameState<AStrategyGameState>();
	const FStrategyFactionState& Faction = State->GetFactionState(EStrategyFaction::Player);
	uint32 Signature = GetTypeHash(static_cast<uint8>(Context));
	Signature = HashCombineFast(Signature, GetTypeHash(Faction.UsedPopulation + Faction.ReservedPopulation));
	Signature = HashCombineFast(Signature, GetTypeHash(Faction.PopulationCap));
	Signature = HashCombineFast(Signature, GetTypeHash(Controller->GetSelectedSquads().Num()));
	if (const AStrategyBuilding* Building = Controller->GetSelectedBuilding())
	{
		Signature = HashCombineFast(Signature, PointerHash(Building));
		Signature = HashCombineFast(Signature, GetTypeHash(Building->GetQueueLength()));
		Signature = HashCombineFast(Signature, GetTypeHash(Building->IsConstructionComplete()));
		for (const EStrategyUnitType UnitType : State->GetBuildingDefinition(Building->GetBuildingType())->TrainableUnits)
		{
			const UStrategyUnitDataAsset* Unit = State->GetUnitDefinition(UnitType);
			const EStrategyHUDUnavailableReason Reason = FStrategyHUDActionRules::GetTrainingUnavailableReason(
				Building->IsConstructionComplete(), true, Building->GetQueueLength(), Faction.Gold,
				Faction.UsedPopulation + Faction.ReservedPopulation, Faction.PopulationCap,
				Unit->GoldCost, Unit->PopulationCost);
			Signature = HashCombineFast(Signature, GetTypeHash(static_cast<uint8>(Reason)));
		}
		if (Building->GetBuildingType() == EStrategyBuildingType::Wall)
		{
			const float GateCost = State->GetBuildingDefinition(EStrategyBuildingType::Gate)->GoldCost;
			Signature = HashCombineFast(Signature,
				GetTypeHash(FStrategyHUDLayoutRules::ResolveAffordabilityState(Faction.Gold, GateCost)));
		}
	}
	if (const AStrategyControlPoint* Town = Controller->GetSelectedControlPoint())
	{
		Signature = HashCombineFast(Signature, PointerHash(Town));
		Signature = HashCombineFast(Signature, GetTypeHash(static_cast<uint8>(Town->GetTownDevelopment().State)));
		Signature = HashCombineFast(Signature, GetTypeHash(static_cast<uint8>(Town->GetStrategyFaction())));
		Signature = HashCombineFast(Signature, GetTypeHash(FStrategyHUDLayoutRules::ResolveAffordabilityState(
			Faction.Gold, FStrategyTownDevelopmentRules::SpecializationCost)));
	}
	Signature = HashCombineFast(Signature, GetTypeHash(Controller->IsBuildingPlacementActive()));
	Signature = HashCombineFast(Signature, GetTypeHash(Controller->IsWallPlacementActive()));
	return Signature;
}

void UStrategyHUDRoot::RefreshMatchResult()
{
	const AStrategyGameState* State = Controller->GetWorld()->GetGameState<AStrategyGameState>();
	if (State->IsMatchRunning())
	{
		MatchOverlay->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	SetStableText(MatchText, TEXT("Match"), State->GetWinner() == EStrategyFaction::Player ? TEXT("胜利") : TEXT("失败"));
	MatchOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UStrategyHUDRoot::ApplyViewportScale()
{
	int32 Width = 0;
	int32 Height = 0;
	Controller->GetViewportSize(Width, Height);
	const FVector2D Scale(FStrategyHUDLayoutRules::ResolveScale(static_cast<float>(Height)));
	if (UpdateCache.AcceptScalar(TEXT("ViewportScale"), Scale.X))
	{
		ResourceBar->SetRenderScale(Scale);
		BottomBar->SetRenderScale(Scale);
	}
	const FVector2D MinimapDimensions = FStrategyMinimapLayoutRules::ResolveSize(static_cast<float>(Height));
	if (UpdateCache.AcceptScalar(TEXT("MinimapWidth"), MinimapDimensions.X))
	{
		MinimapSize->SetWidthOverride(MinimapDimensions.X);
		MinimapSize->SetHeightOverride(MinimapDimensions.Y);
	}
}

void UStrategyHUDRoot::ShowIdleContext()
{
	ObjectProgress->SetVisibility(ESlateVisibility::Collapsed);
	DetailProgress->SetVisibility(ESlateVisibility::Collapsed);
	SetStableText(ObjectText, TEXT("Object"), TEXT("城邦争霸\n等待指令"));
	SetStableText(DetailText, TEXT("Detail"), TEXT("左键选择/框选    右键移动/攻击    F 攻击移动    X 停止    B 建造    Esc 暂停"));
	if (!bRebuildCommands) return;
	ClearCommands();
	AddCommandButton(TEXT("建造\nB"), GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandleBuildMenuClicked));
	AddCommandButton(TEXT("暂停\nEsc"), GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandlePauseClicked));
}

void UStrategyHUDRoot::ShowSquadContext()
{
	AStrategyGameState* State = Controller->GetWorld()->GetGameState<AStrategyGameState>();
	int32 AliveMembers = 0;
	int32 InitialMembers = 0;
	float CurrentHealth = 0.0f;
	float InitialHealth = 0.0f;
	int32 GarrisonedCount = 0;
	const AStrategySquad* FirstGarrisoned = nullptr;
	for (const AStrategySquad* Squad : Controller->GetSelectedSquads())
	{
		if (!IsValid(Squad))
		{
			continue;
		}
		const UStrategyUnitDataAsset* Definition = State->GetUnitDefinition(Squad->GetUnitType());
		const float SquadInitialHealth = Definition->MaxHealth * Definition->MemberCount;
		AliveMembers += Squad->GetMembers().Num();
		InitialMembers += Definition->MemberCount;
		CurrentHealth += Squad->GetHealthPercent() * SquadInitialHealth;
		InitialHealth += SquadInitialHealth;
		if (Squad->IsGarrisoned())
		{
			++GarrisonedCount;
			FirstGarrisoned = FirstGarrisoned ? FirstGarrisoned : Squad;
		}
	}
	const float HealthPercent = InitialHealth > 0.0f ? CurrentHealth / InitialHealth : 0.0f;
	ObjectProgress->SetVisibility(ESlateVisibility::HitTestInvisible);
	ObjectProgress->SetPercent(HealthPercent);
	ObjectProgress->SetFillColorAndOpacity(HealthPercent > 0.5f ? FLinearColor(0.18f, 0.72f, 0.35f, 1.0f)
		: HealthPercent > 0.25f ? FLinearColor(0.95f, 0.72f, 0.18f, 1.0f) : FLinearColor(0.88f, 0.18f, 0.16f, 1.0f));
	DetailProgress->SetVisibility(ESlateVisibility::Collapsed);
	SetStableText(ObjectText, TEXT("Object"), FString::Printf(TEXT("已选小队 ×%d\n综合生命 %.0f%%"),
		Controller->GetSelectedSquads().Num(), HealthPercent * 100.0f));
	FString Details = FString::Printf(TEXT("存活 %d / 初始 %d    可接收移动、攻击移动和停止命令\n拖动世界队徽可快速下令"),
		AliveMembers, InitialMembers);
	if (FirstGarrisoned)
	{
		const AStrategyControlPoint* Point = FirstGarrisoned->GetGarrisonPoint();
		const FStrategyTownDevelopment& Development = Point->GetTownDevelopment();
		const float Delay = FStrategyGarrisonRules::GetRecoveryDelay(Development.Specialization, Development.State);
		const float Rate = FStrategyGarrisonRules::GetRecoveryRate(Development.Specialization, Development.State);
		const float Interval = FStrategyGarrisonRules::GetReinforcementInterval(Development.Specialization, Development.State);
		const FString Recovery = FirstGarrisoned->GetGarrisonElapsed() < Delay
			? FString::Printf(TEXT("等待整补 %d 秒"), FMath::CeilToInt(Delay - FirstGarrisoned->GetGarrisonElapsed()))
			: FString::Printf(TEXT("恢复 %.0f%%/秒"), Rate * 100.0f);
		const FString Reinforcement = FirstGarrisoned->GetMembers().Num() < FirstGarrisoned->GetInitialMemberCount()
			? FString::Printf(TEXT("补员 %d 秒"), FMath::Max(0, FMath::CeilToInt(Interval - FirstGarrisoned->GetReinforcementElapsed())))
			: TEXT("已满编");
		Details += FString::Printf(TEXT("\n驻防 %s（已选 %d）  容量 %d/%d  %s  %s"),
			Point->IsCapital() ? TEXT("主城") : TEXT("城镇"), GarrisonedCount,
			Point->GetGarrisonedSquads().Num(), Point->GetGarrisonCapacity(), *Recovery, *Reinforcement);
	}
	SetStableText(DetailText, TEXT("Detail"), Details);
	if (!bRebuildCommands) return;
	ClearCommands();
	AddCommandButton(TEXT("移动\n右键"), GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandleMoveClicked));
	AddCommandButton(TEXT("攻击移动\nF"), GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandleAttackMoveClicked));
	AddCommandButton(TEXT("停止\nX"), GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandleStopClicked));
}

void UStrategyHUDRoot::ShowBuildingContext()
{
	AStrategyBuilding* Building = Controller->GetSelectedBuilding();
	const float HealthPercent = Building->GetHealthPercent();
	ObjectProgress->SetVisibility(ESlateVisibility::HitTestInvisible);
	ObjectProgress->SetPercent(HealthPercent);
	ObjectProgress->SetFillColorAndOpacity(HealthPercent > 0.5f ? FLinearColor(0.18f, 0.72f, 0.35f, 1.0f)
		: HealthPercent > 0.25f ? FLinearColor(0.95f, 0.72f, 0.18f, 1.0f) : FLinearColor(0.88f, 0.18f, 0.16f, 1.0f));
	SetStableText(ObjectText, TEXT("Object"), FString::Printf(TEXT("%s\n生命 %.0f%%"),
		*StrategyHUDText::Building(Building->GetBuildingType()), HealthPercent * 100.0f));
	if (!Building->IsConstructionComplete())
	{
		DetailProgress->SetVisibility(ESlateVisibility::HitTestInvisible);
		DetailProgress->SetPercent(Building->GetConstructionProgress());
		DetailProgress->SetFillColorAndOpacity(StrategyHUDStyle::Gold);
		SetStableText(DetailText, TEXT("Detail"), FString::Printf(TEXT("建设中 %.0f%%"), Building->GetConstructionProgress() * 100.0f));
	}
	else
	{
		DetailProgress->SetVisibility(Building->GetQueueLength() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		DetailProgress->SetPercent(Building->GetTrainingProgress());
		DetailProgress->SetFillColorAndOpacity(StrategyHUDStyle::Gold);
		SetStableText(DetailText, TEXT("Detail"), FString::Printf(TEXT("训练队列 %d/5    当前进度 %.0f%%"),
			Building->GetQueueLength(), Building->GetTrainingProgress() * 100.0f));
	}
	if (!bRebuildCommands) return;
	ClearCommands();
	AStrategyGameState* State = Controller->GetWorld()->GetGameState<AStrategyGameState>();
	const FStrategyFactionState& Faction = State->GetFactionState(EStrategyFaction::Player);
	for (const EStrategyUnitType UnitType : State->GetBuildingDefinition(Building->GetBuildingType())->TrainableUnits)
	{
		const UStrategyUnitDataAsset* Unit = State->GetUnitDefinition(UnitType);
		const EStrategyHUDUnavailableReason Reason = FStrategyHUDActionRules::GetTrainingUnavailableReason(
			Building->IsConstructionComplete(), true, Building->GetQueueLength(), Faction.Gold,
			Faction.UsedPopulation + Faction.ReservedPopulation, Faction.PopulationCap, Unit->GoldCost, Unit->PopulationCost);
		const FName Handler = UnitType == EStrategyUnitType::Infantry
			? GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandleTrainInfantryClicked)
			: UnitType == EStrategyUnitType::Archer
				? GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandleTrainArcherClicked)
				: GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandleTrainCavalryClicked);
		AddCommandButton(FString::Printf(TEXT("训练%s\n%.0f 金币"), *StrategyHUDText::Unit(UnitType), Unit->GoldCost),
			Handler, Reason == EStrategyHUDUnavailableReason::None, FStrategyHUDActionRules::GetUnavailableReasonText(Reason));
	}
	if (Building->GetBuildingType() == EStrategyBuildingType::Wall)
	{
		const UStrategyBuildingDataAsset* Gate = State->GetBuildingDefinition(EStrategyBuildingType::Gate);
		const bool bCanUpgrade = Building->IsConstructionComplete() && Faction.Gold >= Gate->GoldCost;
		AddCommandButton(FString::Printf(TEXT("升级城门\n7 · %.0f 金币"), Gate->GoldCost),
			GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandleUpgradeGateClicked), bCanUpgrade,
			Building->IsConstructionComplete() ? TEXT("金币不足") : TEXT("城墙尚未完工"));
	}
}

void UStrategyHUDRoot::ShowTownContext()
{
	ObjectProgress->SetVisibility(ESlateVisibility::Collapsed);
	DetailProgress->SetVisibility(ESlateVisibility::Collapsed);
	AStrategyControlPoint* Town = Controller->GetSelectedControlPoint();
	AStrategyGameState* State = Controller->GetWorld()->GetGameState<AStrategyGameState>();
	const EStrategyFaction Owner = Town->GetStrategyFaction();
	const bool bVisible = State->IsVisibleToFaction(EStrategyFaction::Player, Town->GetActorLocation());
	const FStrategyTownDevelopment& Development = Town->GetTownDevelopment();
	const EStrategyTownSpecialization PublicSpecialization = FStrategyTownVisibilityRules::GetPublicSpecialization(
		EStrategyFaction::Player, Owner, bVisible, Development.Specialization, Development.State);
	SetStableText(ObjectText, TEXT("Object"), FString::Printf(TEXT("%s城镇\n%s"),
		*StrategyHUDText::Owner(Owner), *StrategyHUDText::Specialization(PublicSpecialization)));
	FString Details = TEXT("基础：+3 金币/秒，+5 人口");
	if (FStrategyTownVisibilityRules::CanShowLiveDetails(EStrategyFaction::Player, Owner, bVisible))
	{
		Details += FString::Printf(TEXT("    补给：%s    状态：%s"), State->IsTownSupplyConnected(Town) ? TEXT("已连接") : TEXT("未连接"),
			*StrategyHUDText::DevelopmentState(Development.State));
		if (Development.State == EStrategyTownDevelopmentState::Building
			|| Development.State == EStrategyTownDevelopmentState::Downgrading
			|| Development.State == EStrategyTownDevelopmentState::DisabledAfterCapture)
		{
			Details += FString::Printf(TEXT(" %.0f%%"), Town->GetDevelopmentProgress() * 100.0f);
			DetailProgress->SetVisibility(ESlateVisibility::HitTestInvisible);
			DetailProgress->SetPercent(Town->GetDevelopmentProgress());
			DetailProgress->SetFillColorAndOpacity(Development.State == EStrategyTownDevelopmentState::Downgrading
				? FLinearColor(0.92f, 0.42f, 0.16f, 1.0f) : StrategyHUDStyle::Gold);
		}
		if (Owner == EStrategyFaction::Player)
		{
			Details += FString::Printf(TEXT("    驻防：%d/%d"), Town->GetGarrisonedSquads().Num(), Town->GetGarrisonCapacity());
		}
	}
	SetStableText(DetailText, TEXT("Detail"), Details);
	if (!bRebuildCommands) return;
	ClearCommands();
	if (FStrategyTownActionRules::CanChooseSpecialization(EStrategyFaction::Player, Owner, Town->IsCapital(), Development.State))
	{
		const bool bCanAfford = State->GetFactionState(EStrategyFaction::Player).Gold >= 250.0f;
		AddCommandButton(TEXT("贸易\n250 金币"), GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandleTradeClicked), bCanAfford, TEXT("金币不足"));
		AddCommandButton(TEXT("征募\n250 金币"), GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandleRecruitmentClicked), bCanAfford, TEXT("金币不足"));
		AddCommandButton(TEXT("要塞\n250 金币"), GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandleFortressClicked), bCanAfford, TEXT("金币不足"));
	}
	else if (FStrategyTownActionRules::CanDowngrade(EStrategyFaction::Player, Owner, Town->IsCapital(), Development.State))
	{
		AddCommandButton(TEXT("降级\n返还 100"), GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandleDowngradeClicked));
	}
}

void UStrategyHUDRoot::ShowBuildContext()
{
	ObjectProgress->SetVisibility(ESlateVisibility::Collapsed);
	DetailProgress->SetVisibility(ESlateVisibility::Collapsed);
	SetStableText(ObjectText, TEXT("Object"), Controller->IsWallPlacementActive() ? TEXT("建造模式\n城墙") : TEXT("建造模式"));
	FString PlacementText = Controller->IsWallPlacementActive()
		? TEXT("按住左键拖动城墙    绿色有效 / 红色无效    B 取消")
		: Controller->IsBuildingPlacementActive()
			? TEXT("左键在有效领地放置建筑    B 取消")
			: TEXT("选择要建造的设施");
	AStrategyGameState* State = Controller->GetWorld()->GetGameState<AStrategyGameState>();
	if (Controller->IsBuildingPlacementActive())
	{
		const EStrategyBuildingType Type = static_cast<EStrategyBuildingType>(Controller->GetPendingBuildingIndex());
		const UStrategyBuildingDataAsset* Definition = State->GetBuildingDefinition(Type);
		if (State->GetFactionState(EStrategyFaction::Player).Gold < Definition->GoldCost)
		{
			PlacementText = TEXT("金币不足");
		}
		else
		{
			FVector Location;
			if (Controller->GetCursorWorldLocationForUI(Location))
			{
				PlacementText = FStrategyPlacementIssueRules::GetIssueText(State->GetBuildingPlacementIssue(
					EStrategyFaction::Player, Type, Location));
			}
		}
	}
	SetStableText(DetailText, TEXT("Detail"), PlacementText);
	if (!bRebuildCommands) return;
	ClearCommands();
	const float Gold = State->GetFactionState(EStrategyFaction::Player).Gold;
	const EStrategyBuildingType Types[] = { EStrategyBuildingType::Barracks, EStrategyBuildingType::ArcheryRange,
		EStrategyBuildingType::Stable, EStrategyBuildingType::House, EStrategyBuildingType::Tower, EStrategyBuildingType::Wall };
	const FName Handlers[] = { GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandleBuildBarracksClicked),
		GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandleBuildArcheryClicked), GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandleBuildStableClicked),
		GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandleBuildHouseClicked), GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandleBuildTowerClicked),
		GET_FUNCTION_NAME_CHECKED(UStrategyHUDRoot, HandleBuildWallClicked) };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Types); ++Index)
	{
		const UStrategyBuildingDataAsset* Definition = State->GetBuildingDefinition(Types[Index]);
		const bool bCanAfford = Gold >= Definition->GoldCost;
		AddCommandButton(FString::Printf(TEXT("%s\n%d · %.0f 金币"), *StrategyHUDText::Building(Types[Index]), Index + 1,
			Definition->GoldCost), Handlers[Index], bCanAfford, TEXT("金币不足"));
	}
}

UButton* UStrategyHUDRoot::AddCommandButton(const FString& Label, FName HandlerName, bool bEnabled,
	const FString& UnavailableReason)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	Button->IsFocusable = false;
	FButtonStyle ButtonStyle;
	ButtonStyle.SetNormal(FSlateColorBrush(FLinearColor(0.18f, 0.11f, 0.07f, 1.0f)));
	ButtonStyle.SetHovered(FSlateColorBrush(FLinearColor(0.38f, 0.25f, 0.10f, 1.0f)));
	ButtonStyle.SetPressed(FSlateColorBrush(FLinearColor(0.11f, 0.07f, 0.05f, 1.0f)));
	ButtonStyle.SetDisabled(FSlateColorBrush(FLinearColor(0.16f, 0.17f, 0.17f, 0.82f)));
	ButtonStyle.SetNormalPadding(FMargin(5.0f));
	ButtonStyle.SetPressedPadding(FMargin(7.0f, 7.0f, 3.0f, 3.0f));
	Button->SetStyle(ButtonStyle);
	Button->SetIsEnabled(bEnabled);
	if (!bEnabled)
	{
		Button->SetToolTipText(FText::FromString(UnavailableReason));
	}
	FScriptDelegate ClickDelegate;
	ClickDelegate.BindUFunction(this, HandlerName);
	Button->OnClicked.Add(ClickDelegate);
	UTextBlock* LabelText = MakeText(WidgetTree, 15, StrategyHUDStyle::Text);
	LabelText->SetJustification(ETextJustify::Center);
	LabelText->SetText(FText::FromString(Label));
	Button->AddChild(LabelText);
	UHorizontalBoxSlot* ButtonSlot = CommandBox->AddChildToHorizontalBox(Button);
	SetFill(ButtonSlot, 1.0f);
	ButtonSlot->SetPadding(FMargin(4.0f));
	return Button;
}

void UStrategyHUDRoot::ClearCommands()
{
	CommandBox->ClearChildren();
}

void UStrategyHUDRoot::HandleMoveClicked() { Controller->BeginMoveCommandFromUI(); }
void UStrategyHUDRoot::HandleAttackMoveClicked() { Controller->BeginAttackMoveCommandFromUI(); }
void UStrategyHUDRoot::HandleStopClicked() { Controller->StopSelectedSquadsFromUI(); }
void UStrategyHUDRoot::HandleBuildMenuClicked() { Controller->ToggleBuildMenuFromUI(); }
void UStrategyHUDRoot::HandlePauseClicked() { Controller->OpenPauseMenu(); }
void UStrategyHUDRoot::HandleBuildBarracksClicked() { Controller->SelectBuildItemFromUI(1); }
void UStrategyHUDRoot::HandleBuildArcheryClicked() { Controller->SelectBuildItemFromUI(2); }
void UStrategyHUDRoot::HandleBuildStableClicked() { Controller->SelectBuildItemFromUI(3); }
void UStrategyHUDRoot::HandleBuildHouseClicked() { Controller->SelectBuildItemFromUI(4); }
void UStrategyHUDRoot::HandleBuildTowerClicked() { Controller->SelectBuildItemFromUI(5); }
void UStrategyHUDRoot::HandleBuildWallClicked() { Controller->SelectBuildItemFromUI(6); }
void UStrategyHUDRoot::HandleUpgradeGateClicked() { Controller->SelectBuildItemFromUI(7); }
void UStrategyHUDRoot::HandleTrainInfantryClicked() { Controller->TrainSelectedBuildingFromUI(EStrategyUnitType::Infantry); }
void UStrategyHUDRoot::HandleTrainArcherClicked() { Controller->TrainSelectedBuildingFromUI(EStrategyUnitType::Archer); }
void UStrategyHUDRoot::HandleTrainCavalryClicked() { Controller->TrainSelectedBuildingFromUI(EStrategyUnitType::Cavalry); }
void UStrategyHUDRoot::HandleTradeClicked() { Controller->TrySpecializeSelectedTown(EStrategyTownSpecialization::Trade); Controller->RestoreGameFocus(); }
void UStrategyHUDRoot::HandleRecruitmentClicked() { Controller->TrySpecializeSelectedTown(EStrategyTownSpecialization::Recruitment); Controller->RestoreGameFocus(); }
void UStrategyHUDRoot::HandleFortressClicked() { Controller->TrySpecializeSelectedTown(EStrategyTownSpecialization::Fortress); Controller->RestoreGameFocus(); }
void UStrategyHUDRoot::HandleDowngradeClicked() { Controller->TryDowngradeSelectedTown(); Controller->RestoreGameFocus(); }
