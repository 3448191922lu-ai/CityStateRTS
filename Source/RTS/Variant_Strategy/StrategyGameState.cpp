#include "StrategyGameState.h"

#include "Engine/World.h"
#include "NavigationSystem.h"
#include "StrategyRules.h"
#include "StrategySystems.h"
#include "StrategyWorldActors.h"

namespace StrategyDataPaths
{
	static const TCHAR* UnitPaths[] = {
		TEXT("/Game/CityStateRTS/Data/DA_Unit_Infantry.DA_Unit_Infantry"),
		TEXT("/Game/CityStateRTS/Data/DA_Unit_Archer.DA_Unit_Archer"),
		TEXT("/Game/CityStateRTS/Data/DA_Unit_Cavalry.DA_Unit_Cavalry")
	};

	static const TCHAR* BuildingPaths[] = {
		TEXT("/Game/CityStateRTS/Data/DA_Building_Barracks.DA_Building_Barracks"),
		TEXT("/Game/CityStateRTS/Data/DA_Building_ArcheryRange.DA_Building_ArcheryRange"),
		TEXT("/Game/CityStateRTS/Data/DA_Building_Stable.DA_Building_Stable"),
		TEXT("/Game/CityStateRTS/Data/DA_Building_House.DA_Building_House"),
		TEXT("/Game/CityStateRTS/Data/DA_Building_Tower.DA_Building_Tower"),
		TEXT("/Game/CityStateRTS/Data/DA_Building_Wall.DA_Building_Wall"),
		TEXT("/Game/CityStateRTS/Data/DA_Building_Gate.DA_Building_Gate")
	};
}

AStrategyGameState::AStrategyGameState()
{
	PrimaryActorTick.bCanEverTick = true;
	for (EStrategyFaction Faction : {EStrategyFaction::Player, EStrategyFaction::Enemy})
	{
		FStrategyFactionState State;
		State.Gold = 500.0f;
		State.PopulationCap = 0;
		State.OwnedPoints = 0;
		State.IncomePerSecond = 0.0f;
		Factions.Add(Faction, State);
	}
}

void AStrategyGameState::BeginPlay()
{
	Super::BeginPlay();
	EnsureDefinitionsLoaded();
}

void AStrategyGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsMatchRunning())
	{
		return;
	}

	for (TPair<EStrategyFaction, FStrategyFactionState>& Pair : Factions)
	{
		Pair.Value.AddIncome(DeltaSeconds);
	}
}

const FStrategyFactionState& AStrategyGameState::GetFactionState(EStrategyFaction Faction) const
{
	return Factions.FindChecked(Faction);
}

FStrategyFactionState& AStrategyGameState::GetMutableFactionState(EStrategyFaction Faction)
{
	return Factions.FindChecked(Faction);
}

bool AStrategyGameState::TrySpendAndReserve(EStrategyFaction Faction, float Cost, int32 PopulationCost)
{
	return IsMatchRunning() && GetMutableFactionState(Faction).TrySpendAndReserve(Cost, PopulationCost);
}

void AStrategyGameState::CommitPopulation(EStrategyFaction Faction, int32 PopulationCost)
{
	GetMutableFactionState(Faction).CommitPopulation(PopulationCost);
}

void AStrategyGameState::ReleaseReservedPopulation(EStrategyFaction Faction, int32 PopulationCost)
{
	GetMutableFactionState(Faction).ReleaseReservedPopulation(PopulationCost);
}

void AStrategyGameState::RemovePopulation(EStrategyFaction Faction, int32 PopulationCost)
{
	GetMutableFactionState(Faction).RemovePopulation(PopulationCost);
}

void AStrategyGameState::AdjustPopulationCap(EStrategyFaction Faction, int32 Delta)
{
	FStrategyFactionState& State = GetMutableFactionState(Faction);
	State.PopulationCap = FMath::Clamp(State.PopulationCap + Delta, 0, 60);
}

const UStrategyUnitDataAsset* AStrategyGameState::GetUnitDefinition(EStrategyUnitType UnitType)
{
	EnsureDefinitionsLoaded();
	return UnitDefinitions.FindChecked(UnitType);
}

const UStrategyBuildingDataAsset* AStrategyGameState::GetBuildingDefinition(EStrategyBuildingType BuildingType)
{
	EnsureDefinitionsLoaded();
	return BuildingDefinitions.FindChecked(BuildingType);
}

AStrategySquad* AStrategyGameState::SpawnSquad(EStrategyFaction Faction, EStrategyUnitType UnitType, const FVector& Location, bool bPopulationReserved)
{
	const UStrategyUnitDataAsset* Definition = GetUnitDefinition(UnitType);
	AStrategySquad* Squad = GetWorld()->SpawnActor<AStrategySquad>(AStrategySquad::StaticClass(), Location, FRotator::ZeroRotator);
	check(Squad);
	Squad->Initialize(Faction, Definition, bPopulationReserved);
	return Squad;
}

AStrategyBuilding* AStrategyGameState::TryPlaceBuilding(EStrategyFaction Faction, EStrategyBuildingType BuildingType, const FVector& Location,
	const FRotator& Rotation)
{
	const UStrategyBuildingDataAsset* Definition = GetBuildingDefinition(BuildingType);
	if (!CanPlaceBuilding(Faction, BuildingType, Location, Rotation))
	{
		return nullptr;
	}

	if (!TrySpendAndReserve(Faction, Definition->GoldCost, 0))
	{
		return nullptr;
	}

	AStrategyBuilding* Building = GetWorld()->SpawnActor<AStrategyBuilding>(AStrategyBuilding::StaticClass(), Location, Rotation);
	check(Building);
	Building->Initialize(Faction, Definition);
	return Building;
}

int32 AStrategyGameState::TryPlaceWallLine(EStrategyFaction Faction, const FVector& Start, const FVector& End)
{
	const TArray<FStrategyWallSegmentPlan> Plans = FStrategyWallPlanner::BuildLine(Start, End, 400.0f, 30);
	TArray<bool> ValidSegments;
	ValidSegments.Reserve(Plans.Num());
	for (const FStrategyWallSegmentPlan& Plan : Plans)
	{
		ValidSegments.Add(CanPlaceBuilding(Faction, EStrategyBuildingType::Wall, Plan.Location, Plan.Rotation));
	}

	const UStrategyBuildingDataAsset* Definition = GetBuildingDefinition(EStrategyBuildingType::Wall);
	const TArray<int32> BuildableIndices = FStrategyWallPlacementRules::SelectBuildableIndices(
		ValidSegments, GetFactionState(Faction).Gold, Definition->GoldCost);
	int32 BuiltCount = 0;
	for (const int32 Index : BuildableIndices)
	{
		BuiltCount += TryPlaceBuilding(Faction, EStrategyBuildingType::Wall, Plans[Index].Location, Plans[Index].Rotation) != nullptr;
	}
	return BuiltCount;
}

AStrategyBuilding* AStrategyGameState::TryUpgradeWallToGate(EStrategyFaction Faction, AStrategyBuilding* Wall)
{
	if (!Wall || !FStrategyWallPlacementRules::CanUpgradeToGate(Faction, Wall->GetStrategyFaction(),
		Wall->GetBuildingType(), Wall->IsConstructionComplete()))
	{
		return nullptr;
	}

	const UStrategyBuildingDataAsset* GateDefinition = GetBuildingDefinition(EStrategyBuildingType::Gate);
	if (!TrySpendAndReserve(Faction, GateDefinition->GoldCost, 0))
	{
		return nullptr;
	}

	const FTransform GateTransform = Wall->GetActorTransform();
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStrategyBuilding* Gate = GetWorld()->SpawnActor<AStrategyBuilding>(
		AStrategyBuilding::StaticClass(), GateTransform, SpawnParameters);
	check(Gate);
	Gate->Initialize(Faction, GateDefinition);
	Wall->Destroy();
	return Gate;
}

bool AStrategyGameState::CanPlaceBuilding(EStrategyFaction Faction, EStrategyBuildingType BuildingType, const FVector& Location,
	const FRotator& Rotation)
{
	const UStrategyBuildingDataAsset* Definition = GetBuildingDefinition(BuildingType);
	if (!IsFootprintInTerritory(Faction, Location, Definition->FootprintExtent, Rotation.Yaw))
	{
		return false;
	}

	FNavLocation ProjectedLocation;
	if (!UNavigationSystemV1::GetCurrent(GetWorld())->ProjectPointToNavigation(Location, ProjectedLocation,
		FVector(Definition->FootprintExtent.X, Definition->FootprintExtent.Y, 300.0f)))
	{
		return false;
	}

	FCollisionObjectQueryParams ObjectTypes;
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectTypes.AddObjectTypesToQuery(ECC_GameTraceChannel1);
	ObjectTypes.AddObjectTypesToQuery(ECC_GameTraceChannel2);
	return !GetWorld()->OverlapAnyTestByObjectType(Location + FVector(0.0f, 0.0f, 251.0f), Rotation.Quaternion(), ObjectTypes,
		FCollisionShape::MakeBox(FVector(Definition->FootprintExtent.X, Definition->FootprintExtent.Y, 250.0f)));
}

bool AStrategyGameState::IsLocationInTerritory(EStrategyFaction Faction, const FVector& Location, float FootprintRadius) const
{
	for (const AStrategyControlPoint* Point : ControlPoints)
	{
		if (IsValid(Point) && Point->GetStrategyFaction() == Faction && FStrategyRules::IsInsideTerritory(Location, FootprintRadius, Point->GetActorLocation(), Point->GetTerritoryRadius()))
		{
			return true;
		}
	}
	return false;
}

bool AStrategyGameState::IsFootprintInTerritory(EStrategyFaction Faction, const FVector& Location,
	const FVector2D& FootprintExtent, float YawDegrees) const
{
	for (const AStrategyControlPoint* Point : ControlPoints)
	{
		if (IsValid(Point) && Point->GetStrategyFaction() == Faction && FStrategyRules::IsFootprintInsideTerritory(
			Location, FootprintExtent, YawDegrees, Point->GetActorLocation(), Point->GetTerritoryRadius()))
		{
			return true;
		}
	}
	return false;
}

void AStrategyGameState::RegisterSquad(AStrategySquad* Squad)
{
	Squads.AddUnique(Squad);
}

void AStrategyGameState::UnregisterSquad(AStrategySquad* Squad)
{
	Squads.Remove(Squad);
}

void AStrategyGameState::RegisterBuilding(AStrategyBuilding* Building)
{
	Buildings.AddUnique(Building);
}

void AStrategyGameState::UnregisterBuilding(AStrategyBuilding* Building)
{
	Buildings.Remove(Building);
}

void AStrategyGameState::RegisterControlPoint(AStrategyControlPoint* Point)
{
	ControlPoints.AddUnique(Point);
	ApplyPointContribution(Point->GetStrategyFaction(), 1, Point);
}

void AStrategyGameState::ChangeControlPointOwner(AStrategyControlPoint* Point, EStrategyFaction OldOwner, EStrategyFaction NewOwner)
{
	ApplyPointContribution(OldOwner, -1, Point);
	ApplyPointContribution(NewOwner, 1, Point);
}

AStrategyControlPoint* AStrategyGameState::FindCapital(EStrategyFaction Faction) const
{
	for (AStrategyControlPoint* Point : ControlPoints)
	{
		if (IsValid(Point) && Point->IsCapital() && Point->GetStrategyFaction() == Faction)
		{
			return Point;
		}
	}
	return nullptr;
}

bool AStrategyGameState::IsVisibleToFaction(EStrategyFaction Faction, const FVector& Location) const
{
	return !FogOfWar || FogOfWar->IsVisibleToFaction(Faction, Location);
}

void AStrategyGameState::NotifyCapitalDestroyed(EStrategyFaction DestroyedFaction)
{
	if (!IsMatchRunning())
	{
		return;
	}
	Winner = DestroyedFaction == EStrategyFaction::Player ? EStrategyFaction::Enemy : EStrategyFaction::Player;
}

void AStrategyGameState::EnsureDefinitionsLoaded()
{
	if (!UnitDefinitions.IsEmpty())
	{
		return;
	}

	for (int32 Index = 0; Index < 3; ++Index)
	{
		UStrategyUnitDataAsset* Definition = LoadObject<UStrategyUnitDataAsset>(nullptr, StrategyDataPaths::UnitPaths[Index]);
		checkf(Definition, TEXT("缺少单位数据资产：%s"), StrategyDataPaths::UnitPaths[Index]);
		UnitDefinitions.Add(static_cast<EStrategyUnitType>(Index), Definition);
	}

	for (int32 Index = 0; Index < 7; ++Index)
	{
		UStrategyBuildingDataAsset* Definition = LoadObject<UStrategyBuildingDataAsset>(nullptr, StrategyDataPaths::BuildingPaths[Index]);
		checkf(Definition, TEXT("缺少建筑数据资产：%s"), StrategyDataPaths::BuildingPaths[Index]);
		BuildingDefinitions.Add(static_cast<EStrategyBuildingType>(Index), Definition);
	}
}

void AStrategyGameState::ApplyPointContribution(EStrategyFaction Faction, int32 Direction, const AStrategyControlPoint* Point)
{
	if (Faction == EStrategyFaction::Neutral || !Factions.Contains(Faction))
	{
		return;
	}

	FStrategyFactionState& State = GetMutableFactionState(Faction);
	State.OwnedPoints = FMath::Max(0, State.OwnedPoints + Direction);
	State.IncomePerSecond = FMath::Max(0.0f, State.IncomePerSecond + Direction * Point->GetIncomePerSecond());
	State.PopulationCap = FMath::Clamp(State.PopulationCap + Direction * Point->GetPopulationBonus(), 0, 60);
}
