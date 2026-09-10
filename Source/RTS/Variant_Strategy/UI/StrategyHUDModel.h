#pragma once

#include "CoreMinimal.h"
#include "StrategyTypes.h"

enum class EStrategyHUDContext : uint8
{
	Idle,
	Squad,
	Building,
	Town,
	Build
};

enum class EStrategyHUDUnavailableReason : uint8
{
	None,
	UnderConstruction,
	WrongBuilding,
	QueueFull,
	NotEnoughGold,
	PopulationFull
};

struct FStrategyHUDContextInputs
{
	bool bBuildMode = false;
	bool bHasTown = false;
	bool bHasBuilding = false;
	int32 SelectedSquadCount = 0;
};

struct FStrategyHUDLayoutRules
{
	static EStrategyHUDContext ResolveContext(const FStrategyHUDContextInputs& Inputs);
	static float ResolveScale(float ViewportHeight);
	static uint8 ResolveAffordabilityState(float Gold, float Cost);
};

struct FStrategyHUDVisibility
{
	bool bObjectPanel = false;
	bool bDetailPanel = false;
	bool bCommandPanel = false;
	bool bIdleHelp = false;

	static FStrategyHUDVisibility ForContext(EStrategyHUDContext Context);
};

struct FStrategyHUDActionRules
{
	static EStrategyHUDUnavailableReason GetTrainingUnavailableReason(bool bConstructionComplete,
		bool bTrainable, int32 QueueLength, float Gold, int32 UsedAndReservedPopulation,
		int32 PopulationCap, float GoldCost, int32 PopulationCost);
	static FString GetUnavailableReasonText(EStrategyHUDUnavailableReason Reason);
};

struct FStrategyPlacementIssueRules
{
	static EStrategyBuildingPlacementIssue Resolve(bool bMapAllowed, bool bInsideTerritory,
		bool bNavigable, bool bOverlapping);
	static FString GetIssueText(EStrategyBuildingPlacementIssue Issue);
};

struct FStrategyHUDNotification
{
	FString Message;
	FLinearColor Color = FLinearColor::White;
	float RemainingSeconds = 3.0f;
};

struct FStrategyHUDNotificationQueue
{
	void Push(const FString& Message, const FLinearColor& Color);
	void Update(float DeltaSeconds);
	const TArray<FStrategyHUDNotification>& GetItems() const { return Items; }

private:
	TArray<FStrategyHUDNotification> Items;
};

struct FStrategyHUDUpdateCache
{
	bool AcceptText(FName Key, const FString& Value);
	bool AcceptScalar(FName Key, float Value);
	void Reset();

private:
	TMap<FName, FString> TextValues;
	TMap<FName, float> ScalarValues;
};
