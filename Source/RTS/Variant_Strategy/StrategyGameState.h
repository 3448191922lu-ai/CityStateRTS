#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "StrategyTypes.h"
#include "StrategyGameState.generated.h"

class AStrategyBuilding;
class AStrategyControlPoint;
class AStrategyFogOfWar;
class AStrategySquad;
class UTexture2D;

DECLARE_MULTICAST_DELEGATE_TwoParams(FStrategyFactionNotificationEvent, EStrategyFaction, const FString&);

UCLASS()
class AStrategyGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AStrategyGameState();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	const FStrategyFactionState& GetFactionState(EStrategyFaction Faction) const;
	FStrategyFactionState& GetMutableFactionState(EStrategyFaction Faction);
	bool HasFactionState(EStrategyFaction Faction) const { return Factions.Contains(Faction); }
	bool TrySpendAndReserve(EStrategyFaction Faction, float Cost, int32 PopulationCost);
	void CommitPopulation(EStrategyFaction Faction, int32 PopulationCost);
	void ReleaseReservedPopulation(EStrategyFaction Faction, int32 PopulationCost);
	void RemovePopulation(EStrategyFaction Faction, int32 PopulationCost);
	void AdjustPopulationCap(EStrategyFaction Faction, int32 Delta);

	const UStrategyUnitDataAsset* GetUnitDefinition(EStrategyUnitType UnitType);
	const UStrategyBuildingDataAsset* GetBuildingDefinition(EStrategyBuildingType BuildingType);
	const UStrategyPresentationDataAsset* GetPresentationDefinition();
	const UStrategyPresentationDataAsset* GetPresentationDefinition() const;

	AStrategySquad* SpawnSquad(EStrategyFaction Faction, EStrategyUnitType UnitType, const FVector& Location, bool bPopulationReserved);
	AStrategyBuilding* TryPlaceBuilding(EStrategyFaction Faction, EStrategyBuildingType BuildingType, const FVector& Location,
		const FRotator& Rotation = FRotator::ZeroRotator);
	int32 TryPlaceWallLine(EStrategyFaction Faction, const FVector& Start, const FVector& End);
	AStrategyBuilding* TryUpgradeWallToGate(EStrategyFaction Faction, AStrategyBuilding* Wall);
	bool CanPlaceBuilding(EStrategyFaction Faction, EStrategyBuildingType BuildingType, const FVector& Location,
		const FRotator& Rotation = FRotator::ZeroRotator);
	EStrategyBuildingPlacementIssue GetBuildingPlacementIssue(EStrategyFaction Faction, EStrategyBuildingType BuildingType,
		const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator);
	bool IsLocationInTerritory(EStrategyFaction Faction, const FVector& Location, float FootprintRadius) const;
	bool IsFootprintInTerritory(EStrategyFaction Faction, const FVector& Location, const FVector2D& FootprintExtent, float YawDegrees) const;

	void RegisterSquad(AStrategySquad* Squad);
	void UnregisterSquad(AStrategySquad* Squad);
	void RegisterBuilding(AStrategyBuilding* Building);
	void UnregisterBuilding(AStrategyBuilding* Building);
	void RegisterControlPoint(AStrategyControlPoint* Point);
	void ChangeControlPointOwner(AStrategyControlPoint* Point, EStrategyFaction OldOwner, EStrategyFaction NewOwner);
	bool TryStartTownSpecialization(EStrategyFaction Faction, AStrategyControlPoint* Town,
		EStrategyTownSpecialization Specialization);
	bool TryStartTownDowngrade(EStrategyFaction Faction, AStrategyControlPoint* Town);
	void NotifyTownDevelopmentChanged(AStrategyControlPoint* Town);
	void RecalculateFactionEconomy();
	bool IsTownSupplyConnected(const AStrategyControlPoint* Town) const;
	float GetTrainingTimeMultiplierAt(EStrategyFaction Faction, const FVector& Location) const;
	void SetFogOfWar(AStrategyFogOfWar* InFogOfWar) { FogOfWar = InFogOfWar; }

	const TArray<TObjectPtr<AStrategySquad>>& GetSquads() const { return Squads; }
	const TArray<TObjectPtr<AStrategyBuilding>>& GetBuildings() const { return Buildings; }
	const TArray<TObjectPtr<AStrategyControlPoint>>& GetControlPoints() const { return ControlPoints; }
	AStrategyControlPoint* FindCapital(EStrategyFaction Faction) const;
	bool IsVisibleToFaction(EStrategyFaction Faction, const FVector& Location) const;
	bool IsExploredToFaction(EStrategyFaction Faction, const FVector& Location) const;
	UTexture2D* GetPlayerFogTexture() const;

	void NotifyCapitalDestroyed(EStrategyFaction DestroyedFaction);
	EStrategyFaction GetWinner() const { return Winner; }
	bool IsMatchRunning() const { return Winner == EStrategyFaction::Neutral; }
	FStrategyFactionNotificationEvent& OnFactionNotification() { return FactionNotification; }
	void NotifyFaction(EStrategyFaction Faction, const FString& Message);

private:
	void EnsureDefinitionsLoaded();
	FStrategyFactionNotificationEvent FactionNotification;

	UPROPERTY()
	TMap<EStrategyFaction, FStrategyFactionState> Factions;

	UPROPERTY()
	TMap<EStrategyUnitType, TObjectPtr<UStrategyUnitDataAsset>> UnitDefinitions;

	UPROPERTY()
	TMap<EStrategyBuildingType, TObjectPtr<UStrategyBuildingDataAsset>> BuildingDefinitions;

	UPROPERTY()
	TObjectPtr<UStrategyPresentationDataAsset> PresentationDefinition;

	UPROPERTY()
	TArray<TObjectPtr<AStrategySquad>> Squads;

	UPROPERTY()
	TArray<TObjectPtr<AStrategyBuilding>> Buildings;

	UPROPERTY()
	TArray<TObjectPtr<AStrategyControlPoint>> ControlPoints;

	UPROPERTY()
	TObjectPtr<AStrategyFogOfWar> FogOfWar;

	UPROPERTY(VisibleAnywhere)
	EStrategyFaction Winner = EStrategyFaction::Neutral;
};
