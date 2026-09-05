#pragma once

#include "CoreMinimal.h"
#include "StrategyTypes.h"

namespace StrategyArtStyle
{
FLinearColor GetFactionColor(EStrategyFaction Faction);
FLinearColor GetEnvironmentColor();
FLinearColor GetBuildingColor(EStrategyBuildingType BuildingType, EStrategyFaction Faction);
FVector GetUnitSilhouetteScale(EStrategyUnitType UnitType);
FVector GetBuildingSilhouetteScale(EStrategyBuildingType BuildingType);
}
