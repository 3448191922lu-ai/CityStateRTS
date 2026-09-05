#pragma once

#include "CoreMinimal.h"
#include "StrategyTypes.h"

struct FStrategyRules
{
	static float GetDamageMultiplier(EStrategyUnitType Attacker, EStrategyUnitType Defender);
	static EStrategyFaction GetWinner(bool bPlayerCapitalAlive, bool bEnemyCapitalAlive);
	static bool IsInsideTerritory(const FVector& Location, float FootprintRadius, const FVector& PointLocation, float TerritoryRadius);
	static bool IsFootprintInsideTerritory(const FVector& Location, const FVector2D& FootprintExtent, float YawDegrees, const FVector& PointLocation, float TerritoryRadius);
};
