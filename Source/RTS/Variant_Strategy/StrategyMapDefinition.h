#pragma once

#include "CoreMinimal.h"

struct FStrategySkirmishMapDefinition
{
	FVector PlayerCapital = FVector(-7000.0f, 0.0f, 0.0f);
	FVector EnemyCapital = FVector(7000.0f, 0.0f, 0.0f);
	TArray<FVector> NeutralTowns;
	FVector PlayerSquadStart = FVector(-5600.0f, 0.0f, 100.0f);
	FVector EnemySquadStart = FVector(5600.0f, 0.0f, 100.0f);
	FVector2D CameraMin = FVector2D(-11000.0f, -9000.0f);
	FVector2D CameraMax = FVector2D(11000.0f, 9000.0f);
	FVector2D FogMin = FVector2D(-16000.0f, -14000.0f);
	FVector2D FogMax = FVector2D(16000.0f, 14000.0f);
	FIntPoint FogGridSize = FIntPoint(192, 160);
	TArray<FBox2D> NoBuildZones;
	TArray<FBox2D> MinimapWaterAreas;
	TArray<FBox2D> MinimapRouteAreas;
	bool bSpawnRiverValleyTerrain = false;
};

struct FStrategyMapDefinitions
{
	static FStrategySkirmishMapDefinition Resolve(const FString& MapName);
	static bool IsBuildingAllowed(const FStrategySkirmishMapDefinition& Definition, const FVector& Location,
		const FVector2D& FootprintExtent);
};
