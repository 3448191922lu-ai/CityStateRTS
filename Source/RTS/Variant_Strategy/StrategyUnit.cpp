// Copyright Epic Games, Inc. All Rights Reserved.


#include "StrategyUnit.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/SphereComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryInstanceBlueprintWrapper.h"
#include "Engine/OverlapResult.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "StrategyGameState.h"
#include "StrategyRules.h"
#include "StrategySystems.h"
#include "StrategyWorldActors.h"
#include "StrategyArtStyle.h"

AStrategyUnit::AStrategyUnit()
{
	PrimaryActorTick.bCanEverTick = true;

	// ensure this unit has a valid AI controller to handle move requests
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// create the interaction range sphere
	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("Interaction Range"));
	InteractionRange->SetupAttachment(RootComponent);

	InteractionRange->SetSphereRadius(100.0f);
	InteractionRange->SetCollisionProfileName(FName("OverlapAllDynamic"));

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body Mesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetHiddenInGame(true);

	SelectionRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Selection Ring"));
	SelectionRing->SetupAttachment(GetCapsuleComponent());
	SelectionRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SelectionRing->SetHiddenInGame(true);
	SelectionRing->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
	SelectionRing->SetRelativeLocation(FVector(0.0f, 0.0f, -85.0f));
	SelectionRing->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.03f));

	// configure movement
	GetCharacterMovement()->GravityScale = 1.5f;
	GetCharacterMovement()->MaxAcceleration = 1000.0f;
	GetCharacterMovement()->BrakingFrictionFactor = 1.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1000.0f;
	GetCharacterMovement()->PerchRadiusThreshold = 20.0f;
	GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 150.0f;
	GetCharacterMovement()->AvoidanceWeight = 1.0f;
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;
	GetCharacterMovement()->SetFixedBrakingDistance(200.0f);
	GetCharacterMovement()->SetFixedBrakingDistance(true);
}

void AStrategyUnit::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateCombat(DeltaSeconds);
}

void AStrategyUnit::Initialize(AStrategySquad* InSquad, EStrategyFaction InFaction, const UStrategyUnitDataAsset* InDefinition)
{
	Squad = InSquad;
	Faction = InFaction;
	UnitType = InDefinition->UnitType;
	Health = InDefinition->MaxHealth;
	Damage = InDefinition->Damage;
	AttackInterval = InDefinition->AttackInterval;
	AttackRange = InDefinition->AttackRange;
	GetCharacterMovement()->MaxWalkSpeed = InDefinition->MoveSpeed;
	GetCapsuleComponent()->SetCollisionObjectType(Faction == EStrategyFaction::Player
		? ECC_GameTraceChannel1
		: ECC_GameTraceChannel2);

	const TCHAR* MeshPath = UnitType == EStrategyUnitType::Infantry
		? TEXT("/Engine/BasicShapes/Cylinder.Cylinder")
		: UnitType == EStrategyUnitType::Archer
			? TEXT("/Engine/BasicShapes/Cone.Cone")
			: TEXT("/Engine/BasicShapes/Cube.Cube");
	UStaticMesh* UnitMesh = InDefinition->VisualMesh.Get();
	if (!UnitMesh)
	{
		UnitMesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
	}
	BodyMesh->SetStaticMesh(UnitMesh);
	BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -25.0f));
	BodyMesh->SetRelativeScale3D(StrategyArtStyle::GetUnitSilhouetteScale(UnitType) * InDefinition->VisualScale);

	UMaterialInterface* BaseMaterial = InDefinition->VisualMaterial.Get();
	if (!BaseMaterial)
	{
		BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	Material->SetVectorParameterValue(TEXT("Color"), StrategyArtStyle::GetFactionColor(Faction));
	BodyMesh->SetMaterial(0, Material);

	UMaterialInstanceDynamic* RingMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	RingMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.05f, 1.0f, 0.1f));
	SelectionRing->SetMaterial(0, RingMaterial);
}

void AStrategyUnit::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	// validate and save a copy of the AI controller reference
	AIController = Cast<AAIController>(Controller);
	
	if (AIController)
	{
		// subscribe to the move finished handler on the path following component
		UPathFollowingComponent* PFComp = AIController->GetPathFollowingComponent();
		if (PFComp)
		{
			PFComp->OnRequestFinished.AddUObject(this, &AStrategyUnit::OnMoveFinished);
		}
	}
}

void AStrategyUnit::StopMoving()
{
	// use the character movement component to stop movement
	GetCharacterMovement()->StopMovementImmediately();
	if (AIController)
	{
		AIController->StopMovement();
	}

	// stop the unit's interaction animation
	BP_StopAnimation();
}

void AStrategyUnit::UnitSelected()
{
	SelectionRing->SetHiddenInGame(false);
	// pass control to BP
	BP_UnitSelected();
}

void AStrategyUnit::UnitDeselected()
{
	SelectionRing->SetHiddenInGame(true);
	// pass control to BP
	BP_UnitDeselected();
}

void AStrategyUnit::Interact(AStrategyUnit* Interactor)
{
	// ensure the interactor is valid
	if (IsValid(Interactor))
	{
		// rotate towards the actor we're interacting with
		SetActorRotation(UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), Interactor->GetActorLocation()));

		// signal the interactor to play its interaction behavior
		Interactor->BP_InteractionBehavior(this);

		// play our own interaction behavior
		BP_InteractionBehavior(Interactor);
	}
	
}

void AStrategyUnit::MoveToLocation(const FVector& Location, bool bInteract, const TArray<AStrategyUnit*> IgnoreList)
{
	// cache the movement and interaction parameters
	CurrentMovementGoal = Location;
	bInteractOnArrival = bInteract;
	InteractIgnoreList = IgnoreList;

	// stop movement and animation
	StopMoving();

	// choose the EnvQuery to use
	UEnvQuery* MoveQuery = bInteractOnArrival ? InteractionQuery : NoInteractionQuery;
	if (!MoveQuery)
	{
		if (AIController)
		{
			AIController->MoveToLocation(CurrentMovementGoal, MovementAcceptanceRadius, true, true, true, false, nullptr, true);
		}
		return;
	}

	// choose the run mode to use. The main interacting unit gets the closest result, all others choose randomly from top 25%
	TEnumAsByte<EEnvQueryRunMode::Type> RunMode = bInteractOnArrival ? EEnvQueryRunMode::SingleResult : EEnvQueryRunMode::RandomBest25Pct;

	// run an EQS to resolve the movement destination using the NavMesh
	EnvQueryInstance = UEnvQueryManager::RunEQSQuery(this, MoveQuery, this,  RunMode, UEnvQueryInstanceBlueprintWrapper::StaticClass());

	if (IsValid(EnvQueryInstance))
	{
		EnvQueryInstance->GetOnQueryFinishedEvent().AddDynamic(this, &AStrategyUnit::OnEQSFinished);
	}
}

FVector AStrategyUnit::GetMovementGoal() const
{
	return CurrentMovementGoal;
}

void AStrategyUnit::IssueOrder(const FStrategyOrder& Order)
{
	bResumeMoveAfterBlocker = false;
	CurrentOrder = Order;
	CurrentTarget = Order.TargetActor;
	if (Order.Type == EStrategyOrderType::Stop)
	{
		CurrentTarget = nullptr;
		StopMoving();
	}
	else if (Order.Type == EStrategyOrderType::Move || Order.Type == EStrategyOrderType::AttackMove)
	{
		if (Order.Type == EStrategyOrderType::Move)
		{
			CurrentTarget = nullptr;
		}
		MoveToLocation(Order.Destination, false, {});
	}
}

void AStrategyUnit::ReceiveStrategyDamage(float InDamage, EStrategyUnitType AttackerType, EStrategyFaction SourceFaction)
{
	if (SourceFaction == Faction || Health <= 0.0f)
	{
		return;
	}
	Health -= InDamage * FStrategyRules::GetDamageMultiplier(AttackerType, UnitType);
	if (Health <= 0.0f)
	{
		if (Squad)
		{
			Squad->NotifyMemberDied(this);
		}
		Destroy();
	}
}

void AStrategyUnit::FindNearestTarget()
{
	if (CurrentOrder.Type == EStrategyOrderType::Move)
	{
		return;
	}

	AActor* BestTarget = nullptr;
	float BestDistance = FMath::Square(1800.0f);
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Candidate = *It;
		IStrategyDamageable* Damageable = Cast<IStrategyDamageable>(Candidate);
		if (!Damageable || Candidate == this || Damageable->GetStrategyFaction() == Faction || Damageable->GetStrategyFaction() == EStrategyFaction::Neutral || !Damageable->IsStrategyAlive())
		{
			continue;
		}
		if (!State->IsVisibleToFaction(Faction, Candidate->GetActorLocation()))
		{
			continue;
		}
		const float Distance = FVector::DistSquared2D(GetActorLocation(), Candidate->GetActorLocation());
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			BestTarget = Candidate;
		}
	}
	CurrentTarget = BestTarget;
}

void AStrategyUnit::UpdateCombat(float DeltaSeconds)
{
	AttackCooldown = FMath::Max(0.0f, AttackCooldown - DeltaSeconds);
	TargetSearchCooldown -= DeltaSeconds;
	IStrategyDamageable* Damageable = Cast<IStrategyDamageable>(CurrentTarget);
	if (!CurrentTarget || !Damageable || !Damageable->IsStrategyAlive() || Damageable->GetStrategyFaction() == Faction)
	{
		CurrentTarget = nullptr;
		if (bResumeMoveAfterBlocker)
		{
			bResumeMoveAfterBlocker = false;
			IssueOrder(ResumeOrderAfterBlocker);
			return;
		}
		if (TargetSearchCooldown <= 0.0f)
		{
			TargetSearchCooldown = 0.5f;
			FindNearestTarget();
		}
		Damageable = Cast<IStrategyDamageable>(CurrentTarget);
	}

	if (!CurrentTarget || !Damageable)
	{
		return;
	}
	const float Distance = FMath::Sqrt(CurrentTarget->GetComponentsBoundingBox().ComputeSquaredDistanceToPoint(GetActorLocation()));
	if (Distance > AttackRange)
	{
		if (TargetSearchCooldown <= 0.0f)
		{
			TargetSearchCooldown = 0.35f;
			MoveToLocation(CurrentTarget->GetActorLocation(), false, {});
		}
		return;
	}

	StopMoving();
	SetActorRotation(UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), CurrentTarget->GetActorLocation()));
	if (AttackCooldown <= 0.0f)
	{
		Damageable->ReceiveStrategyDamage(Damage, UnitType, Faction);
		AttackCooldown = AttackInterval;
	}
}

void AStrategyUnit::OnEQSFinished(UEnvQueryInstanceBlueprintWrapper* QueryInstance, EEnvQueryStatus::Type QueryStatus)
{
	// was the EnvQuery successful?
	if (QueryInstance)
	{
		// get the query result locations
		TArray<FVector> ResultLocations;

		if(QueryInstance->GetQueryResultsAsLocations(ResultLocations))
		{
			// grab the top result
			CurrentMovementGoal = ResultLocations[0];

			// ensure we have a valid AI Controller
			if (AIController)
			{
				// set up the AI Move Request
				FAIMoveRequest MoveReq;

				MoveReq.SetGoalLocation(CurrentMovementGoal);
				MoveReq.SetAcceptanceRadius(MovementAcceptanceRadius);
				MoveReq.SetAllowPartialPath(true);
				MoveReq.SetUsePathfinding(true);
				MoveReq.SetProjectGoalLocation(true);
				MoveReq.SetRequireNavigableEndLocation(true);
				MoveReq.SetNavigationFilter(AIController->GetDefaultNavigationFilterClass());
				MoveReq.SetCanStrafe(false);

				// request a move to the AI Controller
				FNavPathSharedPtr FollowedPath;
				const FPathFollowingRequestResult ResultData = AIController->MoveTo(MoveReq, &FollowedPath);
		
				// check if we're already at the goal
				if(ResultData.Code == EPathFollowingRequestResult::AlreadyAtGoal)
				{
					// finish movement immediately
					HandleMoveFinished();
				}
			}
		}
	}
}

void AStrategyUnit::OnMoveFinished(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	if (!Result.IsSuccess())
	{
		FHitResult Hit;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);
		const FVector TraceOffset(0.0f, 0.0f, 40.0f);
		if (GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation() + TraceOffset, CurrentMovementGoal + TraceOffset,
			ECC_Visibility, QueryParams))
		{
			if (AStrategyBuilding* Building = Cast<AStrategyBuilding>(Hit.GetActor());
				Building && FStrategyGateCollisionRules::CanAttackBlockingBuilding(
					Faction, Building->GetStrategyFaction(), Building->GetBuildingType()))
			{
				ResumeOrderAfterBlocker = CurrentOrder;
				bResumeMoveAfterBlocker = true;
				CurrentOrder.Type = EStrategyOrderType::AttackTarget;
				CurrentTarget = Building;
				return;
			}
		}
	}
	HandleMoveFinished();
}

void AStrategyUnit::HandleMoveFinished()
{
	// broadcast the move completed delegate
	OnMoveCompleted.Broadcast(this);

	if (bInteractOnArrival)
	{
		// do an overlap test to find nearby interactive objects
		TArray<FOverlapResult> OutOverlaps;

		FCollisionShape CollisionSphere;
		CollisionSphere.SetSphere(InteractionRadius);

		FCollisionObjectQueryParams ObjectParams;
		ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

		FCollisionQueryParams QueryParams;

		// add the selected units to the ignored list
		QueryParams.AddIgnoredActor(this);

		for (const AActor* Current : InteractIgnoreList)
		{
			QueryParams.AddIgnoredActor(Current);
		}

		if (GetWorld()->OverlapMultiByObjectType(OutOverlaps, GetActorLocation(), FQuat::Identity, ObjectParams, CollisionSphere, QueryParams))
		{
			// find the first unit we've overlapped, and interact with it
			for (const FOverlapResult& CurrentOverlap : OutOverlaps)
			{
				if (AStrategyUnit* CurrentUnit = Cast<AStrategyUnit>(CurrentOverlap.GetActor()))
				{
					CurrentUnit->Interact(this);
					return;
				}
			}
		}
	}
}
