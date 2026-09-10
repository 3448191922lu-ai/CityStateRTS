#include "StrategyMapDefinition.h"

FStrategySkirmishMapDefinition FStrategyMapDefinitions::Resolve(const FString& MapName)
{
	FStrategySkirmishMapDefinition Definition;
	if (MapName.Contains(TEXT("LVL_RiverValleySkirmish")))
	{
		Definition.NeutralTowns = {
			FVector(-2200.0f, 3900.0f, 0.0f),
			FVector(0.0f, 0.0f, 0.0f),
			FVector(2200.0f, -3900.0f, 0.0f)};
		Definition.NoBuildZones.Add(FBox2D(FVector2D(-750.0f, -7000.0f), FVector2D(750.0f, 7000.0f)));
		Definition.MinimapWaterAreas = {
			FBox2D(FVector2D(-550.0f, -7000.0f), FVector2D(550.0f, -3900.0f)),
			FBox2D(FVector2D(-550.0f, -2800.0f), FVector2D(550.0f, 0.0f)),
			FBox2D(FVector2D(-550.0f, 0.0f), FVector2D(550.0f, 2800.0f)),
			FBox2D(FVector2D(-550.0f, 3900.0f), FVector2D(550.0f, 7000.0f))};
		Definition.MinimapRouteAreas = {
			FBox2D(FVector2D(-700.0f, -3900.0f), FVector2D(700.0f, -2100.0f)),
			FBox2D(FVector2D(-550.0f, -600.0f), FVector2D(550.0f, 600.0f)),
			FBox2D(FVector2D(-700.0f, 2100.0f), FVector2D(700.0f, 3900.0f))};
		Definition.bSpawnRiverValleyTerrain = true;
	}
	else
	{
		Definition.NeutralTowns = {
			FVector(0.0f, 0.0f, 0.0f),
			FVector(0.0f, 4000.0f, 0.0f),
			FVector(0.0f, -4000.0f, 0.0f)};
	}
	return Definition;
}

bool FStrategyMapDefinitions::IsBuildingAllowed(const FStrategySkirmishMapDefinition& Definition,
	const FVector& Location, const FVector2D& FootprintExtent)
{
	const FVector2D Center(Location.X, Location.Y);
	const FBox2D Footprint(Center - FootprintExtent, Center + FootprintExtent);
	for (const FBox2D& Zone : Definition.NoBuildZones)
	{
		if (Zone.Intersect(Footprint))
		{
			return false;
		}
	}
	return true;
}
