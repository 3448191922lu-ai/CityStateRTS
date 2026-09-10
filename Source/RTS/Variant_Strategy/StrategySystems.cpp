#include "StrategySystems.h"

bool FStrategyTownDevelopmentRules::CanStartSpecialization(const FStrategyTownDevelopment& Town,
	EStrategyTownSpecialization Specialization, float Gold)
{
	return Town.State == EStrategyTownDevelopmentState::Unspecialized
		&& Specialization != EStrategyTownSpecialization::None
		&& Gold >= SpecializationCost;
}

bool FStrategyTownDevelopmentRules::CanStartDowngrade(const FStrategyTownDevelopment& Town)
{
	return Town.State == EStrategyTownDevelopmentState::Active
		&& Town.Specialization != EStrategyTownSpecialization::None;
}

void FStrategyTownDevelopmentRules::StartSpecialization(FStrategyTownDevelopment& Town,
	EStrategyTownSpecialization Specialization)
{
	Town.Specialization = Specialization;
	Town.State = EStrategyTownDevelopmentState::Building;
	Town.ProgressSeconds = 0.0f;
}

void FStrategyTownDevelopmentRules::StartDowngrade(FStrategyTownDevelopment& Town)
{
	Town.State = EStrategyTownDevelopmentState::Downgrading;
	Town.ProgressSeconds = 0.0f;
}

void FStrategyTownDevelopmentRules::HandleOwnershipChanged(FStrategyTownDevelopment& Town)
{
	if (Town.State == EStrategyTownDevelopmentState::Building)
	{
		Town.Specialization = EStrategyTownSpecialization::None;
		Town.State = EStrategyTownDevelopmentState::Unspecialized;
	}
	else if (Town.State == EStrategyTownDevelopmentState::Active
		|| Town.State == EStrategyTownDevelopmentState::Downgrading)
	{
		Town.State = EStrategyTownDevelopmentState::DisabledAfterCapture;
	}
	Town.ProgressSeconds = 0.0f;
}

EStrategyTownUpdateResult FStrategyTownDevelopmentRules::Update(FStrategyTownDevelopment& Town,
	float DeltaSeconds, bool bContested, bool bOwnerSquadPresent)
{
	if (bContested)
	{
		return EStrategyTownUpdateResult::None;
	}

	if (Town.State == EStrategyTownDevelopmentState::Building)
	{
		Town.ProgressSeconds += DeltaSeconds;
		if (Town.ProgressSeconds >= BuildDuration)
		{
			Town.ProgressSeconds = BuildDuration;
			Town.State = EStrategyTownDevelopmentState::Active;
			return EStrategyTownUpdateResult::BuildCompleted;
		}
	}
	else if (Town.State == EStrategyTownDevelopmentState::Downgrading)
	{
		Town.ProgressSeconds += DeltaSeconds;
		if (Town.ProgressSeconds >= DowngradeDuration)
		{
			Town.Specialization = EStrategyTownSpecialization::None;
			Town.State = EStrategyTownDevelopmentState::Unspecialized;
			Town.ProgressSeconds = 0.0f;
			return EStrategyTownUpdateResult::DowngradeCompleted;
		}
	}
	else if (Town.State == EStrategyTownDevelopmentState::DisabledAfterCapture)
	{
		Town.ProgressSeconds = bOwnerSquadPresent
			? Town.ProgressSeconds + DeltaSeconds
			: FMath::Max(0.0f, Town.ProgressSeconds - DeltaSeconds * ReactivationDuration / ReactivationRecoveryDuration);
		if (Town.ProgressSeconds >= ReactivationDuration)
		{
			Town.ProgressSeconds = ReactivationDuration;
			Town.State = EStrategyTownDevelopmentState::Active;
			return EStrategyTownUpdateResult::ReactivationCompleted;
		}
	}
	return EStrategyTownUpdateResult::None;
}

float FStrategyTownDevelopmentRules::GetDowngradeRefund()
{
	return SpecializationCost * 0.4f;
}

namespace
{
	bool IsActiveFortress(EStrategyTownSpecialization Specialization, EStrategyTownDevelopmentState State)
	{
		return Specialization == EStrategyTownSpecialization::Fortress
			&& State == EStrategyTownDevelopmentState::Active;
	}
}

int32 FStrategyGarrisonRules::GetCapacity(bool, EStrategyTownSpecialization Specialization,
	EStrategyTownDevelopmentState State)
{
	return IsActiveFortress(Specialization, State) ? 3 : 2;
}

float FStrategyGarrisonRules::GetRecoveryDelay(EStrategyTownSpecialization Specialization,
	EStrategyTownDevelopmentState State)
{
	return IsActiveFortress(Specialization, State) ? 0.0f : 3.0f;
}

float FStrategyGarrisonRules::GetRecoveryRate(EStrategyTownSpecialization Specialization,
	EStrategyTownDevelopmentState State)
{
	return IsActiveFortress(Specialization, State) ? 0.05f : 0.03f;
}

float FStrategyGarrisonRules::GetReinforcementInterval(EStrategyTownSpecialization Specialization,
	EStrategyTownDevelopmentState State)
{
	return IsActiveFortress(Specialization, State) ? 6.0f : 8.0f;
}

bool FStrategyGarrisonRules::ShouldExitForDestination(float Distance)
{
	return Distance > ExitDistance;
}

bool FStrategyGarrisonRules::ShouldAIRetreat(float HealthPercent)
{
	return HealthPercent < AIRetreatHealth;
}

bool FStrategyGarrisonRules::ShouldAILeave(float HealthPercent)
{
	return HealthPercent >= AILeaveHealth;
}

float FStrategyGarrisonRules::GetRecoveryAmount(float InitialTotalHealth, float RecoveryRate, float DeltaSeconds)
{
	return InitialTotalHealth * RecoveryRate * DeltaSeconds;
}

float FStrategyGarrisonRules::ClampRecoveredHealth(float CurrentHealth, float MaxHealth, float RecoveryAmount)
{
	return FMath::Min(MaxHealth, CurrentHealth + RecoveryAmount);
}

bool FStrategyGarrisonRules::ShouldReinforce(int32 CurrentMembers, int32 InitialMembers,
	float ElapsedSeconds, float IntervalSeconds)
{
	return CurrentMembers < InitialMembers && ElapsedSeconds >= IntervalSeconds;
}

bool FStrategyGarrisonRules::CanEnter(EStrategyFaction SquadFaction, EStrategyFaction Owner,
	bool bSquadAlive, bool bAlreadyGarrisoned, int32 CurrentCount, int32 Capacity)
{
	return Owner != EStrategyFaction::Neutral && SquadFaction == Owner && bSquadAlive
		&& !bAlreadyGarrisoned && CurrentCount < Capacity;
}

bool FStrategyGarrisonRules::ShouldSortie(EStrategyFaction Owner, bool bEnemyPresent,
	bool, int32 GarrisonCount)
{
	return Owner != EStrategyFaction::Neutral && bEnemyPresent && GarrisonCount > 0;
}

int32 FStrategyGarrisonRules::FindBestDestination(const FVector2D& Origin,
	const TArray<FStrategyGarrisonDestination>& Destinations)
{
	int32 BestIndex = INDEX_NONE;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	int32 BestPriority = MAX_int32;
	for (int32 Index = 0; Index < Destinations.Num(); ++Index)
	{
		const FStrategyGarrisonDestination& Candidate = Destinations[Index];
		if (Candidate.AvailableSlots <= 0)
		{
			continue;
		}
		const float DistanceSquared = FVector2D::DistSquared(Origin, Candidate.Location);
		const int32 Priority = Candidate.bActiveFortress ? 0 : Candidate.bCapital ? 2 : 1;
		if (DistanceSquared < BestDistanceSquared
			|| (FMath::IsNearlyEqual(DistanceSquared, BestDistanceSquared) && Priority < BestPriority))
		{
			BestIndex = Index;
			BestDistanceSquared = DistanceSquared;
			BestPriority = Priority;
		}
	}
	return BestIndex;
}

bool FStrategyGarrisonCommandRules::ShouldEnterPoint(EStrategyFaction SquadFaction,
	EStrategyFaction PointOwner, bool bTargetIsControlPoint)
{
	return bTargetIsControlPoint && PointOwner != EStrategyFaction::Neutral && SquadFaction == PointOwner;
}

bool FStrategyGarrisonCommandRules::ShouldExitForOrder(EStrategyOrderType OrderType, float Distance)
{
	return OrderType != EStrategyOrderType::Stop && FStrategyGarrisonRules::ShouldExitForDestination(Distance);
}

FVector FStrategyBuildingPlacementRules::ResolveGroundLocation(const FVector& RequestedLocation,
	const FVector& ProjectedGround)
{
	return FVector(RequestedLocation.X, RequestedLocation.Y, ProjectedGround.Z);
}

TSet<int32> FStrategySupplyRules::FindConnectedTownIndices(const TArray<FStrategySupplyNode>& Nodes,
	EStrategyFaction Faction)
{
	TSet<int32> Visited;
	TArray<int32> Pending;
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		if (Nodes[Index].Owner == Faction && Nodes[Index].bCapital)
		{
			Visited.Add(Index);
			Pending.Add(Index);
		}
	}

	for (int32 PendingIndex = 0; PendingIndex < Pending.Num(); ++PendingIndex)
	{
		const int32 SourceIndex = Pending[PendingIndex];
		for (int32 TargetIndex = 0; TargetIndex < Nodes.Num(); ++TargetIndex)
		{
			if (!Visited.Contains(TargetIndex)
				&& Nodes[TargetIndex].Owner == Faction
				&& FVector2D::DistSquared(Nodes[SourceIndex].Location, Nodes[TargetIndex].Location) <= FMath::Square(LinkDistance))
			{
				Visited.Add(TargetIndex);
				Pending.Add(TargetIndex);
			}
		}
	}

	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		if (Nodes[Index].bCapital)
		{
			Visited.Remove(Index);
		}
	}
	return Visited;
}

float FStrategyTownSpecializationRules::GetIncomeBonus(EStrategyTownSpecialization Specialization,
	EStrategyTownDevelopmentState State, bool bConnected)
{
	return State == EStrategyTownDevelopmentState::Active && Specialization == EStrategyTownSpecialization::Trade
		? (bConnected ? 6.0f : 4.0f)
		: 0.0f;
}

int32 FStrategyTownSpecializationRules::GetPopulationBonus(EStrategyTownSpecialization Specialization,
	EStrategyTownDevelopmentState State)
{
	return State == EStrategyTownDevelopmentState::Active && Specialization == EStrategyTownSpecialization::Recruitment
		? 10
		: 0;
}

float FStrategyTownSpecializationRules::GetTrainingTimeMultiplier(EStrategyTownSpecialization Specialization,
	EStrategyTownDevelopmentState State, bool bConnected)
{
	return State == EStrategyTownDevelopmentState::Active && Specialization == EStrategyTownSpecialization::Recruitment
		? (bConnected ? 0.7f : 0.8f)
		: 1.0f;
}

float FStrategyTownSpecializationRules::GetCaptureDuration(EStrategyTownSpecialization Specialization,
	EStrategyTownDevelopmentState State)
{
	return State == EStrategyTownDevelopmentState::Active && Specialization == EStrategyTownSpecialization::Fortress
		? 15.0f
		: 10.0f;
}

float FStrategyTownSpecializationRules::GetFortressRange(EStrategyTownSpecialization Specialization,
	EStrategyTownDevelopmentState State, bool bConnected)
{
	return State == EStrategyTownDevelopmentState::Active && Specialization == EStrategyTownSpecialization::Fortress
		? (bConnected ? 1500.0f : 1200.0f)
		: 0.0f;
}

float FStrategyTownSpecializationRules::GetFortressDamage(EStrategyTownSpecialization Specialization,
	EStrategyTownDevelopmentState State, bool bConnected)
{
	return State == EStrategyTownDevelopmentState::Active && Specialization == EStrategyTownSpecialization::Fortress
		? (bConnected ? 25.0f : 20.0f)
		: 0.0f;
}

FStrategyFactionEconomyTotals FStrategyFactionEconomyRules::Calculate(EStrategyFaction Faction,
	const TArray<FStrategyTownContribution>& Contributions, int32 CompletedBuildingPopulation)
{
	FStrategyFactionEconomyTotals Totals;
	Totals.PopulationCap = CompletedBuildingPopulation;
	for (const FStrategyTownContribution& Contribution : Contributions)
	{
		if (Contribution.Owner == Faction)
		{
			Totals.IncomePerSecond += Contribution.IncomePerSecond;
			Totals.PopulationCap += Contribution.PopulationCap;
			++Totals.OwnedPoints;
		}
	}
	Totals.PopulationCap = FMath::Min(Totals.PopulationCap, 60);
	return Totals;
}

bool FStrategyTownActionRules::CanChooseSpecialization(EStrategyFaction Viewer, EStrategyFaction Owner,
	bool bCapital, EStrategyTownDevelopmentState State)
{
	return Viewer == Owner && !bCapital && State == EStrategyTownDevelopmentState::Unspecialized;
}

bool FStrategyTownActionRules::CanDowngrade(EStrategyFaction Viewer, EStrategyFaction Owner,
	bool bCapital, EStrategyTownDevelopmentState State)
{
	return Viewer == Owner && !bCapital && State == EStrategyTownDevelopmentState::Active;
}

bool FStrategyTownVisibilityRules::CanShowLiveDetails(EStrategyFaction Viewer, EStrategyFaction Owner,
	bool bCurrentlyVisible)
{
	return Viewer == Owner;
}

bool FStrategyTownVisibilityRules::CanShowPublicDetails(EStrategyFaction Viewer, EStrategyFaction Owner,
	bool bCurrentlyVisible)
{
	return Viewer == Owner || bCurrentlyVisible;
}

bool FStrategyTownVisibilityRules::CanShowSupplyConnection(EStrategyFaction Viewer, EStrategyFaction Owner)
{
	return Viewer == Owner;
}

EStrategyTownSpecialization FStrategyTownVisibilityRules::GetPublicSpecialization(EStrategyFaction Viewer,
	EStrategyFaction Owner, bool bCurrentlyVisible, EStrategyTownSpecialization Specialization,
	EStrategyTownDevelopmentState State)
{
	if (!CanShowPublicDetails(Viewer, Owner, bCurrentlyVisible))
	{
		return EStrategyTownSpecialization::None;
	}
	return Viewer != Owner && State == EStrategyTownDevelopmentState::Building
		? EStrategyTownSpecialization::None
		: Specialization;
}

bool FStrategyTownPanelLayoutRules::ShouldUseDesiredSize()
{
	return true;
}

bool FStrategyTrainingQueue::Enqueue(EStrategyUnitType UnitType, float TrainingTime, int32 PopulationCost)
{
	if (Items.Num() >= 5)
	{
		return false;
	}

	Items.Add({UnitType, TrainingTime, TrainingTime, PopulationCost});
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

float FStrategyTrainingQueue::GetFrontProgress() const
{
	return Items.IsEmpty() ? 0.0f
		: 1.0f - FMath::Clamp(Items[0].RemainingTime / Items[0].TotalTime, 0.0f, 1.0f);
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

EStrategyTownSpecialization FStrategyTownAIPlanner::ChooseSpecialization(const FStrategyTownAIInputs& Inputs)
{
	if (Inputs.bTownThreatened || (Inputs.bHasTradeTown && Inputs.bHasRecruitmentTown))
	{
		return EStrategyTownSpecialization::Fortress;
	}
	return Inputs.bHasTradeTown
		? EStrategyTownSpecialization::Recruitment
		: EStrategyTownSpecialization::Trade;
}

bool FStrategyTownAIPlanner::ShouldRespecialize(EStrategyTownSpecialization Current,
	EStrategyTownSpecialization Preferred, float Gold)
{
	return Current != Preferred && Gold >= 500.0f;
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

FVector FStrategyCameraMovement::GetOrbitCameraLocation(const FVector& CameraLocation, float CameraPitch,
	float CurrentYaw, float DesiredYaw)
{
	const FVector CurrentForward = FRotator(CameraPitch, CurrentYaw, 0.0f).Vector();
	const FVector GroundFocus = CameraLocation + CurrentForward * (CameraLocation.Z / -CurrentForward.Z);
	const FVector DesiredForward = FRotator(CameraPitch, DesiredYaw, 0.0f).Vector();
	return GroundFocus - DesiredForward * (CameraLocation.Z / -DesiredForward.Z);
}

FVector2D FStrategyCameraBoundsRules::GetGroundViewExtents(float OrthoWidth, float AspectRatio, float PitchDegrees,
	float YawDegrees)
{
	const float HalfHeight = OrthoWidth * 0.5f;
	const float HalfWidth = HalfHeight * AspectRatio;
	const float GroundHalfHeight = HalfHeight / FMath::Sin(FMath::DegreesToRadians(PitchDegrees));
	const float YawRadians = FMath::DegreesToRadians(YawDegrees);
	const FVector2D Forward(FMath::Cos(YawRadians), FMath::Sin(YawRadians));
	const FVector2D Right(-Forward.Y, Forward.X);
	return FVector2D(
		FMath::Abs(Right.X) * HalfWidth + FMath::Abs(Forward.X) * GroundHalfHeight,
		FMath::Abs(Right.Y) * HalfWidth + FMath::Abs(Forward.Y) * GroundHalfHeight);
}

FVector2D FStrategyCameraBoundsRules::ClampGroundFocus(const FVector2D& DesiredFocus, const FVector2D& MapMin,
	const FVector2D& MapMax, const FVector2D& ViewExtents, float Padding)
{
	const FVector2D Minimum = MapMin + ViewExtents + FVector2D(Padding);
	const FVector2D Maximum = MapMax - ViewExtents - FVector2D(Padding);
	return FVector2D(
		Minimum.X <= Maximum.X ? FMath::Clamp(DesiredFocus.X, Minimum.X, Maximum.X) : (MapMin.X + MapMax.X) * 0.5f,
		Minimum.Y <= Maximum.Y ? FMath::Clamp(DesiredFocus.Y, Minimum.Y, Maximum.Y) : (MapMin.Y + MapMax.Y) * 0.5f);
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

float FStrategyIntroPromptRules::GetOpacity(float ElapsedSeconds)
{
	return FMath::Clamp(8.0f - ElapsedSeconds, 0.0f, 1.0f);
}

bool FStrategyOrderTargetRules::CanAttack(EStrategyFaction SourceFaction, EStrategyFaction TargetFaction,
	bool bTargetAlive, bool bTargetVisible)
{
	return TargetFaction != EStrategyFaction::Neutral
		&& TargetFaction != SourceFaction
		&& bTargetAlive
		&& bTargetVisible;
}

EStrategyUnitVisualState FStrategyPresentationRules::ResolveUnitVisualState(bool bAlive, bool bAttacking, float SpeedSquared)
{
	if (!bAlive)
	{
		return EStrategyUnitVisualState::Dead;
	}
	if (bAttacking)
	{
		return EStrategyUnitVisualState::Attack;
	}
	return SpeedSquared > KINDA_SMALL_NUMBER ? EStrategyUnitVisualState::Move : EStrategyUnitVisualState::Idle;
}

bool FStrategyPresentationRules::CanPlayWorldFeedback(EStrategyFaction SourceFaction, bool bVisibleToPlayer)
{
	return SourceFaction == EStrategyFaction::Player || bVisibleToPlayer;
}

FRotator FStrategyPresentationRules::GetImportedCharacterMeshRotation()
{
	// Quaternius FBX 模型正面为 +Y，旋转到 ACharacter 使用的 +X 前向。
	return FRotator(0.0f, -90.0f, 0.0f);
}
