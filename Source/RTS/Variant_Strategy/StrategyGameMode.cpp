// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "StrategyGameState.h"
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
	auto SpawnPoint = [this](const FVector& Location, EStrategyFaction Faction, bool bCapital)
	{
		AStrategyControlPoint* Point = GetWorld()->SpawnActor<AStrategyControlPoint>(AStrategyControlPoint::StaticClass(), Location, FRotator::ZeroRotator);
		check(Point);
		Point->Initialize(Faction, bCapital);
	};

	SpawnPoint(FVector(-7000.0f, 0.0f, 0.0f), EStrategyFaction::Player, true);
	SpawnPoint(FVector(7000.0f, 0.0f, 0.0f), EStrategyFaction::Enemy, true);
	SpawnPoint(FVector(0.0f, 0.0f, 0.0f), EStrategyFaction::Neutral, false);
	SpawnPoint(FVector(0.0f, 4000.0f, 0.0f), EStrategyFaction::Neutral, false);
	SpawnPoint(FVector(0.0f, -4000.0f, 0.0f), EStrategyFaction::Neutral, false);

	for (const TPair<EStrategyFaction, FVector> Start : {
		TPair<EStrategyFaction, FVector>(EStrategyFaction::Player, FVector(-5600.0f, 0.0f, 100.0f)),
		TPair<EStrategyFaction, FVector>(EStrategyFaction::Enemy, FVector(5600.0f, 0.0f, 100.0f))})
	{
		const UStrategyUnitDataAsset* Infantry = State->GetUnitDefinition(EStrategyUnitType::Infantry);
		check(State->TrySpendAndReserve(Start.Key, 0.0f, Infantry->PopulationCost));
		State->SpawnSquad(Start.Key, EStrategyUnitType::Infantry, Start.Value, true);
	}

	GetWorld()->SpawnActor<AStrategyAICommander>();
	GetWorld()->SpawnActor<AStrategyFogOfWar>();

}

