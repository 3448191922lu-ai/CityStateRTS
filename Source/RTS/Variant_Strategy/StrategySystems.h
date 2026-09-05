#pragma once

#include "CoreMinimal.h"
#include "StrategyTypes.h"

struct FStrategyTrainingItem
{
	EStrategyUnitType UnitType = EStrategyUnitType::Infantry;
	float RemainingTime = 0.0f;
	int32 PopulationCost = 0;
};

struct FStrategyTrainingQueue
{
	bool Enqueue(EStrategyUnitType UnitType, float TrainingTime, int32 PopulationCost);
	bool Update(float DeltaSeconds, EStrategyUnitType& OutCompletedType, int32& OutPopulationCost);
	int32 GetReservedPopulation() const;
	int32 Num() const { return Items.Num(); }
	void Reset() { Items.Reset(); }

private:
	TArray<FStrategyTrainingItem> Items;
};

struct FStrategyFogGrid
{
	void Initialize(int32 InWidth, int32 InHeight, const FVector2D& InMin, const FVector2D& InMax);
	void BeginVisibilityUpdate();
	void Reveal(const FVector2D& WorldLocation, float Radius);
	bool IsVisible(const FVector2D& WorldLocation) const;
	bool IsExplored(const FVector2D& WorldLocation) const;
	int32 GetWidth() const { return Width; }
	int32 GetHeight() const { return Height; }
	bool IsCellVisible(int32 Index) const { return Visible.IsValidIndex(Index) && Visible[Index]; }
	bool IsCellExplored(int32 Index) const { return Explored.IsValidIndex(Index) && Explored[Index]; }

private:
	int32 ToIndex(const FVector2D& WorldLocation) const;

	int32 Width = 0;
	int32 Height = 0;
	FVector2D Min = FVector2D::ZeroVector;
	FVector2D Max = FVector2D::ZeroVector;
	TBitArray<> Visible;
	TBitArray<> Explored;
};

enum class EStrategyAIAction : uint8
{
	Defend,
	Capture,
	Build,
	Train,
	Attack,
	Idle
};

struct FStrategyAIInputs
{
	bool bOwnedPointThreatened = false;
	bool bNeutralPointAvailable = false;
	bool bMissingProductionBuilding = false;
	int32 AvailableSquads = 0;
};

struct FStrategyAIPlanner
{
	static EStrategyAIAction ChooseAction(const FStrategyAIInputs& Inputs);
};

struct FStrategyCameraMovement
{
	static FVector2D ResolveScreenDirection(bool bUp, bool bDown, bool bLeft, bool bRight);
	static FVector ScreenToWorld(const FVector2D& ScreenDirection, float CameraYaw = -45.0f);
};

struct FStrategyWallSegmentPlan
{
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
};

struct FStrategyWallPlanner
{
	static TArray<FStrategyWallSegmentPlan> BuildLine(const FVector& Start, const FVector& End, float SegmentLength, int32 MaxSegments);
	static int32 GetAffordableCount(int32 ValidSegmentCount, float Gold, float SegmentCost);
};

struct FStrategyWallPlacementRules
{
	static TArray<int32> SelectBuildableIndices(const TArray<bool>& ValidSegments, float Gold, float SegmentCost);
	static bool CanUpgradeToGate(EStrategyFaction RequestingFaction, EStrategyFaction WallFaction,
		EStrategyBuildingType BuildingType, bool bConstructionComplete);
};

struct FStrategyGateCollisionRules
{
	static bool ShouldBlock(EStrategyFaction GateFaction, EStrategyFaction UnitFaction, bool bConstructionComplete);
	static bool CanAttackBlockingBuilding(EStrategyFaction UnitFaction, EStrategyFaction BuildingFaction,
		EStrategyBuildingType BuildingType);
};

struct FStrategyTowerTargetRules
{
	static bool CanTarget(EStrategyFaction TowerFaction, EStrategyFaction TargetFaction, bool bConstructionComplete,
		bool bTargetAlive, bool bTargetVisible, float Distance, float AttackRange);
};

struct FStrategySquadMarkerRules
{
	static int32 FindHoveredMarker(const TArray<FVector2D>& MarkerPositions,
		const FVector2D& CursorPosition, float HitRadius);
	static bool ShouldCommandSelectedSquads(bool bSourceSelected);
	static EStrategyOrderType ResolveOrderType(bool bVisibleEnemyTarget);
	static float CalculateHealthPercent(float CurrentHealth, float InitialTotalHealth);
	static bool CanInteract(bool bBuildingPlacementActive, bool bWallPlacementActive);
};
