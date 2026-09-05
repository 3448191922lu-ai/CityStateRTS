#include "StrategySystems.h"

bool FStrategyTrainingQueue::Enqueue(EStrategyUnitType UnitType, float TrainingTime, int32 PopulationCost)
{
	if (Items.Num() >= 5)
	{
		return false;
	}

	Items.Add({UnitType, TrainingTime, PopulationCost});
	return true;
}

bool FStrategyTrainingQueue::Update(float DeltaSeconds, EStrategyUnitType& OutCompletedType, int32& OutPopulationCost)
{
	if (Items.IsEmpty())
	{
		return false;
	}

	Items[0].RemainingTime -= DeltaSeconds;
	if (Items[0].RemainingTime > 0.0f)
	{
		return false;
	}

	OutCompletedType = Items[0].UnitType;
	OutPopulationCost = Items[0].PopulationCost;
	Items.RemoveAt(0);
	return true;
}

int32 FStrategyTrainingQueue::GetReservedPopulation() const
{
	int32 Total = 0;
	for (const FStrategyTrainingItem& Item : Items)
	{
		Total += Item.PopulationCost;
	}
	return Total;
}

void FStrategyFogGrid::Initialize(int32 InWidth, int32 InHeight, const FVector2D& InMin, const FVector2D& InMax)
{
	Width = InWidth;
	Height = InHeight;
	Min = InMin;
	Max = InMax;
	Visible.Init(false, Width * Height);
	Explored.Init(false, Width * Height);
}

void FStrategyFogGrid::BeginVisibilityUpdate()
{
	Visible.Init(false, Width * Height);
}

void FStrategyFogGrid::Reveal(const FVector2D& WorldLocation, float Radius)
{
	if (Width <= 0 || Height <= 0)
	{
		return;
	}

	const FVector2D CellSize((Max.X - Min.X) / Width, (Max.Y - Min.Y) / Height);
	const int32 MinX = FMath::Clamp(FMath::FloorToInt((WorldLocation.X - Radius - Min.X) / CellSize.X), 0, Width - 1);
	const int32 MaxX = FMath::Clamp(FMath::FloorToInt((WorldLocation.X + Radius - Min.X) / CellSize.X), 0, Width - 1);
	const int32 MinY = FMath::Clamp(FMath::FloorToInt((WorldLocation.Y - Radius - Min.Y) / CellSize.Y), 0, Height - 1);
	const int32 MaxY = FMath::Clamp(FMath::FloorToInt((WorldLocation.Y + Radius - Min.Y) / CellSize.Y), 0, Height - 1);
	const float RadiusSquared = FMath::Square(Radius);

	for (int32 Y = MinY; Y <= MaxY; ++Y)
	{
		for (int32 X = MinX; X <= MaxX; ++X)
		{
			const FVector2D CellCenter(Min.X + (X + 0.5f) * CellSize.X, Min.Y + (Y + 0.5f) * CellSize.Y);
			if (FVector2D::DistSquared(CellCenter, WorldLocation) <= RadiusSquared)
			{
				const int32 Index = Y * Width + X;
				Visible[Index] = true;
				Explored[Index] = true;
			}
		}
	}
}

bool FStrategyFogGrid::IsVisible(const FVector2D& WorldLocation) const
{
	const int32 Index = ToIndex(WorldLocation);
	return Visible.IsValidIndex(Index) && Visible[Index];
}

bool FStrategyFogGrid::IsExplored(const FVector2D& WorldLocation) const
{
	const int32 Index = ToIndex(WorldLocation);
	return Explored.IsValidIndex(Index) && Explored[Index];
}

int32 FStrategyFogGrid::ToIndex(const FVector2D& WorldLocation) const
{
	if (Width <= 0 || Height <= 0 || WorldLocation.X < Min.X || WorldLocation.Y < Min.Y || WorldLocation.X > Max.X || WorldLocation.Y > Max.Y)
	{
		return INDEX_NONE;
	}

	const float NormalizedX = (WorldLocation.X - Min.X) / (Max.X - Min.X);
	const float NormalizedY = (WorldLocation.Y - Min.Y) / (Max.Y - Min.Y);
	const int32 X = FMath::Clamp(FMath::FloorToInt(NormalizedX * Width), 0, Width - 1);
	const int32 Y = FMath::Clamp(FMath::FloorToInt(NormalizedY * Height), 0, Height - 1);
	return Y * Width + X;
}

EStrategyAIAction FStrategyAIPlanner::ChooseAction(const FStrategyAIInputs& Inputs)
{
	if (Inputs.bOwnedPointThreatened)
	{
		return EStrategyAIAction::Defend;
	}
	if (Inputs.bNeutralPointAvailable)
	{
		return EStrategyAIAction::Capture;
	}
	if (Inputs.bMissingProductionBuilding)
	{
		return EStrategyAIAction::Build;
	}
	if (Inputs.AvailableSquads >= 3)
	{
		return EStrategyAIAction::Attack;
	}
	return EStrategyAIAction::Train;
}

FVector2D FStrategyCameraMovement::ResolveScreenDirection(bool bUp, bool bDown, bool bLeft, bool bRight)
{
	return FVector2D(
		static_cast<float>(bRight) - static_cast<float>(bLeft),
		static_cast<float>(bUp) - static_cast<float>(bDown));
}

FVector FStrategyCameraMovement::ScreenToWorld(const FVector2D& ScreenDirection, float CameraYaw)
{
	return FRotator(0.0f, CameraYaw, 0.0f).RotateVector(FVector(ScreenDirection.Y, ScreenDirection.X, 0.0f));
}

TArray<FStrategyWallSegmentPlan> FStrategyWallPlanner::BuildLine(const FVector& Start, const FVector& End, float SegmentLength, int32 MaxSegments)
{
	TArray<FStrategyWallSegmentPlan> Result;
	const FVector Delta = End - Start;
	const float Distance = Delta.Size2D();
	if (Distance <= 0.0f || SegmentLength <= 0.0f || MaxSegments <= 0)
	{
		return Result;
	}

	const FVector Direction = FVector(Delta.X, Delta.Y, 0.0f).GetSafeNormal();
	const int32 SegmentCount = FMath::Min(FMath::CeilToInt(Distance / SegmentLength), MaxSegments);
	const FRotator Rotation(0.0f, Direction.Rotation().Yaw, 0.0f);
	Result.Reserve(SegmentCount);
	for (int32 Index = 0; Index < SegmentCount; ++Index)
	{
		Result.Add({Start + Direction * ((Index + 0.5f) * SegmentLength), Rotation});
	}
	return Result;
}

int32 FStrategyWallPlanner::GetAffordableCount(int32 ValidSegmentCount, float Gold, float SegmentCost)
{
	return SegmentCost > 0.0f
		? FMath::Clamp(FMath::FloorToInt(Gold / SegmentCost), 0, ValidSegmentCount)
		: ValidSegmentCount;
}

TArray<int32> FStrategyWallPlacementRules::SelectBuildableIndices(const TArray<bool>& ValidSegments, float Gold, float SegmentCost)
{
	TArray<int32> Result;
	for (int32 Index = 0; Index < ValidSegments.Num() && Gold >= SegmentCost; ++Index)
	{
		if (ValidSegments[Index])
		{
			Result.Add(Index);
			Gold -= SegmentCost;
		}
	}
	return Result;
}

bool FStrategyWallPlacementRules::CanUpgradeToGate(EStrategyFaction RequestingFaction, EStrategyFaction WallFaction,
	EStrategyBuildingType BuildingType, bool bConstructionComplete)
{
	return RequestingFaction == WallFaction
		&& BuildingType == EStrategyBuildingType::Wall
		&& bConstructionComplete;
}

bool FStrategyGateCollisionRules::ShouldBlock(EStrategyFaction GateFaction, EStrategyFaction UnitFaction, bool bConstructionComplete)
{
	return !bConstructionComplete || GateFaction != UnitFaction;
}

bool FStrategyGateCollisionRules::CanAttackBlockingBuilding(EStrategyFaction UnitFaction, EStrategyFaction BuildingFaction,
	EStrategyBuildingType BuildingType)
{
	return BuildingFaction != EStrategyFaction::Neutral
		&& UnitFaction != BuildingFaction
		&& BuildingType == EStrategyBuildingType::Gate;
}

bool FStrategyTowerTargetRules::CanTarget(EStrategyFaction TowerFaction, EStrategyFaction TargetFaction, bool bConstructionComplete,
	bool bTargetAlive, bool bTargetVisible, float Distance, float AttackRange)
{
	return bConstructionComplete
		&& TargetFaction != TowerFaction
		&& TargetFaction != EStrategyFaction::Neutral
		&& bTargetAlive
		&& bTargetVisible
		&& Distance <= AttackRange;
}

int32 FStrategySquadMarkerRules::FindHoveredMarker(const TArray<FVector2D>& MarkerPositions,
	const FVector2D& CursorPosition, float HitRadius)
{
	int32 ClosestIndex = INDEX_NONE;
	float ClosestDistanceSquared = FMath::Square(HitRadius);
	for (int32 Index = 0; Index < MarkerPositions.Num(); ++Index)
	{
		const float DistanceSquared = FVector2D::DistSquared(MarkerPositions[Index], CursorPosition);
		if (DistanceSquared <= ClosestDistanceSquared)
		{
			ClosestIndex = Index;
			ClosestDistanceSquared = DistanceSquared;
		}
	}
	return ClosestIndex;
}

bool FStrategySquadMarkerRules::ShouldCommandSelectedSquads(bool bSourceSelected)
{
	return bSourceSelected;
}

EStrategyOrderType FStrategySquadMarkerRules::ResolveOrderType(bool bVisibleEnemyTarget)
{
	return bVisibleEnemyTarget ? EStrategyOrderType::AttackTarget : EStrategyOrderType::Move;
}

float FStrategySquadMarkerRules::CalculateHealthPercent(float CurrentHealth, float InitialTotalHealth)
{
	return FMath::Clamp(CurrentHealth / InitialTotalHealth, 0.0f, 1.0f);
}

bool FStrategySquadMarkerRules::CanInteract(bool bBuildingPlacementActive, bool bWallPlacementActive)
{
	return !bBuildingPlacementActive && !bWallPlacementActive;
}
