#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/Interface.h"
#include "StrategyTypes.generated.h"

class AStrategyUnit;
class UAnimationAsset;
class UMaterialInterface;
class UNiagaraSystem;
class USkeletalMesh;
class USoundAttenuation;
class USoundBase;
class USoundConcurrency;
class UStaticMesh;

UENUM(BlueprintType)
enum class EStrategyFaction : uint8
{
	Neutral,
	Player,
	Enemy
};

UENUM(BlueprintType)
enum class EStrategyUnitType : uint8
{
	Infantry,
	Archer,
	Cavalry
};

UENUM(BlueprintType)
enum class EStrategyBuildingType : uint8
{
	Barracks,
	ArcheryRange,
	Stable,
	House,
	Tower,
	Wall,
	Gate,
	Capital
};

UENUM(BlueprintType)
enum class EStrategyBuildingPlacementIssue : uint8
{
	None,
	MapRestricted,
	OutsideTerritory,
	NotNavigable,
	Overlap
};

UENUM(BlueprintType)
enum class EStrategyTownSpecialization : uint8
{
	None,
	Trade,
	Recruitment,
	Fortress
};

UENUM(BlueprintType)
enum class EStrategyTownDevelopmentState : uint8
{
	Unspecialized,
	Building,
	Active,
	Downgrading,
	DisabledAfterCapture
};

UENUM(BlueprintType)
enum class EStrategyOrderType : uint8
{
	Move,
	AttackMove,
	AttackTarget,
	Stop
};

UENUM(BlueprintType)
enum class EStrategyUnitVisualState : uint8
{
	Idle,
	Move,
	Attack,
	Dead
};

UINTERFACE()
class UStrategyDamageable : public UInterface
{
	GENERATED_BODY()
};

class IStrategyDamageable
{
	GENERATED_BODY()

public:
	virtual EStrategyFaction GetStrategyFaction() const = 0;
	virtual bool IsStrategyAlive() const = 0;
	virtual bool IsStrategyUnit() const = 0;
	virtual EStrategyUnitType GetStrategyUnitType() const = 0;
	virtual void ReceiveStrategyDamage(float Damage, EStrategyUnitType AttackerType, EStrategyFaction SourceFaction) = 0;
};

USTRUCT(BlueprintType)
struct FStrategyOrder
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EStrategyOrderType Type = EStrategyOrderType::Move;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Destination = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AActor> TargetActor;
};

USTRUCT(BlueprintType)
struct FStrategyFactionState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float Gold = 500.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 UsedPopulation = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 ReservedPopulation = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 PopulationCap = 20;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 OwnedPoints = 1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float IncomePerSecond = 5.0f;

	void AddIncome(float DeltaSeconds);
	bool CanAfford(float Cost, int32 PopulationCost = 0) const;
	bool TrySpendAndReserve(float Cost, int32 PopulationCost);
	void CommitPopulation(int32 PopulationCost);
	void ReleaseReservedPopulation(int32 PopulationCost);
	void RemovePopulation(int32 PopulationCost);
};

USTRUCT(BlueprintType)
struct FStrategyCaptureState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EStrategyFaction Owner = EStrategyFaction::Neutral;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EStrategyFaction Challenger = EStrategyFaction::Neutral;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	float ProgressSeconds = 0.0f;

	void Update(float DeltaSeconds, bool bPlayerPresent, bool bEnemyPresent, float CaptureDurationSeconds = 10.0f);
};

UCLASS(BlueprintType)
class UStrategyUnitDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EStrategyUnitType UnitType = EStrategyUnitType::Infantry;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float GoldCost = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float TrainingTime = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 PopulationCost = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 MemberCount = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxHealth = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Damage = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float AttackInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float AttackRange = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MoveSpeed = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AStrategyUnit> UnitClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<UStaticMesh> VisualMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<UMaterialInterface> VisualMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	FVector VisualScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	FName FactionMaterialSlot = TEXT("Faction");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<UAnimationAsset> IdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<UAnimationAsset> MoveAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<UAnimationAsset> AttackAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<UAnimationAsset> DeathAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<USkeletalMesh> RiderSkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	FName RiderFactionMaterialSlot = TEXT("Faction");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<UAnimationAsset> RiderIdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<UAnimationAsset> RiderMoveAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<UAnimationAsset> RiderAttackAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<UAnimationAsset> RiderDeathAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<UStaticMesh> ProjectileMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<USoundBase> AttackSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	float AttackVisualDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	float DeathVisualDuration = 0.8f;
};

UCLASS(BlueprintType)
class UStrategyBuildingDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EStrategyBuildingType BuildingType = EStrategyBuildingType::Barracks;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float GoldCost = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ConstructionTime = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float MaxHealth = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float FootprintRadius = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector2D FootprintExtent = FVector2D(250.0f, 250.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 PopulationBonus = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<EStrategyUnitType> TrainableUnits;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AActor> BuildingClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<UStaticMesh> VisualMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	TObjectPtr<UMaterialInterface> VisualMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Visual")
	FVector VisualScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	FName FactionMaterialSlot = TEXT("Faction");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<UStaticMesh> ProjectileMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<USoundBase> AttackSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Presentation")
	TObjectPtr<USoundBase> DestroyedSound;
};

UCLASS(BlueprintType)
class UStrategyPresentationDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> PlayerFactionMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> EnemyFactionMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> NeutralFactionMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> ConstructionMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> HitFlashMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> CaptureRingMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> CapitalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> TownMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> FlagMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> TreeMeshA;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> TreeMeshB;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> RockMeshA;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> BridgeMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> MoveCommandEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> AttackMoveCommandEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> AttackTargetEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> HitEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> ConstructionEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> ConstructionCompleteEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> DestructionEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> CaptureEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> ProjectileTrailEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> SelectSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> MoveSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> AttackOrderSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> InvalidSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> ConstructionStartSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> ConstructionCompleteSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> TrainingCompleteSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> CaptureContestedSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> CaptureCompleteSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> MeleeHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> ArrowShotSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> ArrowHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> HoofSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> TowerShotSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> BuildingHitSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> BuildingDestroyedSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> VictorySound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundBase> DefeatSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundAttenuation> WorldAttenuation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<USoundConcurrency> CombatConcurrency;
};
