#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategySystems.h"
#include "StrategyTypes.h"
#include "StrategyWorldActors.generated.h"

class AStrategyGameState;
class AStrategyUnit;
class UBoxComponent;
class UMaterialInstanceDynamic;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTexture2D;

UCLASS()
class AStrategySquad : public AActor
{
	GENERATED_BODY()

public:
	AStrategySquad();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void Initialize(EStrategyFaction InFaction, const UStrategyUnitDataAsset* InDefinition, bool bPopulationReserved);
	void IssueOrder(const FStrategyOrder& Order);
	void SetSelected(bool bSelected);
	void NotifyMemberDied(AStrategyUnit* Member);

	EStrategyFaction GetFaction() const { return Faction; }
	EStrategyUnitType GetUnitType() const;
	int32 GetPopulationCost() const;
	bool IsAlive() const { return !Members.IsEmpty(); }
	FVector GetCenterLocation() const;
	float GetHealthPercent() const;
	FVector GetMarkerWorldLocation() const { return GetCenterLocation() + FVector(0.0f, 0.0f, 260.0f); }
	const TArray<TObjectPtr<AStrategyUnit>>& GetMembers() const { return Members; }

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY()
	TArray<TObjectPtr<AStrategyUnit>> Members;

	UPROPERTY()
	TObjectPtr<const UStrategyUnitDataAsset> Definition;

	UPROPERTY()
	EStrategyFaction Faction = EStrategyFaction::Neutral;

	float InitialTotalHealth = 0.0f;
	bool bPopulationReleased = false;
};

UCLASS()
class AStrategyBuilding : public AActor, public IStrategyDamageable
{
	GENERATED_BODY()

public:
	AStrategyBuilding();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void Initialize(EStrategyFaction InFaction, const UStrategyBuildingDataAsset* InDefinition);
	bool QueueUnit(EStrategyUnitType UnitType);
	void SetSelected(bool bSelected);

	EStrategyBuildingType GetBuildingType() const;
	bool IsConstructionComplete() const { return bConstructionComplete; }
	int32 GetQueueLength() const { return TrainingQueue.Num(); }
	float GetHealthPercent() const;

	virtual EStrategyFaction GetStrategyFaction() const override { return Faction; }
	virtual bool IsStrategyAlive() const override { return Health > 0.0f; }
	virtual bool IsStrategyUnit() const override { return false; }
	virtual EStrategyUnitType GetStrategyUnitType() const override { return EStrategyUnitType::Infantry; }
	virtual void ReceiveStrategyDamage(float Damage, EStrategyUnitType AttackerType, EStrategyFaction SourceFaction) override;

private:
	void CompleteConstruction();
	void UpdateTower(float DeltaSeconds);
	void ApplyCleanup();
	void UpdateAppearance();
	void UpdateAppearanceScale(float HeightAlpha);
	void UpdateCollision();

	UPROPERTY()
	TObjectPtr<UBoxComponent> Collision;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> GateLeftPost;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> GateRightPost;

	UPROPERTY()
	TObjectPtr<const UStrategyBuildingDataAsset> Definition;

	UPROPERTY()
	EStrategyFaction Faction = EStrategyFaction::Neutral;

	FStrategyTrainingQueue TrainingQueue;
	float Health = 0.0f;
	float ConstructionElapsed = 0.0f;
	float AttackCooldown = 0.0f;
	bool bConstructionComplete = false;
	bool bCleanupApplied = false;
};

UCLASS()
class AStrategyControlPoint : public AActor, public IStrategyDamageable
{
	GENERATED_BODY()

public:
	AStrategyControlPoint();
	virtual void Tick(float DeltaSeconds) override;

	void Initialize(EStrategyFaction InFaction, bool bInCapital);
	bool IsCapital() const { return bCapital; }
	float GetTerritoryRadius() const { return TerritoryRadius; }
	float GetIncomePerSecond() const { return IncomePerSecond; }
	int32 GetPopulationBonus() const { return PopulationBonus; }
	float GetCaptureProgress() const { return CaptureState.ProgressSeconds / 10.0f; }

	virtual EStrategyFaction GetStrategyFaction() const override { return CaptureState.Owner; }
	virtual bool IsStrategyAlive() const override { return bCapital && Health > 0.0f; }
	virtual bool IsStrategyUnit() const override { return false; }
	virtual EStrategyUnitType GetStrategyUnitType() const override { return EStrategyUnitType::Infantry; }
	virtual void ReceiveStrategyDamage(float Damage, EStrategyUnitType AttackerType, EStrategyFaction SourceFaction) override;

private:
	void UpdateAppearance();

	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY()
	TObjectPtr<USphereComponent> CaptureArea;

	FStrategyCaptureState CaptureState;
	float TerritoryRadius = 2500.0f;
	float IncomePerSecond = 3.0f;
	int32 PopulationBonus = 5;
	float Health = 0.0f;
	bool bCapital = false;
};

UCLASS()
class AStrategyFogOfWar : public AActor
{
	GENERATED_BODY()

public:
	AStrategyFogOfWar();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	bool IsVisibleToFaction(EStrategyFaction Faction, const FVector& Location) const;

private:
	void UpdateFog();
	void RevealFaction(FStrategyFogGrid& Grid, EStrategyFaction Faction);
	void UpdateActorVisibility();
	void UpdateTexture();

	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> FogPlane;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> FogTexture;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FogMaterial;

	FStrategyFogGrid PlayerGrid;
	FStrategyFogGrid EnemyGrid;
	float UpdateAccumulator = 0.0f;
};

UCLASS()
class AStrategyAICommander : public AActor
{
	GENERATED_BODY()

public:
	AStrategyAICommander();
	virtual void Tick(float DeltaSeconds) override;

private:
	void RunDecision();
	AStrategyControlPoint* FindThreatenedPoint() const;
	AStrategyControlPoint* FindNeutralPoint() const;
	EStrategyBuildingType FindMissingProductionBuilding() const;
	bool TryBuild(EStrategyBuildingType BuildingType);
	void TrainCounterUnit();
	void IssueAllSquads(const FStrategyOrder& Order, int32 MaximumSquads = MAX_int32);
	FVector GetNextBuildLocation() const;

	float DecisionAccumulator = 0.0f;
	mutable int32 BuildIndex = 0;
};
