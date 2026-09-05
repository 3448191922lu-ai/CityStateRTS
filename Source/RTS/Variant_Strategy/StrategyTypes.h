#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UObject/Interface.h"
#include "StrategyTypes.generated.h"

class AStrategyUnit;
class UMaterialInterface;
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
enum class EStrategyOrderType : uint8
{
	Move,
	AttackMove,
	AttackTarget,
	Stop
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

	void Update(float DeltaSeconds, bool bPlayerPresent, bool bEnemyPresent);
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
};
