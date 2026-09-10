#include "StrategyHUDModel.h"

EStrategyHUDContext FStrategyHUDLayoutRules::ResolveContext(const FStrategyHUDContextInputs& Inputs)
{
	if (Inputs.bBuildMode)
	{
		return EStrategyHUDContext::Build;
	}
	if (Inputs.bHasTown)
	{
		return EStrategyHUDContext::Town;
	}
	if (Inputs.bHasBuilding)
	{
		return EStrategyHUDContext::Building;
	}
	return Inputs.SelectedSquadCount > 0 ? EStrategyHUDContext::Squad : EStrategyHUDContext::Idle;
}

float FStrategyHUDLayoutRules::ResolveScale(float ViewportHeight)
{
	// 文字保持 1:1 像素缩放，避免窗口客户区高度产生小数缩放和字形跳动。
	return 1.0f;
}

uint8 FStrategyHUDLayoutRules::ResolveAffordabilityState(float Gold, float Cost)
{
	return Gold >= Cost ? 1 : 0;
}

FStrategyHUDVisibility FStrategyHUDVisibility::ForContext(EStrategyHUDContext Context)
{
	if (Context == EStrategyHUDContext::Idle)
	{
		FStrategyHUDVisibility Visibility;
		Visibility.bIdleHelp = true;
		return Visibility;
	}
	FStrategyHUDVisibility Visibility;
	Visibility.bObjectPanel = true;
	Visibility.bDetailPanel = true;
	Visibility.bCommandPanel = true;
	return Visibility;
}

EStrategyHUDUnavailableReason FStrategyHUDActionRules::GetTrainingUnavailableReason(bool bConstructionComplete,
	bool bTrainable, int32 QueueLength, float Gold, int32 UsedAndReservedPopulation,
	int32 PopulationCap, float GoldCost, int32 PopulationCost)
{
	if (!bConstructionComplete)
	{
		return EStrategyHUDUnavailableReason::UnderConstruction;
	}
	if (!bTrainable)
	{
		return EStrategyHUDUnavailableReason::WrongBuilding;
	}
	if (QueueLength >= 5)
	{
		return EStrategyHUDUnavailableReason::QueueFull;
	}
	if (Gold < GoldCost)
	{
		return EStrategyHUDUnavailableReason::NotEnoughGold;
	}
	return UsedAndReservedPopulation + PopulationCost > PopulationCap
		? EStrategyHUDUnavailableReason::PopulationFull
		: EStrategyHUDUnavailableReason::None;
}

FString FStrategyHUDActionRules::GetUnavailableReasonText(EStrategyHUDUnavailableReason Reason)
{
	switch (Reason)
	{
	case EStrategyHUDUnavailableReason::UnderConstruction: return TEXT("建筑尚未完工");
	case EStrategyHUDUnavailableReason::WrongBuilding: return TEXT("该建筑不能训练此单位");
	case EStrategyHUDUnavailableReason::QueueFull: return TEXT("训练队列已满");
	case EStrategyHUDUnavailableReason::NotEnoughGold: return TEXT("金币不足");
	case EStrategyHUDUnavailableReason::PopulationFull: return TEXT("人口已满");
	default: return FString();
	}
}

EStrategyBuildingPlacementIssue FStrategyPlacementIssueRules::Resolve(bool bMapAllowed, bool bInsideTerritory,
	bool bNavigable, bool bOverlapping)
{
	if (!bMapAllowed) return EStrategyBuildingPlacementIssue::MapRestricted;
	if (!bInsideTerritory) return EStrategyBuildingPlacementIssue::OutsideTerritory;
	if (!bNavigable) return EStrategyBuildingPlacementIssue::NotNavigable;
	return bOverlapping ? EStrategyBuildingPlacementIssue::Overlap : EStrategyBuildingPlacementIssue::None;
}

FString FStrategyPlacementIssueRules::GetIssueText(EStrategyBuildingPlacementIssue Issue)
{
	switch (Issue)
	{
	case EStrategyBuildingPlacementIssue::MapRestricted: return TEXT("地图区域不可建造");
	case EStrategyBuildingPlacementIssue::OutsideTerritory: return TEXT("超出己方领地");
	case EStrategyBuildingPlacementIssue::NotNavigable: return TEXT("地面不可导航");
	case EStrategyBuildingPlacementIssue::Overlap: return TEXT("与建筑或单位重叠");
	default: return TEXT("✓ 领地、地形与占地均有效");
	}
}

void FStrategyHUDNotificationQueue::Push(const FString& Message, const FLinearColor& Color)
{
	Items.Add({Message, Color, 3.0f});
	while (Items.Num() > 3)
	{
		Items.RemoveAt(0);
	}
}

void FStrategyHUDNotificationQueue::Update(float DeltaSeconds)
{
	for (FStrategyHUDNotification& Item : Items)
	{
		Item.RemainingSeconds -= DeltaSeconds;
	}
	Items.RemoveAll([](const FStrategyHUDNotification& Item) { return Item.RemainingSeconds <= 0.0f; });
}

bool FStrategyHUDUpdateCache::AcceptText(FName Key, const FString& Value)
{
	if (const FString* Previous = TextValues.Find(Key); Previous && *Previous == Value)
	{
		return false;
	}
	TextValues.Add(Key, Value);
	return true;
}

bool FStrategyHUDUpdateCache::AcceptScalar(FName Key, float Value)
{
	if (const float* Previous = ScalarValues.Find(Key); Previous && *Previous == Value)
	{
		return false;
	}
	ScalarValues.Add(Key, Value);
	return true;
}

void FStrategyHUDUpdateCache::Reset()
{
	TextValues.Reset();
	ScalarValues.Reset();
}
