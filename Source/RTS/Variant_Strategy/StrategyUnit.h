// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "StrategyTypes.h"
#include "StrategyUnit.generated.h"

class AStrategySquad;
class USphereComponent;
class USkeletalMeshComponent;
class UStaticMeshComponent;
class UEnvQuery;
class UEnvQueryInstanceBlueprintWrapper;

/** Delegate to report that this unit has finished moving */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUnitMoveCompletedDelegate, AStrategyUnit*, Unit);

/**
 *  A simple strategy game unit
 *  Rather than react to inputs, it's controlled indirectly by the Strategy Player Controller
 */
UCLASS()
class AStrategyUnit : public ACharacter, public IStrategyDamageable
{
	GENERATED_BODY()

private:

	/** Interaction range sphere */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* InteractionRange;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* RiderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* SelectionRing;

protected:

	/** Cast reference to the AI Controlling this unit */
	TObjectPtr<AAIController> AIController;

public:

	/** Constructor */
	AStrategyUnit();
	virtual void Tick(float DeltaSeconds) override;
	void Initialize(AStrategySquad* InSquad, EStrategyFaction InFaction, const UStrategyUnitDataAsset* InDefinition);
	void IssueOrder(const FStrategyOrder& Order);
	AStrategySquad* GetSquad() const { return Squad; }
	float GetCurrentHealth() const { return FMath::Max(0.0f, Health); }
	float RestoreHealth(float Amount);
	void SetGarrisoned(bool bInGarrison);
	bool IsGarrisoned() const { return bGarrisoned; }

	virtual EStrategyFaction GetStrategyFaction() const override { return Faction; }
	virtual bool IsStrategyAlive() const override { return Health > 0.0f && !bGarrisoned; }
	virtual bool IsStrategyUnit() const override { return true; }
	virtual EStrategyUnitType GetStrategyUnitType() const override { return UnitType; }
	virtual void ReceiveStrategyDamage(float Damage, EStrategyUnitType AttackerType, EStrategyFaction SourceFaction) override;

protected:

	virtual void NotifyControllerChanged() override;

public:

	/** Stops unit movement immediately */
	void StopMoving();

	/** Notifies this unit that it was selected */
	void UnitSelected();

	/** Notifies this unit that it was deselected */
	void UnitDeselected();

	/** Notifies this unit that it's been interacted with by another actor */
	void Interact(AStrategyUnit* Interactor);

	/** Attempts to move this unit to the passed location, and optionally signals it to interact on arrival */
	void MoveToLocation(const FVector& Location, bool bInteract, const TArray<AStrategyUnit*> IgnoreList);

	/** Returns the last cached movement goal location */
	FVector GetMovementGoal() const;

protected:

	/** Called by EQS when the movement destination query has finished */
	UFUNCTION()
	void OnEQSFinished(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus);

	/** Called by the AI controller when this unit has finished moving */
	void OnMoveFinished(FAIRequestID RequestID, const FPathFollowingResult& Result);

	/** Wraps up movement logic */
	void HandleMoveFinished();
	void FindNearestTarget();
	void UpdateCombat(float DeltaSeconds);
	void UpdatePresentation(float DeltaSeconds);
	void PlayVisualState(EStrategyUnitVisualState NewState);
	void PlayAttackFeedback();
	void PlayHitFeedback();
	void BeginDeathPresentation();

protected:

	/** Blueprint handler for strategy game selection */
	UFUNCTION(BlueprintImplementableEvent, Category="NPC", meta = (DisplayName="Unit Selected"))
	void BP_UnitSelected();

	/** Blueprint handler for strategy game deselection */
	UFUNCTION(BlueprintImplementableEvent, Category="NPC", meta = (DisplayName="Unit Deselected"))
	void BP_UnitDeselected();

	/** Blueprint handler to stop the unit's interaction animation */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category="NPC", meta = (DisplayName="Stop Animation"))
	void BP_StopAnimation();

	/** Blueprint handler for strategy game interactions */
	UFUNCTION(BlueprintImplementableEvent, Category="NPC", meta = (DisplayName="Interaction Behavior"))
	void BP_InteractionBehavior(AStrategyUnit* Interactor);

protected:

	/** EnvQuery to use when this unit interacts after movement */
	UPROPERTY(EditAnywhere, Category="NPC")
	TObjectPtr<UEnvQuery> InteractionQuery;

	/** EnvQuery to use when this unit does not interact after movement */
	UPROPERTY(EditAnywhere, Category="NPC")
	TObjectPtr<UEnvQuery> NoInteractionQuery;

	/** How close we should get to the movement goal to consider ourselves as having reached it */
	UPROPERTY(EditAnywhere, Category="NPC", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float MovementAcceptanceRadius = 100.0f;

	/** Max distance to look for nearby units when doing an interaction check */
	UPROPERTY(EditAnywhere, Category="Input", meta = (ClampMin = 0, ClampMax = 10000, Units = "cm"))
	float InteractionRadius = 250.0f;

	/** EQS instance running the movement query for this unit */
	TObjectPtr<UEnvQueryInstanceBlueprintWrapper> EnvQueryInstance;

	/** Cached movement goal for this unit */
	FVector CurrentMovementGoal;

	/** If true, this unit will attempt to interact with a nearby unit upon finishing movement */
	bool bInteractOnArrival = false;

	/** List of actors to ignore when searching for units to interact with */
	TArray<AStrategyUnit*> InteractIgnoreList;

	UPROPERTY()
	TObjectPtr<AStrategySquad> Squad;

	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget;

	TObjectPtr<const UStrategyUnitDataAsset> Definition;

	EStrategyFaction Faction = EStrategyFaction::Neutral;
	EStrategyUnitType UnitType = EStrategyUnitType::Infantry;
	FStrategyOrder CurrentOrder;
	FStrategyOrder ResumeOrderAfterBlocker;
	float Health = 1.0f;
	float Damage = 1.0f;
	float AttackInterval = 1.0f;
	float AttackRange = 180.0f;
	float AttackCooldown = 0.0f;
	float TargetSearchCooldown = 0.0f;
	float AttackVisualLockRemaining = 0.0f;
	float HitFlashRemaining = 0.0f;
	EStrategyUnitVisualState CurrentVisualState = EStrategyUnitVisualState::Dead;
	bool bResumeMoveAfterBlocker = false;
	bool bDeathPresentationStarted = false;
	bool bGarrisoned = false;

public:

	FOnUnitMoveCompletedDelegate OnMoveCompleted;
};
