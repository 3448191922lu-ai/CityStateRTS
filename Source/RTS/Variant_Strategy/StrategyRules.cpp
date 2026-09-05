#include "StrategyRules.h"

void FStrategyFactionState::AddIncome(float DeltaSeconds)
{
	Gold += IncomePerSecond * DeltaSeconds;
}

bool FStrategyFactionState::CanAfford(float Cost, int32 PopulationCost) const
{
	return Gold >= Cost && UsedPopulation + ReservedPopulation + PopulationCost <= FMath::Min(PopulationCap, 60);
}

bool FStrategyFactionState::TrySpendAndReserve(float Cost, int32 PopulationCost)
{
	if (!CanAfford(Cost, PopulationCost))
	{
		return false;
	}

	Gold -= Cost;
	ReservedPopulation += PopulationCost;
	return true;
}

void FStrategyFactionState::CommitPopulation(int32 PopulationCost)
{
	ReservedPopulation = FMath::Max(0, ReservedPopulation - PopulationCost);
	UsedPopulation += PopulationCost;
}

void FStrategyFactionState::ReleaseReservedPopulation(int32 PopulationCost)
{
	ReservedPopulation = FMath::Max(0, ReservedPopulation - PopulationCost);
}

void FStrategyFactionState::RemovePopulation(int32 PopulationCost)
{
	UsedPopulation = FMath::Max(0, UsedPopulation - PopulationCost);
}

void FStrategyCaptureState::Update(float DeltaSeconds, bool bPlayerPresent, bool bEnemyPresent)
{
	if (bPlayerPresent && bEnemyPresent)
	{
		return;
	}

	EStrategyFaction NewChallenger = EStrategyFaction::Neutral;
	if (bPlayerPresent && Owner != EStrategyFaction::Player)
	{
		NewChallenger = EStrategyFaction::Player;
	}
	else if (bEnemyPresent && Owner != EStrategyFaction::Enemy)
	{
		NewChallenger = EStrategyFaction::Enemy;
	}

	if (NewChallenger != EStrategyFaction::Neutral)
	{
		if (Challenger != NewChallenger)
		{
			Challenger = NewChallenger;
			ProgressSeconds = 0.0f;
		}

		ProgressSeconds += DeltaSeconds;
		if (ProgressSeconds >= 10.0f)
		{
			Owner = Challenger;
			Challenger = EStrategyFaction::Neutral;
			ProgressSeconds = 0.0f;
		}
		return;
	}

	ProgressSeconds = FMath::Max(0.0f, ProgressSeconds - DeltaSeconds * 2.0f);
	if (ProgressSeconds <= 0.0f)
	{
		Challenger = EStrategyFaction::Neutral;
	}
}

float FStrategyRules::GetDamageMultiplier(EStrategyUnitType Attacker, EStrategyUnitType Defender)
{
	const bool bCounters =
		(Attacker == EStrategyUnitType::Infantry && Defender == EStrategyUnitType::Cavalry) ||
		(Attacker == EStrategyUnitType::Cavalry && Defender == EStrategyUnitType::Archer) ||
		(Attacker == EStrategyUnitType::Archer && Defender == EStrategyUnitType::Infantry);
	return bCounters ? 1.5f : 1.0f;
}

EStrategyFaction FStrategyRules::GetWinner(bool bPlayerCapitalAlive, bool bEnemyCapitalAlive)
{
	if (bPlayerCapitalAlive == bEnemyCapitalAlive)
	{
		return EStrategyFaction::Neutral;
	}
	return bPlayerCapitalAlive ? EStrategyFaction::Player : EStrategyFaction::Enemy;
}

bool FStrategyRules::IsInsideTerritory(const FVector& Location, float FootprintRadius, const FVector& PointLocation, float TerritoryRadius)
{
	return FVector::DistSquared2D(Location, PointLocation) <= FMath::Square(FMath::Max(0.0f, TerritoryRadius - FootprintRadius));
}

bool FStrategyRules::IsFootprintInsideTerritory(const FVector& Location, const FVector2D& FootprintExtent, float YawDegrees, const FVector& PointLocation, float TerritoryRadius)
{
	const FQuat Rotation(FVector::UpVector, FMath::DegreesToRadians(YawDegrees));
	for (const FVector2D Corner : {
		FVector2D(-FootprintExtent.X, -FootprintExtent.Y), FVector2D(-FootprintExtent.X, FootprintExtent.Y),
		FVector2D(FootprintExtent.X, -FootprintExtent.Y), FVector2D(FootprintExtent.X, FootprintExtent.Y)})
	{
		const FVector WorldCorner = Location + Rotation.RotateVector(FVector(Corner.X, Corner.Y, 0.0f));
		if (FVector::DistSquared2D(WorldCorner, PointLocation) > FMath::Square(TerritoryRadius))
		{
			return false;
		}
	}
	return true;
}
