#pragma once

#include "CoreMinimal.h"
#include "StrategyTypes.h"

enum class EStrategyTownUpdateResult : uint8
{
	None,
	BuildCompleted,
	DowngradeCompleted,
	ReactivationCompleted
};

struct FStrategyTownDevelopment
{
	EStrategyTownSpecialization Specialization = EStrategyTownSpecialization::None;
	EStrategyTownDevelopmentState State = EStrategyTownDevelopmentState::Unspecialized;
	float ProgressSeconds = 0.0f;
};

struct FStrategySupplyNode
{
	FVector2D Location = FVector2D::ZeroVector;
	EStrategyFaction Owner = EStrategyFaction::Neutral;
	bool bCapital = false;
};

struct FStrategyTownDevelopmentRules
{
	static constexpr float SpecializationCost = 250.0f;
	static constexpr float BuildDuration = 20.0f;
	static constexpr float DowngradeDuration = 10.0f;
	static constexpr float ReactivationDuration = 10.0f;
	static constexpr float ReactivationRecoveryDuration = 5.0f;

	static bool CanStartSpecialization(const FStrategyTownDevelopment& Town,
		EStrategyTownSpecialization Specialization, float Gold);
	static bool CanStartDowngrade(const FStrategyTownDevelopment& Town);
	static void StartSpecialization(FStrategyTownDevelopment& Town, EStrategyTownSpecialization Specialization);
	static void StartDowngrade(FStrategyTownDevelopment& Town);
	static void HandleOwnershipChanged(FStrategyTownDevelopment& Town);
	static EStrategyTownUpdateResult Update(FStrategyTownDevelopment& Town, float DeltaSeconds,
		bool bContested, bool bOwnerSquadPresent);
	static float GetDowngradeRefund();
};

struct FStrategySupplyRules
{
	static constexpr float LinkDistance = 7500.0f;
	static TSet<int32> FindConnectedTownIndices(const TArray<FStrategySupplyNode>& Nodes,
		EStrategyFaction Faction);
};

struct FStrategyTownSpecializationRules
{
	static float GetIncomeBonus(EStrategyTownSpecialization Specialization,
		EStrategyTownDevelopmentState State, bool bConnected);
	static int32 GetPopulationBonus(EStrategyTownSpecialization Specialization,
		EStrategyTownDevelopmentState State);
	static float GetTrainingTimeMultiplier(EStrategyTownSpecialization Specialization,
		EStrategyTownDevelopmentState State, bool bConnected);
	static float GetCaptureDuration(EStrategyTownSpecialization Specialization,
		EStrategyTownDevelopmentState State);
	static float GetFortressRange(EStrategyTownSpecialization Specialization,
		EStrategyTownDevelopmentState State, bool bConnected);
	static float GetFortressDamage(EStrategyTownSpecialization Specialization,
		EStrategyTownDevelopmentState State, bool bConnected);
};

struct FStrategyGarrisonDestination
{
	FVector2D Location = FVector2D::ZeroVector;
	int32 AvailableSlots = 0;
	bool bActiveFortress = false;
	bool bCapital = false;
};

struct FStrategyGarrisonRules
{
	static constexpr float ExitDistance = 650.0f;
	static constexpr float AIRetreatHealth = 0.60f;
	static constexpr float AILeaveHealth = 0.90f;

	static int32 GetCapacity(bool bCapital, EStrategyTownSpecialization Specialization,
		EStrategyTownDevelopmentState State);
	static float GetRecoveryDelay(EStrategyTownSpecialization Specialization,
		EStrategyTownDevelopmentState State);
	static float GetRecoveryRate(EStrategyTownSpecialization Specialization,
		EStrategyTownDevelopmentState State);
	static float GetReinforcementInterval(EStrategyTownSpecialization Specialization,
		EStrategyTownDevelopmentState State);
	static bool ShouldExitForDestination(float Distance);
	static bool ShouldAIRetreat(float HealthPercent);
	static bool ShouldAILeave(float HealthPercent);
	static float GetRecoveryAmount(float InitialTotalHealth, float RecoveryRate, float DeltaSeconds);
	static float ClampRecoveredHealth(float CurrentHealth, float MaxHealth, float RecoveryAmount);
	static bool ShouldReinforce(int32 CurrentMembers, int32 InitialMembers,
		float ElapsedSeconds, float IntervalSeconds);
	static bool CanEnter(EStrategyFaction SquadFaction, EStrategyFaction Owner,
		bool bSquadAlive, bool bAlreadyGarrisoned, int32 CurrentCount, int32 Capacity);
	static bool ShouldSortie(EStrategyFaction Owner, bool bEnemyPresent,
		bool bOwnerPresent, int32 GarrisonCount);
	static int32 FindBestDestination(const FVector2D& Origin,
		const TArray<FStrategyGarrisonDestination>& Destinations);
};

struct FStrategyGarrisonCommandRules
{
	static bool ShouldEnterPoint(EStrategyFaction SquadFaction, EStrategyFaction PointOwner,
		bool bTargetIsControlPoint);
	static bool ShouldExitForOrder(EStrategyOrderType OrderType, float Distance);
};

struct FStrategyTownContribution
{
	EStrategyFaction Owner = EStrategyFaction::Neutral;
	float IncomePerSecond = 0.0f;
	int32 PopulationCap = 0;
};

struct FStrategyFactionEconomyTotals
{
	float IncomePerSecond = 0.0f;
	int32 PopulationCap = 0;
	int32 OwnedPoints = 0;
};

struct FStrategyFactionEconomyRules
{
	static FStrategyFactionEconomyTotals Calculate(EStrategyFaction Faction,
		const TArray<FStrategyTownContribution>& Contributions, int32 CompletedBuildingPopulation);
};

struct FStrategyTownActionRules
{
	static bool CanChooseSpecialization(EStrategyFaction Viewer, EStrategyFaction Owner,
		bool bCapital, EStrategyTownDevelopmentState State);
	static bool CanDowngrade(EStrategyFaction Viewer, EStrategyFaction Owner,
		bool bCapital, EStrategyTownDevelopmentState State);
};

struct FStrategyTownVisibilityRules
{
	static bool CanShowLiveDetails(EStrategyFaction Viewer, EStrategyFaction Owner,
		bool bCurrentlyVisible);
	static bool CanShowPublicDetails(EStrategyFaction Viewer, EStrategyFaction Owner,
		bool bCurrentlyVisible);
	static bool CanShowSupplyConnection(EStrategyFaction Viewer, EStrategyFaction Owner);
	static EStrategyTownSpecialization GetPublicSpecialization(EStrategyFaction Viewer,
		EStrategyFaction Owner, bool bCurrentlyVisible, EStrategyTownSpecialization Specialization,
		EStrategyTownDevelopmentState State);
};

struct FStrategyTownPanelLayoutRules
{
	static bool ShouldUseDesiredSize();
};

struct FStrategyTrainingItem
{
	EStrategyUnitType UnitType = EStrategyUnitType::Infantry;
	float RemainingTime = 0.0f;
	float TotalTime = 0.0f;
	int32 PopulationCost = 0;
};

struct FStrategyTrainingQueue
{
	bool Enqueue(EStrategyUnitType UnitType, float TrainingTime, int32 PopulationCost);
	bool Update(float DeltaSeconds, EStrategyUnitType& OutCompletedType, int32& OutPopulationCost);
	int32 GetReservedPopulation() const;
	int32 Num() const { return Items.Num(); }
	const TArray<FStrategyTrainingItem>& GetItems() const { return Items; }
	float GetFrontProgress() const;
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

struct FStrategyTownAIInputs
{
	bool bHasTradeTown = false;
	bool bHasRecruitmentTown = false;
	bool bTownThreatened = false;
};

struct FStrategyTownAIPlanner
{
	static EStrategyTownSpecialization ChooseSpecialization(const FStrategyTownAIInputs& Inputs);
	static bool ShouldRespecialize(EStrategyTownSpecialization Current,
		EStrategyTownSpecialization Preferred, float Gold);
};

struct FStrategyCameraMovement
{
	static FVector2D ResolveScreenDirection(bool bUp, bool bDown, bool bLeft, bool bRight);
	static FVector ScreenToWorld(const FVector2D& ScreenDirection, float CameraYaw = -45.0f);
	static FVector GetOrbitCameraLocation(const FVector& CameraLocation, float CameraPitch,
		float CurrentYaw, float DesiredYaw);
};

struct FStrategyCameraBoundsRules
{
	static FVector2D GetGroundViewExtents(float OrthoWidth, float AspectRatio, float PitchDegrees, float YawDegrees);
	static FVector2D ClampGroundFocus(const FVector2D& DesiredFocus, const FVector2D& MapMin, const FVector2D& MapMax,
		const FVector2D& ViewExtents, float Padding);
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

struct FStrategyBuildingPlacementRules
{
	static FVector ResolveGroundLocation(const FVector& RequestedLocation, const FVector& ProjectedGround);
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

struct FStrategyIntroPromptRules
{
	static float GetOpacity(float ElapsedSeconds);
};

struct FStrategyOrderTargetRules
{
	static bool CanAttack(EStrategyFaction SourceFaction, EStrategyFaction TargetFaction,
		bool bTargetAlive, bool bTargetVisible);
};

struct FStrategyPresentationRules
{
	static EStrategyUnitVisualState ResolveUnitVisualState(bool bAlive, bool bAttacking, float SpeedSquared);
	static bool CanPlayWorldFeedback(EStrategyFaction SourceFaction, bool bVisibleToPlayer);
	static FRotator GetImportedCharacterMeshRotation();
};
