// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "StrategyGameState.h"
#include "StrategyMapDefinition.h"
#include "StrategyMapTerrain.h"
#include "StrategyPawn.h"
#include "StrategyUnit.h"
#include "StrategyWorldActors.h"

AStrategyGameMode::AStrategyGameMode()
{
	GameStateClass = AStrategyGameState::StaticClass();
}

void AStrategyGameMode::BeginPlay()
{
	Super::BeginPlay();

	TArray<AActor*> TemplateUnits;
	UGameplayStatics::GetAllActorsOfClass(this, AStrategyUnit::StaticClass(), TemplateUnits);
	for (AActor* Unit : TemplateUnits)
	{
		Unit->Destroy();
	}

	AStrategyGameState* State = GetGameState<AStrategyGameState>();
	check(State);
	const FStrategySkirmishMapDefinition MapDefinition = FStrategyMapDefinitions::Resolve(GetWorld()->GetMapName());
	auto SpawnPoint = [this](const FVector& Location, EStrategyFaction Faction, bool bCapital)
	{
		AStrategyControlPoint* Point = GetWorld()->SpawnActor<AStrategyControlPoint>(AStrategyControlPoint::StaticClass(), Location, FRotator::ZeroRotator);
		check(Point);
		Point->Initialize(Faction, bCapital);
	};

	SpawnPoint(MapDefinition.PlayerCapital, EStrategyFaction::Player, true);
	SpawnPoint(MapDefinition.EnemyCapital, EStrategyFaction::Enemy, true);
	for (const FVector& TownLocation : MapDefinition.NeutralTowns)
	{
		SpawnPoint(TownLocation, EStrategyFaction::Neutral, false);
	}

	for (const TPair<EStrategyFaction, FVector> Start : {
		TPair<EStrategyFaction, FVector>(EStrategyFaction::Player, MapDefinition.PlayerSquadStart),
		TPair<EStrategyFaction, FVector>(EStrategyFaction::Enemy, MapDefinition.EnemySquadStart)})
	{
		const UStrategyUnitDataAsset* Infantry = State->GetUnitDefinition(EStrategyUnitType::Infantry);
		check(State->TrySpendAndReserve(Start.Key, 0.0f, Infantry->PopulationCost));
		State->SpawnSquad(Start.Key, EStrategyUnitType::Infantry, Start.Value, true);
	}

	GetWorld()->SpawnActor<AStrategyAICommander>();
	GetWorld()->SpawnActor<AStrategyFogOfWar>();
	AStrategyMapTerrain* Terrain = GetWorld()->SpawnActor<AStrategyMapTerrain>();
	check(Terrain);
	Terrain->InitializePresentation(MapDefinition);

}

