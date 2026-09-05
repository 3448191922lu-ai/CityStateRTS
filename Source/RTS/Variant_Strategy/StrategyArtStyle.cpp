#include "StrategyArtStyle.h"

namespace StrategyArtStyle
{
FLinearColor GetFactionColor(EStrategyFaction Faction)
{
	switch (Faction)
	{
	case EStrategyFaction::Player: return FLinearColor(0.10f, 0.34f, 0.58f);
	case EStrategyFaction::Enemy: return FLinearColor(0.61f, 0.18f, 0.14f);
	default: return FLinearColor(0.68f, 0.54f, 0.32f);
	}
}

FLinearColor GetEnvironmentColor()
{
	return FLinearColor(0.43f, 0.51f, 0.40f);
}

FLinearColor GetBuildingColor(EStrategyBuildingType BuildingType, EStrategyFaction Faction)
{
	const FLinearColor FactionColor = GetFactionColor(Faction);
	const FLinearColor WarmWood(0.42f, 0.28f, 0.18f);
	const FLinearColor Stone(0.48f, 0.46f, 0.40f);
	const bool bStone = BuildingType == EStrategyBuildingType::Tower || BuildingType == EStrategyBuildingType::Wall || BuildingType == EStrategyBuildingType::Gate;
	return FMath::Lerp(bStone ? Stone : WarmWood, FactionColor, BuildingType == EStrategyBuildingType::Capital ? 0.30f : 0.18f);
}

FVector GetUnitSilhouetteScale(EStrategyUnitType UnitType)
{
	switch (UnitType)
	{
	case EStrategyUnitType::Archer: return FVector(0.62f, 0.62f, 1.45f);
	case EStrategyUnitType::Cavalry: return FVector(1.55f, 0.90f, 1.10f);
	default: return FVector(0.72f, 0.72f, 1.35f);
	}
}

FVector GetBuildingSilhouetteScale(EStrategyBuildingType BuildingType)
{
	switch (BuildingType)
	{
	case EStrategyBuildingType::ArcheryRange: return FVector(2.8f, 2.3f, 1.15f);
	case EStrategyBuildingType::Stable: return FVector(3.8f, 2.4f, 1.35f);
	case EStrategyBuildingType::House: return FVector(2.2f, 2.0f, 2.6f);
	case EStrategyBuildingType::Tower: return FVector(1.45f, 1.45f, 4.2f);
	case EStrategyBuildingType::Wall: return FVector(4.0f, 1.2f, 1.6f);
	case EStrategyBuildingType::Gate: return FVector(4.0f, 1.5f, 2.0f);
	case EStrategyBuildingType::Capital: return FVector(4.5f, 3.5f, 2.4f);
	default: return FVector(3.0f, 2.0f, 1.5f);
	}
}
}
