#include "StrategyGameState.h"

#include "Engine/World.h"
#include "NavigationSystem.h"
#include "StrategyRules.h"
#include "StrategyMapDefinition.h"
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

	constexpr const TCHAR* PresentationPath = TEXT("/Game/CityStateRTS/Data/DA_Presentation.DA_Presentation");
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

const UStrategyPresentationDataAsset* AStrategyGameState::GetPresentationDefinition()
{
	EnsureDefinitionsLoaded();
	return PresentationDefinition;
}

const UStrategyPresentationDataAsset* AStrategyGameState::GetPresentationDefinition() const
{
	return const_cast<AStrategyGameState*>(this)->GetPresentationDefinition();
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

	FNavLocation ProjectedLocation;
	UNavigationSystemV1::GetCurrent(GetWorld())->ProjectPointToNavigation(Location, ProjectedLocation,
		FVector(Definition->FootprintExtent.X, Definition->FootprintExtent.Y, 300.0f));
	const FVector GroundLocation = FStrategyBuildingPlacementRules::ResolveGroundLocation(Location, ProjectedLocation.Location);
	AStrategyBuilding* Building = GetWorld()->SpawnActor<AStrategyBuilding>(AStrategyBuilding::StaticClass(), GroundLocation, Rotation);
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
	return GetBuildingPlacementIssue(Faction, BuildingType, Location, Rotation)
		== EStrategyBuildingPlacementIssue::None;
}

void AStrategyGameState::NotifyFaction(EStrategyFaction Faction, const FString& Message)
{
	FactionNotification.Broadcast(Faction, Message);
}

EStrategyBuildingPlacementIssue AStrategyGameState::GetBuildingPlacementIssue(EStrategyFaction Faction,
	EStrategyBuildingType BuildingType, const FVector& Location, const FRotator& Rotation)
{
	const UStrategyBuildingDataAsset* Definition = GetBuildingDefinition(BuildingType);
	const FStrategySkirmishMapDefinition MapDefinition = FStrategyMapDefinitions::Resolve(GetWorld()->GetMapName());
	if (!FStrategyMapDefinitions::IsBuildingAllowed(MapDefinition, Location, Definition->FootprintExtent))
	{
		return EStrategyBuildingPlacementIssue::MapRestricted;
	}
	if (!IsFootprintInTerritory(Faction, Location, Definition->FootprintExtent, Rotation.Yaw))
	{
		return EStrategyBuildingPlacementIssue::OutsideTerritory;
	}

	FNavLocation ProjectedLocation;
	if (!UNavigationSystemV1::GetCurrent(GetWorld())->ProjectPointToNavigation(Location, ProjectedLocation,
		FVector(Definition->FootprintExtent.X, Definition->FootprintExtent.Y, 300.0f)))
	{
		return EStrategyBuildingPlacementIssue::NotNavigable;
	}

	FCollisionObjectQueryParams ObjectTypes;
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectTypes.AddObjectTypesToQuery(ECC_GameTraceChannel1);
	ObjectTypes.AddObjectTypesToQuery(ECC_GameTraceChannel2);
	const FVector GroundLocation = FStrategyBuildingPlacementRules::ResolveGroundLocation(Location, ProjectedLocation.Location);
	return GetWorld()->OverlapAnyTestByObjectType(GroundLocation + FVector(0.0f, 0.0f, 251.0f), Rotation.Quaternion(), ObjectTypes,
		FCollisionShape::MakeBox(FVector(Definition->FootprintExtent.X, Definition->FootprintExtent.Y, 250.0f)))
		? EStrategyBuildingPlacementIssue::Overlap : EStrategyBuildingPlacementIssue::None;
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
	RecalculateFactionEconomy();
}

void AStrategyGameState::UnregisterBuilding(AStrategyBuilding* Building)
{
	Buildings.Remove(Building);
	RecalculateFactionEconomy();
}

void AStrategyGameState::RegisterControlPoint(AStrategyControlPoint* Point)
{
	ControlPoints.AddUnique(Point);
	RecalculateFactionEconomy();
}

void AStrategyGameState::ChangeControlPointOwner(AStrategyControlPoint* Point, EStrategyFaction OldOwner, EStrategyFaction NewOwner)
{
	RecalculateFactionEconomy();
}

bool AStrategyGameState::TryStartTownSpecialization(EStrategyFaction Faction, AStrategyControlPoint* Town,
	EStrategyTownSpecialization Specialization)
{
	if (!Town || Town->IsCapital() || Town->GetStrategyFaction() != Faction
		|| !FStrategyTownDevelopmentRules::CanStartSpecialization(Town->GetTownDevelopment(), Specialization,
			GetFactionState(Faction).Gold)
		|| !TrySpendAndReserve(Faction, FStrategyTownDevelopmentRules::SpecializationCost, 0))
	{
		return false;
	}
	return Town->StartSpecialization(Specialization);
}

bool AStrategyGameState::TryStartTownDowngrade(EStrategyFaction Faction, AStrategyControlPoint* Town)
{
	return Town && !Town->IsCapital() && Town->GetStrategyFaction() == Faction && Town->StartDowngrade();
}

void AStrategyGameState::NotifyTownDevelopmentChanged(AStrategyControlPoint* Town)
{
	RecalculateFactionEconomy();
}

void AStrategyGameState::RecalculateFactionEconomy()
{
	TArray<FStrategyTownContribution> Contributions;
	for (const AStrategyControlPoint* Point : ControlPoints)
	{
		if (!IsValid(Point))
		{
			continue;
		}
		const bool bConnected = IsTownSupplyConnected(Point);
		Contributions.Add({Point->GetStrategyFaction(),
			Point->GetIncomePerSecond() + FStrategyTownSpecializationRules::GetIncomeBonus(
				Point->GetTownDevelopment().Specialization, Point->GetTownDevelopment().State, bConnected),
			Point->GetPopulationBonus() + FStrategyTownSpecializationRules::GetPopulationBonus(
				Point->GetTownDevelopment().Specialization, Point->GetTownDevelopment().State)});
	}

	for (const EStrategyFaction Faction : {EStrategyFaction::Player, EStrategyFaction::Enemy})
	{
		int32 BuildingPopulation = 0;
		for (const AStrategyBuilding* Building : Buildings)
		{
			if (IsValid(Building) && Building->GetStrategyFaction() == Faction && Building->IsConstructionComplete())
			{
				BuildingPopulation += GetBuildingDefinition(Building->GetBuildingType())->PopulationBonus;
			}
		}
		const FStrategyFactionEconomyTotals Totals = FStrategyFactionEconomyRules::Calculate(
			Faction, Contributions, BuildingPopulation);
		FStrategyFactionState& State = GetMutableFactionState(Faction);
		State.IncomePerSecond = Totals.IncomePerSecond;
		State.PopulationCap = Totals.PopulationCap;
		State.OwnedPoints = Totals.OwnedPoints;
	}
}

bool AStrategyGameState::IsTownSupplyConnected(const AStrategyControlPoint* Town) const
{
	if (!IsValid(Town) || Town->IsCapital() || Town->GetStrategyFaction() == EStrategyFaction::Neutral)
	{
		return false;
	}
	TArray<FStrategySupplyNode> Nodes;
	int32 TownIndex = INDEX_NONE;
	for (const AStrategyControlPoint* Point : ControlPoints)
	{
		if (IsValid(Point))
		{
			if (Point == Town)
			{
				TownIndex = Nodes.Num();
			}
			const FVector Location = Point->GetActorLocation();
			Nodes.Add({FVector2D(Location.X, Location.Y), Point->GetStrategyFaction(), Point->IsCapital()});
		}
	}
	return FStrategySupplyRules::FindConnectedTownIndices(Nodes, Town->GetStrategyFaction()).Contains(TownIndex);
}

float AStrategyGameState::GetTrainingTimeMultiplierAt(EStrategyFaction Faction, const FVector& Location) const
{
	float Multiplier = 1.0f;
	for (const AStrategyControlPoint* Point : ControlPoints)
	{
		if (IsValid(Point) && !Point->IsCapital() && Point->GetStrategyFaction() == Faction
			&& FVector::DistSquared2D(Location, Point->GetActorLocation()) <= FMath::Square(Point->GetTerritoryRadius()))
		{
			Multiplier = FMath::Min(Multiplier, FStrategyTownSpecializationRules::GetTrainingTimeMultiplier(
				Point->GetTownDevelopment().Specialization, Point->GetTownDevelopment().State,
				IsTownSupplyConnected(Point)));
		}
	}
	return Multiplier;
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

bool AStrategyGameState::IsExploredToFaction(EStrategyFaction Faction, const FVector& Location) const
{
	return !FogOfWar || FogOfWar->IsExploredToFaction(Faction, Location);
}

UTexture2D* AStrategyGameState::GetPlayerFogTexture() const
{
	return FogOfWar ? FogOfWar->GetPlayerFogTexture() : nullptr;
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

	PresentationDefinition = LoadObject<UStrategyPresentationDataAsset>(nullptr, StrategyDataPaths::PresentationPath);
	checkf(PresentationDefinition, TEXT("缺少共享表现数据资产：%s"), StrategyDataPaths::PresentationPath);
}
