#include "StrategyWorldActors.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "StrategyGameState.h"
#include "StrategyMapDefinition.h"
#include "StrategyPresentationActors.h"
#include "StrategyRules.h"
#include "StrategyUnit.h"
#include "StrategyArtStyle.h"

namespace StrategyVisuals
{
	static UMaterialInterface* GetFactionMaterial(const UStrategyPresentationDataAsset* Presentation, EStrategyFaction Faction)
	{
		return Faction == EStrategyFaction::Player ? Presentation->PlayerFactionMaterial.Get()
			: Faction == EStrategyFaction::Enemy ? Presentation->EnemyFactionMaterial.Get()
			: Presentation->NeutralFactionMaterial.Get();
	}

	static void ApplyFactionMaterial(UStaticMeshComponent* Mesh, FName Slot, EStrategyFaction Faction,
		const UStrategyPresentationDataAsset* Presentation)
	{
		const int32 MaterialIndex = Slot.IsNone() ? 0 : Mesh->GetMaterialIndex(Slot);
		Mesh->SetMaterial(MaterialIndex, GetFactionMaterial(Presentation, Faction));
	}
}

AStrategySquad::AStrategySquad()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;
}

void AStrategySquad::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GarrisonPoint)
	{
		SetActorLocation(GarrisonPoint->GetActorLocation());
		return;
	}
	if (!Members.IsEmpty())
	{
		SetActorLocation(GetCenterLocation());
	}
	if (RequestedGarrisonPoint && FVector::DistSquared2D(GetCenterLocation(), RequestedGarrisonPoint->GetActorLocation())
		<= FMath::Square(FStrategyGarrisonRules::ExitDistance))
	{
		AStrategyControlPoint* Point = RequestedGarrisonPoint;
		RequestedGarrisonPoint = nullptr;
		if (!Point->TryGarrisonSquad(this))
		{
			GetWorld()->GetGameState<AStrategyGameState>()->NotifyFaction(Faction, TEXT("当前据点无法继续驻防"));
		}
	}
}

void AStrategySquad::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GarrisonPoint)
	{
		GarrisonPoint->RemoveGarrisonedSquad(this);
		GarrisonPoint = nullptr;
	}
	if (AStrategyGameState* State = GetWorld() ? GetWorld()->GetGameState<AStrategyGameState>() : nullptr)
	{
		State->UnregisterSquad(this);
	}
	Super::EndPlay(EndPlayReason);
}

void AStrategySquad::Initialize(EStrategyFaction InFaction, const UStrategyUnitDataAsset* InDefinition, bool bPopulationReserved)
{
	Faction = InFaction;
	Definition = InDefinition;
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	check(State && Definition);
	InitialTotalHealth = Definition->MaxHealth * Definition->MemberCount;
	State->RegisterSquad(this);
	if (bPopulationReserved)
	{
		State->CommitPopulation(Faction, Definition->PopulationCost);
	}

	for (int32 Index = 0; Index < Definition->MemberCount; ++Index)
	{
		const int32 Row = Index / 2;
		const int32 Column = Index % 2;
		SpawnMember(GetActorLocation() + FVector((Row - 0.5f) * 140.0f, (Column - 0.5f) * 140.0f, 100.0f));
	}
}

AStrategyUnit* AStrategySquad::SpawnMember(const FVector& Location)
{
	TSubclassOf<AStrategyUnit> UnitClass = Definition->UnitClass;
	if (!UnitClass)
	{
		UnitClass = AStrategyUnit::StaticClass();
	}
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AStrategyUnit* Unit = GetWorld()->SpawnActor<AStrategyUnit>(UnitClass, Location, FRotator::ZeroRotator, SpawnParameters);
	check(Unit);
	Unit->Initialize(this, Faction, Definition);
	Members.Add(Unit);
	return Unit;
}

void AStrategySquad::IssueOrder(const FStrategyOrder& Order)
{
	RequestedGarrisonPoint = nullptr;
	int32 Index = 0;
	for (AStrategyUnit* Unit : Members)
	{
		if (!IsValid(Unit))
		{
			continue;
		}
		FStrategyOrder MemberOrder = Order;
		if (Order.Type == EStrategyOrderType::Move || Order.Type == EStrategyOrderType::AttackMove)
		{
			const int32 Row = Index / 2;
			const int32 Column = Index % 2;
			MemberOrder.Destination += FVector((Row - 0.5f) * 160.0f, (Column - 0.5f) * 160.0f, 0.0f);
		}
		Unit->IssueOrder(MemberOrder);
		++Index;
	}
}

void AStrategySquad::RequestGarrison(AStrategyControlPoint* Point)
{
	if (!Point || GarrisonPoint == Point)
	{
		return;
	}
	if (GarrisonPoint)
	{
		const FVector Direction = (Point->GetActorLocation() - GarrisonPoint->GetActorLocation()).GetSafeNormal2D();
		ExitGarrison(GetActorLocation() + Direction * 760.0f);
	}
	FStrategyOrder Order;
	Order.Type = EStrategyOrderType::Move;
	Order.Destination = Point->GetActorLocation();
	IssueOrder(Order);
	RequestedGarrisonPoint = Point;
}

void AStrategySquad::SetSelected(bool bInSelected)
{
	bSelected = bInSelected;
	for (AStrategyUnit* Unit : Members)
	{
		if (IsValid(Unit))
		{
			bInSelected ? Unit->UnitSelected() : Unit->UnitDeselected();
		}
	}
}

void AStrategySquad::EnterGarrison(AStrategyControlPoint* Point)
{
	RequestedGarrisonPoint = nullptr;
	GarrisonPoint = Point;
	GarrisonElapsed = 0.0f;
	ReinforcementElapsed = 0.0f;
	SetActorLocation(Point->GetActorLocation());
	for (AStrategyUnit* Unit : Members)
	{
		Unit->SetActorLocation(Point->GetActorLocation());
		Unit->SetGarrisoned(true);
	}
}

void AStrategySquad::ExitGarrison(const FVector& ExitLocation)
{
	AStrategyControlPoint* PreviousPoint = GarrisonPoint;
	GarrisonPoint = nullptr;
	if (PreviousPoint)
	{
		PreviousPoint->RemoveGarrisonedSquad(this);
	}
	GarrisonElapsed = 0.0f;
	ReinforcementElapsed = 0.0f;
	SetActorLocation(ExitLocation);
	for (int32 Index = 0; Index < Members.Num(); ++Index)
	{
		AStrategyUnit* Unit = Members[Index];
		const int32 Row = Index / 2;
		const int32 Column = Index % 2;
		Unit->SetActorLocation(ExitLocation + FVector((Row - 0.5f) * 140.0f, (Column - 0.5f) * 140.0f, 100.0f));
		Unit->SetGarrisoned(false);
		if (bSelected)
		{
			Unit->UnitSelected();
		}
	}
}

void AStrategySquad::ApplyGarrisonRecovery(float DeltaSeconds, float RecoveryDelay,
	float RecoveryRate, float ReinforcementInterval)
{
	GarrisonElapsed += DeltaSeconds;
	if (GarrisonElapsed < RecoveryDelay)
	{
		return;
	}

	float RemainingRecovery = FStrategyGarrisonRules::GetRecoveryAmount(InitialTotalHealth, RecoveryRate, DeltaSeconds);
	TArray<AStrategyUnit*> SortedMembers;
	for (AStrategyUnit* Unit : Members)
	{
		SortedMembers.Add(Unit);
	}
	SortedMembers.Sort([](const AStrategyUnit& Left, const AStrategyUnit& Right)
	{
		return Left.GetCurrentHealth() < Right.GetCurrentHealth();
	});
	for (AStrategyUnit* Unit : SortedMembers)
	{
		RemainingRecovery -= Unit->RestoreHealth(RemainingRecovery);
		if (RemainingRecovery <= 0.0f)
		{
			break;
		}
	}

	ReinforcementElapsed += DeltaSeconds;
	if (FStrategyGarrisonRules::ShouldReinforce(Members.Num(), Definition->MemberCount,
		ReinforcementElapsed, ReinforcementInterval))
	{
		RestoreOneMember();
		ReinforcementElapsed -= ReinforcementInterval;
	}
}

bool AStrategySquad::RestoreOneMember()
{
	if (!GarrisonPoint || Members.Num() >= Definition->MemberCount)
	{
		return false;
	}
	AStrategyUnit* Unit = SpawnMember(GarrisonPoint->GetActorLocation());
	Unit->SetGarrisoned(true);
	return true;
}

void AStrategySquad::NotifyMemberDied(AStrategyUnit* Member)
{
	Members.Remove(Member);
	if (!Members.IsEmpty() || bPopulationReleased)
	{
		return;
	}

	bPopulationReleased = true;
	if (AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>())
	{
		State->RemovePopulation(Faction, GetPopulationCost());
		State->UnregisterSquad(this);
	}
	Destroy();
}

EStrategyUnitType AStrategySquad::GetUnitType() const
{
	return Definition ? Definition->UnitType : EStrategyUnitType::Infantry;
}

int32 AStrategySquad::GetPopulationCost() const
{
	return Definition ? Definition->PopulationCost : 0;
}

float AStrategySquad::GetHealthPercent() const
{
	float CurrentHealth = 0.0f;
	for (const AStrategyUnit* Unit : Members)
	{
		if (IsValid(Unit))
		{
			CurrentHealth += Unit->GetCurrentHealth();
		}
	}
	return FStrategySquadMarkerRules::CalculateHealthPercent(CurrentHealth, InitialTotalHealth);
}

FVector AStrategySquad::GetCenterLocation() const
{
	if (GarrisonPoint)
	{
		return GarrisonPoint->GetActorLocation();
	}
	FVector Total = FVector::ZeroVector;
	int32 Count = 0;
	for (const AStrategyUnit* Unit : Members)
	{
		if (IsValid(Unit))
		{
			Total += Unit->GetActorLocation();
			++Count;
		}
	}
	return Count > 0 ? Total / Count : GetActorLocation();
}

FVector AStrategySquad::GetMarkerWorldLocation() const
{
	if (GarrisonPoint)
	{
		return GarrisonPoint->GetGarrisonMarkerWorldLocation(this);
	}
	return GetCenterLocation() + FVector(0.0f, 0.0f, 260.0f);
}

AStrategyBuilding::AStrategyBuilding()
{
	PrimaryActorTick.bCanEverTick = true;
	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	RootComponent = Collision;
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GateLeftPost = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Gate Left Post"));
	GateLeftPost->SetupAttachment(RootComponent);
	GateLeftPost->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GateRightPost = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Gate Right Post"));
	GateRightPost->SetupAttachment(RootComponent);
	GateRightPost->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AStrategyBuilding::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HitFlashRemaining > 0.0f)
	{
		HitFlashRemaining = FMath::Max(0.0f, HitFlashRemaining - DeltaSeconds);
		if (HitFlashRemaining <= 0.0f)
		{
			Mesh->SetOverlayMaterial(bConstructionComplete ? nullptr : GetWorld()->GetGameState<AStrategyGameState>()->GetPresentationDefinition()->ConstructionMaterial);
		}
	}
	if (!bConstructionComplete)
	{
		ConstructionElapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp(ConstructionElapsed / Definition->ConstructionTime, 0.15f, 1.0f);
		UpdateAppearanceScale(Alpha);
		if (ConstructionElapsed >= Definition->ConstructionTime)
		{
			CompleteConstruction();
		}
		return;
	}

	if (GetBuildingType() == EStrategyBuildingType::Tower)
	{
		UpdateTower(DeltaSeconds);
	}

	EStrategyUnitType CompletedType;
	int32 CompletedPopulation = 0;
	if (TrainingQueue.Update(DeltaSeconds, CompletedType, CompletedPopulation))
	{
		AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
		State->SpawnSquad(Faction, CompletedType, GetActorLocation() + GetActorForwardVector() * 450.0f + FVector(0.0f, 0.0f, 100.0f), true);
		PlayWorldFeedback(nullptr, State->GetPresentationDefinition()->TrainingCompleteSound);
		const TCHAR* UnitName = CompletedType == EStrategyUnitType::Infantry ? TEXT("步兵")
			: CompletedType == EStrategyUnitType::Archer ? TEXT("弓兵") : TEXT("骑兵");
		State->NotifyFaction(Faction, FString::Printf(TEXT("训练完成：%s"), UnitName));
	}
}

void AStrategyBuilding::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ApplyCleanup();
	Super::EndPlay(EndPlayReason);
}

void AStrategyBuilding::Initialize(EStrategyFaction InFaction, const UStrategyBuildingDataAsset* InDefinition)
{
	Faction = InFaction;
	Definition = InDefinition;
	Health = Definition->MaxHealth;
	Collision->SetBoxExtent(FVector(Definition->FootprintExtent.X, Definition->FootprintExtent.Y, 250.0f));
	UpdateCollision();
	UpdateAppearance();
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	State->RegisterBuilding(this);
	Mesh->SetOverlayMaterial(State->GetPresentationDefinition()->ConstructionMaterial);
	PlayWorldFeedback(State->GetPresentationDefinition()->ConstructionEffect, State->GetPresentationDefinition()->ConstructionStartSound);
}

bool AStrategyBuilding::QueueUnit(EStrategyUnitType UnitType)
{
	if (!bConstructionComplete || !Definition->TrainableUnits.Contains(UnitType) || TrainingQueue.Num() >= 5)
	{
		return false;
	}

	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	const UStrategyUnitDataAsset* UnitDefinition = State->GetUnitDefinition(UnitType);
	if (!State->TrySpendAndReserve(Faction, UnitDefinition->GoldCost, UnitDefinition->PopulationCost))
	{
		return false;
	}
	return TrainingQueue.Enqueue(UnitType, UnitDefinition->TrainingTime
		* State->GetTrainingTimeMultiplierAt(Faction, GetActorLocation()), UnitDefinition->PopulationCost);
}

void AStrategyBuilding::SetSelected(bool bSelected)
{
	Mesh->SetRenderCustomDepth(bSelected);
	GateLeftPost->SetRenderCustomDepth(bSelected);
	GateRightPost->SetRenderCustomDepth(bSelected);
}

EStrategyBuildingType AStrategyBuilding::GetBuildingType() const
{
	return Definition ? Definition->BuildingType : EStrategyBuildingType::Barracks;
}

float AStrategyBuilding::GetHealthPercent() const
{
	return Definition ? Health / Definition->MaxHealth : 0.0f;
}

float AStrategyBuilding::GetConstructionProgress() const
{
	return bConstructionComplete ? 1.0f : FMath::Clamp(ConstructionElapsed / Definition->ConstructionTime, 0.0f, 1.0f);
}

void AStrategyBuilding::ReceiveStrategyDamage(float Damage, EStrategyUnitType AttackerType, EStrategyFaction SourceFaction)
{
	if (SourceFaction == Faction || !IsStrategyAlive())
	{
		return;
	}
	Health -= Damage;
	const UStrategyPresentationDataAsset* Presentation = GetWorld()->GetGameState<AStrategyGameState>()->GetPresentationDefinition();
	Mesh->SetOverlayMaterial(Presentation->HitFlashMaterial);
	HitFlashRemaining = 0.08f;
	PlayWorldFeedback(Presentation->HitEffect, Definition->HitSound);
	if (Health <= 0.0f)
	{
		PlayWorldFeedback(Presentation->DestructionEffect, Definition->DestroyedSound);
		ApplyCleanup();
		Destroy();
	}
}

void AStrategyBuilding::CompleteConstruction()
{
	bConstructionComplete = true;
	UpdateAppearanceScale(1.0f);
	Mesh->SetOverlayMaterial(nullptr);
	UpdateCollision();
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	State->RecalculateFactionEconomy();
	const UStrategyPresentationDataAsset* Presentation = State->GetPresentationDefinition();
	PlayWorldFeedback(Presentation->ConstructionCompleteEffect, Presentation->ConstructionCompleteSound);
	State->NotifyFaction(Faction, TEXT("建筑已完工"));
}

void AStrategyBuilding::UpdateCollision()
{
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	const bool bGate = GetBuildingType() == EStrategyBuildingType::Gate;
	Collision->SetCanEverAffectNavigation(!bGate);
	if (bGate && bConstructionComplete)
	{
		const ECollisionChannel FriendlyUnitChannel = Faction == EStrategyFaction::Player
			? ECC_GameTraceChannel1
			: ECC_GameTraceChannel2;
		Collision->SetCollisionResponseToChannel(FriendlyUnitChannel, ECR_Ignore);
	}
}

void AStrategyBuilding::UpdateTower(float DeltaSeconds)
{
	AttackCooldown -= DeltaSeconds;
	if (AttackCooldown > 0.0f)
	{
		return;
	}

	AStrategyUnit* BestTarget = nullptr;
	float BestDistance = 1500.0f;
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
	{
		AStrategyUnit* Unit = *It;
		const float Distance = FVector::Dist2D(GetActorLocation(), Unit->GetActorLocation());
		if (FStrategyTowerTargetRules::CanTarget(Faction, Unit->GetStrategyFaction(), bConstructionComplete, Unit->IsStrategyAlive(),
			State->IsVisibleToFaction(Faction, Unit->GetActorLocation()), Distance, BestDistance))
		{
			BestDistance = Distance;
			BestTarget = Unit;
		}
	}

	if (BestTarget)
	{
		const UStrategyPresentationDataAsset* Presentation = State->GetPresentationDefinition();
		if (FStrategyPresentationRules::CanPlayWorldFeedback(Faction,
			State->IsVisibleToFaction(EStrategyFaction::Player, GetActorLocation())))
		{
			AStrategyProjectileVisual* Projectile = GetWorld()->SpawnActor<AStrategyProjectileVisual>();
			Projectile->Initialize(Definition->ProjectileMesh, Presentation->ProjectileTrailEffect,
				GetActorLocation() + FVector(0.0f, 0.0f, 350.0f), BestTarget->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f), 0.3f);
			UGameplayStatics::PlaySoundAtLocation(this, Definition->AttackSound, GetActorLocation());
		}
		BestTarget->ReceiveStrategyDamage(25.0f, EStrategyUnitType::Archer, Faction);
		AttackCooldown = 1.2f;
	}
}

void AStrategyBuilding::ApplyCleanup()
{
	if (bCleanupApplied || !GetWorld())
	{
		return;
	}
	bCleanupApplied = true;
	if (AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>())
	{
		State->ReleaseReservedPopulation(Faction, TrainingQueue.GetReservedPopulation());
		State->UnregisterBuilding(this);
	}
}

void AStrategyBuilding::UpdateAppearance()
{
	Mesh->SetStaticMesh(Definition->VisualMesh);
	GateLeftPost->SetVisibility(false);
	GateRightPost->SetVisibility(false);
	UpdateAppearanceScale(1.0f);
	StrategyVisuals::ApplyFactionMaterial(Mesh, Definition->FactionMaterialSlot, Faction,
		GetWorld()->GetGameState<AStrategyGameState>()->GetPresentationDefinition());
}

void AStrategyBuilding::UpdateAppearanceScale(float HeightAlpha)
{
	const FVector FinalScale = Definition->VisualScale;
	Mesh->SetRelativeScale3D(FVector(FinalScale.X, FinalScale.Y, FinalScale.Z * HeightAlpha));
	Mesh->SetRelativeLocation(FVector::ZeroVector);
}

void AStrategyBuilding::PlayWorldFeedback(UNiagaraSystem* Effect, USoundBase* Sound) const
{
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	if (!FStrategyPresentationRules::CanPlayWorldFeedback(Faction,
		State->IsVisibleToFaction(EStrategyFaction::Player, GetActorLocation())))
	{
		return;
	}
	if (Effect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Effect, GetActorLocation());
	}
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation());
	}
}

AStrategyControlPoint::AStrategyControlPoint()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	FlagMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Flag Mesh"));
	FlagMesh->SetupAttachment(RootComponent);
	FlagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CaptureRing = CreateDefaultSubobject<UDecalComponent>(TEXT("Capture Ring"));
	CaptureRing->SetupAttachment(RootComponent);
	CaptureRing->SetRelativeLocation(FVector(0.0f, 0.0f, 12.0f));
	CaptureRing->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	CaptureRing->DecalSize = FVector(120.0f, 650.0f, 650.0f);
	CaptureArea = CreateDefaultSubobject<USphereComponent>(TEXT("CaptureArea"));
	CaptureArea->SetupAttachment(RootComponent);
	CaptureArea->SetSphereRadius(650.0f);
	CaptureArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AStrategyControlPoint::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	UpdateGarrison(DeltaSeconds);
	bool bPlayerPresent = false;
	bool bEnemyPresent = false;
	FVector NearestEnemyLocation = GetActorLocation();
	float NearestEnemyDistanceSquared = TNumericLimits<float>::Max();
	for (const AStrategySquad* Squad : State->GetSquads())
	{
		if (!IsValid(Squad) || Squad->IsGarrisoned())
		{
			continue;
		}
		const float DistanceSquared = FVector::DistSquared2D(GetActorLocation(), Squad->GetCenterLocation());
		if (DistanceSquared > FMath::Square(FStrategyGarrisonRules::ExitDistance))
		{
			continue;
		}
		bPlayerPresent |= Squad->GetFaction() == EStrategyFaction::Player;
		bEnemyPresent |= Squad->GetFaction() == EStrategyFaction::Enemy;
		if (Squad->GetFaction() != CaptureState.Owner && Squad->GetFaction() != EStrategyFaction::Neutral
			&& DistanceSquared < NearestEnemyDistanceSquared)
		{
			NearestEnemyDistanceSquared = DistanceSquared;
			NearestEnemyLocation = Squad->GetCenterLocation();
		}
	}

	const bool bEnemyToOwnerPresent = CaptureState.Owner == EStrategyFaction::Player
		? bEnemyPresent : CaptureState.Owner == EStrategyFaction::Enemy && bPlayerPresent;
	const bool bOwnerPresentBeforeSortie = CaptureState.Owner == EStrategyFaction::Player
		? bPlayerPresent : CaptureState.Owner == EStrategyFaction::Enemy && bEnemyPresent;
	const bool bSortieTriggered = FStrategyGarrisonRules::ShouldSortie(CaptureState.Owner,
		bEnemyToOwnerPresent, bOwnerPresentBeforeSortie, GarrisonedSquads.Num());
	if (bSortieTriggered)
	{
		SortieGarrison(NearestEnemyLocation);
		bPlayerPresent |= CaptureState.Owner == EStrategyFaction::Player;
		bEnemyPresent |= CaptureState.Owner == EStrategyFaction::Enemy;
	}
	if (bCapital)
	{
		return;
	}

	const EStrategyFaction OldOwner = CaptureState.Owner;
	const bool bContested = (bPlayerPresent && bEnemyPresent) || bSortieTriggered;
	CaptureState.Update(DeltaSeconds, bPlayerPresent, bEnemyPresent, GetRequiredCaptureDuration());
	CaptureRingMaterial->SetScalarParameterValue(TEXT("Progress"), GetCaptureProgress());
	CaptureRingMaterial->SetScalarParameterValue(TEXT("Contested"), bContested ? 1.0f : 0.0f);
	const UStrategyPresentationDataAsset* Presentation = State->GetPresentationDefinition();
	if (bContested && !bWasContested && FStrategyPresentationRules::CanPlayWorldFeedback(
		CaptureState.Owner, State->IsVisibleToFaction(EStrategyFaction::Player, GetActorLocation())))
	{
		UGameplayStatics::PlaySoundAtLocation(this, Presentation->CaptureContestedSound, GetActorLocation());
	}
	bWasContested = bContested;
	PreviousChallenger = CaptureState.Challenger;
	if (OldOwner != CaptureState.Owner)
	{
		FStrategyTownDevelopmentRules::HandleOwnershipChanged(TownDevelopment);
		State->ChangeControlPointOwner(this, OldOwner, CaptureState.Owner);
		UpdateAppearance();
		if (FStrategyPresentationRules::CanPlayWorldFeedback(CaptureState.Owner,
			State->IsVisibleToFaction(EStrategyFaction::Player, GetActorLocation())))
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Presentation->CaptureEffect, GetActorLocation());
			UGameplayStatics::PlaySoundAtLocation(this, Presentation->CaptureCompleteSound, GetActorLocation());
		}
	}

	const bool bOwnerSquadPresent = CaptureState.Owner == EStrategyFaction::Player ? bPlayerPresent : bEnemyPresent;
	const EStrategyTownUpdateResult DevelopmentResult = FStrategyTownDevelopmentRules::Update(
		TownDevelopment, DeltaSeconds, bContested, bOwnerSquadPresent);
	if (DevelopmentResult == EStrategyTownUpdateResult::DowngradeCompleted)
	{
		State->GetMutableFactionState(CaptureState.Owner).Gold += FStrategyTownDevelopmentRules::GetDowngradeRefund();
	}
	if (DevelopmentResult != EStrategyTownUpdateResult::None)
	{
		State->NotifyTownDevelopmentChanged(this);
		const FString Message = DevelopmentResult == EStrategyTownUpdateResult::BuildCompleted
			? TEXT("城镇专精建设完成")
			: DevelopmentResult == EStrategyTownUpdateResult::DowngradeCompleted
				? TEXT("城镇降级完成，返还 100 金币") : TEXT("城镇已重新启用");
		State->NotifyFaction(CaptureState.Owner, Message);
	}
	UpdateFortress(DeltaSeconds);
}

bool AStrategyControlPoint::TryGarrisonSquad(AStrategySquad* Squad)
{
	if (!Squad || !FStrategyGarrisonRules::CanEnter(Squad->GetFaction(), CaptureState.Owner,
		Squad->IsAlive(), Squad->IsGarrisoned(), GarrisonedSquads.Num(), GetGarrisonCapacity())
		|| FVector::DistSquared2D(GetActorLocation(), Squad->GetCenterLocation())
			> FMath::Square(FStrategyGarrisonRules::ExitDistance))
	{
		return false;
	}
	GarrisonedSquads.Add(Squad);
	Squad->EnterGarrison(this);
	return true;
}

void AStrategyControlPoint::RemoveGarrisonedSquad(AStrategySquad* Squad)
{
	GarrisonedSquads.Remove(Squad);
}

int32 AStrategyControlPoint::GetGarrisonCapacity() const
{
	return FStrategyGarrisonRules::GetCapacity(bCapital, TownDevelopment.Specialization, TownDevelopment.State);
}

FVector AStrategyControlPoint::GetGarrisonMarkerWorldLocation(const AStrategySquad* Squad) const
{
	const int32 Index = FMath::Max(0, GarrisonedSquads.IndexOfByKey(Squad));
	const float Offset = (Index - (GarrisonedSquads.Num() - 1) * 0.5f) * 180.0f;
	return GetActorLocation() + FVector(0.0f, Offset, bCapital ? 650.0f : 480.0f);
}

void AStrategyControlPoint::SortieGarrison(const FVector& EnemyLocation)
{
	const FVector Direction = (EnemyLocation - GetActorLocation()).GetSafeNormal2D().IsNearlyZero()
		? FVector::ForwardVector : (EnemyLocation - GetActorLocation()).GetSafeNormal2D();
	const FVector Side = FVector(-Direction.Y, Direction.X, 0.0f);
	const TArray<TObjectPtr<AStrategySquad>> SquadsToSortie = GarrisonedSquads;
	GarrisonedSquads.Reset();
	for (int32 Index = 0; Index < SquadsToSortie.Num(); ++Index)
	{
		AStrategySquad* Squad = SquadsToSortie[Index];
		if (!IsValid(Squad))
		{
			continue;
		}
		const FVector ExitLocation = GetActorLocation() + Direction * 760.0f
			+ Side * ((Index - (SquadsToSortie.Num() - 1) * 0.5f) * 260.0f);
		Squad->ExitGarrison(ExitLocation);
		FStrategyOrder Order;
		Order.Type = EStrategyOrderType::AttackMove;
		Order.Destination = EnemyLocation;
		Squad->IssueOrder(Order);
	}
}

void AStrategyControlPoint::UpdateGarrison(float DeltaSeconds)
{
	GarrisonedSquads.RemoveAll([](const AStrategySquad* Squad)
	{
		return !IsValid(Squad) || !Squad->IsAlive();
	});
	const float Delay = FStrategyGarrisonRules::GetRecoveryDelay(TownDevelopment.Specialization, TownDevelopment.State);
	const float Rate = FStrategyGarrisonRules::GetRecoveryRate(TownDevelopment.Specialization, TownDevelopment.State);
	const float Interval = FStrategyGarrisonRules::GetReinforcementInterval(TownDevelopment.Specialization, TownDevelopment.State);
	for (AStrategySquad* Squad : GarrisonedSquads)
	{
		Squad->ApplyGarrisonRecovery(DeltaSeconds, Delay, Rate, Interval);
	}
}

bool AStrategyControlPoint::StartSpecialization(EStrategyTownSpecialization Specialization)
{
	if (!FStrategyTownDevelopmentRules::CanStartSpecialization(TownDevelopment, Specialization,
		FStrategyTownDevelopmentRules::SpecializationCost))
	{
		return false;
	}
	FStrategyTownDevelopmentRules::StartSpecialization(TownDevelopment, Specialization);
	GetWorld()->GetGameState<AStrategyGameState>()->NotifyTownDevelopmentChanged(this);
	return true;
}

bool AStrategyControlPoint::StartDowngrade()
{
	if (!FStrategyTownDevelopmentRules::CanStartDowngrade(TownDevelopment))
	{
		return false;
	}
	FStrategyTownDevelopmentRules::StartDowngrade(TownDevelopment);
	GetWorld()->GetGameState<AStrategyGameState>()->NotifyTownDevelopmentChanged(this);
	return true;
}

float AStrategyControlPoint::GetDevelopmentProgress() const
{
	if (TownDevelopment.State == EStrategyTownDevelopmentState::Building)
	{
		return TownDevelopment.ProgressSeconds / FStrategyTownDevelopmentRules::BuildDuration;
	}
	if (TownDevelopment.State == EStrategyTownDevelopmentState::Downgrading)
	{
		return TownDevelopment.ProgressSeconds / FStrategyTownDevelopmentRules::DowngradeDuration;
	}
	if (TownDevelopment.State == EStrategyTownDevelopmentState::DisabledAfterCapture)
	{
		return TownDevelopment.ProgressSeconds / FStrategyTownDevelopmentRules::ReactivationDuration;
	}
	return TownDevelopment.State == EStrategyTownDevelopmentState::Active ? 1.0f : 0.0f;
}

float AStrategyControlPoint::GetRequiredCaptureDuration() const
{
	return FStrategyTownSpecializationRules::GetCaptureDuration(TownDevelopment.Specialization, TownDevelopment.State);
}

void AStrategyControlPoint::UpdateFortress(float DeltaSeconds)
{
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	const bool bConnected = State->IsTownSupplyConnected(this);
	const float AttackRange = FStrategyTownSpecializationRules::GetFortressRange(
		TownDevelopment.Specialization, TownDevelopment.State, bConnected);
	if (AttackRange <= 0.0f)
	{
		return;
	}

	FortressAttackCooldown -= DeltaSeconds;
	if (FortressAttackCooldown > 0.0f)
	{
		return;
	}

	AStrategyUnit* BestTarget = nullptr;
	float BestDistance = AttackRange;
	for (TActorIterator<AStrategyUnit> It(GetWorld()); It; ++It)
	{
		AStrategyUnit* Unit = *It;
		const float Distance = FVector::Dist2D(GetActorLocation(), Unit->GetActorLocation());
		if (FStrategyTowerTargetRules::CanTarget(CaptureState.Owner, Unit->GetStrategyFaction(), true,
			Unit->IsStrategyAlive(), State->IsVisibleToFaction(CaptureState.Owner, Unit->GetActorLocation()),
			Distance, BestDistance))
		{
			BestDistance = Distance;
			BestTarget = Unit;
		}
	}

	if (BestTarget)
	{
		const UStrategyBuildingDataAsset* TowerDefinition = State->GetBuildingDefinition(EStrategyBuildingType::Tower);
		const UStrategyPresentationDataAsset* Presentation = State->GetPresentationDefinition();
		if (FStrategyPresentationRules::CanPlayWorldFeedback(CaptureState.Owner,
			State->IsVisibleToFaction(EStrategyFaction::Player, GetActorLocation())))
		{
			AStrategyProjectileVisual* Projectile = GetWorld()->SpawnActor<AStrategyProjectileVisual>();
			Projectile->Initialize(TowerDefinition->ProjectileMesh, Presentation->ProjectileTrailEffect,
				GetActorLocation() + FVector(0.0f, 0.0f, 350.0f), BestTarget->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f), 0.3f);
			UGameplayStatics::PlaySoundAtLocation(this, TowerDefinition->AttackSound, GetActorLocation());
		}
		BestTarget->ReceiveStrategyDamage(FStrategyTownSpecializationRules::GetFortressDamage(
			TownDevelopment.Specialization, TownDevelopment.State, bConnected), EStrategyUnitType::Archer, CaptureState.Owner);
		FortressAttackCooldown = 1.2f;
	}
}

void AStrategyControlPoint::Initialize(EStrategyFaction InFaction, bool bInCapital)
{
	bCapital = bInCapital;
	CaptureState.Owner = InFaction;
	TerritoryRadius = bCapital ? 4000.0f : 2500.0f;
	IncomePerSecond = bCapital ? 5.0f : 3.0f;
	PopulationBonus = bCapital ? 20 : 5;
	Health = bCapital ? 3000.0f : 1.0f;
	const UStrategyPresentationDataAsset* Presentation = GetWorld()->GetGameState<AStrategyGameState>()->GetPresentationDefinition();
	Mesh->SetStaticMesh(bCapital ? Presentation->CapitalMesh : Presentation->TownMesh);
	Mesh->SetRelativeScale3D(FVector::OneVector);
	Mesh->SetRelativeLocation(FVector::ZeroVector);
	FlagMesh->SetStaticMesh(Presentation->FlagMesh);
	FlagMesh->SetRelativeLocation(FVector(0.0f, 0.0f, bCapital ? 520.0f : 360.0f));
	CaptureRing->SetHiddenInGame(bCapital);
	CaptureRingMaterial = UMaterialInstanceDynamic::Create(Presentation->CaptureRingMaterial, this);
	CaptureRing->SetDecalMaterial(CaptureRingMaterial);
	UpdateAppearance();
	GetWorld()->GetGameState<AStrategyGameState>()->RegisterControlPoint(this);
}

void AStrategyControlPoint::ReceiveStrategyDamage(float Damage, EStrategyUnitType AttackerType, EStrategyFaction SourceFaction)
{
	if (!bCapital || SourceFaction == CaptureState.Owner || Health <= 0.0f)
	{
		return;
	}
	Health -= Damage;
	if (Health <= 0.0f)
	{
		GetWorld()->GetGameState<AStrategyGameState>()->NotifyCapitalDestroyed(CaptureState.Owner);
		Destroy();
	}
}

void AStrategyControlPoint::UpdateAppearance()
{
	const UStrategyPresentationDataAsset* Presentation = GetWorld()->GetGameState<AStrategyGameState>()->GetPresentationDefinition();
	StrategyVisuals::ApplyFactionMaterial(Mesh, NAME_None, CaptureState.Owner, Presentation);
	StrategyVisuals::ApplyFactionMaterial(FlagMesh, NAME_None, CaptureState.Owner, Presentation);
	CaptureRingMaterial->SetVectorParameterValue(TEXT("FactionColor"), StrategyArtStyle::GetFactionColor(CaptureState.Owner));
}

AStrategyFogOfWar::AStrategyFogOfWar()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;
	FogPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FogPlane"));
	FogPlane->SetupAttachment(RootComponent);
	FogPlane->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FogPlane->SetCastShadow(false);
}

void AStrategyFogOfWar::BeginPlay()
{
	Super::BeginPlay();
	const FStrategySkirmishMapDefinition MapDefinition = FStrategyMapDefinitions::Resolve(GetWorld()->GetMapName());
	PlayerGrid.Initialize(MapDefinition.FogGridSize.X, MapDefinition.FogGridSize.Y, MapDefinition.FogMin, MapDefinition.FogMax);
	EnemyGrid.Initialize(MapDefinition.FogGridSize.X, MapDefinition.FogGridSize.Y, MapDefinition.FogMin, MapDefinition.FogMax);
	GetWorld()->GetGameState<AStrategyGameState>()->SetFogOfWar(this);

	FogTexture = UTexture2D::CreateTransient(MapDefinition.FogGridSize.X, MapDefinition.FogGridSize.Y, PF_B8G8R8A8);
	FogTexture->SRGB = false;
	FogTexture->Filter = TF_Bilinear;
	FogTexture->UpdateResource();
	FogPlane->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	const FVector2D FogSize = MapDefinition.FogMax - MapDefinition.FogMin;
	const FVector2D FogCenter = (MapDefinition.FogMin + MapDefinition.FogMax) * 0.5f;
	FogPlane->SetRelativeScale3D(FVector(FogSize.X / 100.0f, FogSize.Y / 100.0f, 1.0f));
	FogPlane->SetRelativeLocation(FVector(FogCenter.X, FogCenter.Y, 1200.0f));
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/CityStateRTS/Materials/M_Fog.M_Fog"));
	check(Material);
	FogMaterial = UMaterialInstanceDynamic::Create(Material, this);
	FogMaterial->SetTextureParameterValue(TEXT("FogTexture"), FogTexture);
	FogPlane->SetMaterial(0, FogMaterial);
	UpdateFog();
}

void AStrategyFogOfWar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateAccumulator += DeltaSeconds;
	if (UpdateAccumulator >= 0.25f)
	{
		UpdateAccumulator = 0.0f;
		UpdateFog();
	}
}

bool AStrategyFogOfWar::IsVisibleToFaction(EStrategyFaction Faction, const FVector& Location) const
{
	const FVector2D Point(Location.X, Location.Y);
	return Faction == EStrategyFaction::Player ? PlayerGrid.IsVisible(Point) : EnemyGrid.IsVisible(Point);
}

bool AStrategyFogOfWar::IsExploredToFaction(EStrategyFaction Faction, const FVector& Location) const
{
	const FVector2D Point(Location.X, Location.Y);
	return Faction == EStrategyFaction::Player ? PlayerGrid.IsExplored(Point) : EnemyGrid.IsExplored(Point);
}

void AStrategyFogOfWar::UpdateFog()
{
	PlayerGrid.BeginVisibilityUpdate();
	EnemyGrid.BeginVisibilityUpdate();
	RevealFaction(PlayerGrid, EStrategyFaction::Player);
	RevealFaction(EnemyGrid, EStrategyFaction::Enemy);
	UpdateActorVisibility();
	UpdateTexture();
}

void AStrategyFogOfWar::RevealFaction(FStrategyFogGrid& Grid, EStrategyFaction Faction)
{
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	for (const AStrategySquad* Squad : State->GetSquads())
	{
		if (!IsValid(Squad) || Squad->GetFaction() != Faction)
		{
			continue;
		}
		const float Radius = Squad->GetUnitType() == EStrategyUnitType::Cavalry ? 2600.0f : Squad->GetUnitType() == EStrategyUnitType::Archer ? 2200.0f : 1800.0f;
		const FVector Location = Squad->GetCenterLocation();
		Grid.Reveal(FVector2D(Location.X, Location.Y), Radius);
	}
	for (const AStrategyControlPoint* Point : State->GetControlPoints())
	{
		if (IsValid(Point) && Point->GetStrategyFaction() == Faction)
		{
			const FVector Location = Point->GetActorLocation();
			Grid.Reveal(FVector2D(Location.X, Location.Y), Point->IsCapital() ? 3000.0f : 2200.0f);
		}
	}
	for (const AStrategyBuilding* Building : State->GetBuildings())
	{
		if (IsValid(Building) && Building->GetStrategyFaction() == Faction && Building->GetBuildingType() == EStrategyBuildingType::Tower && Building->IsConstructionComplete())
		{
			const FVector Location = Building->GetActorLocation();
			Grid.Reveal(FVector2D(Location.X, Location.Y), 2800.0f);
		}
	}
}

void AStrategyFogOfWar::UpdateActorVisibility()
{
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	for (const AStrategySquad* Squad : State->GetSquads())
	{
		if (!IsValid(Squad) || Squad->GetFaction() != EStrategyFaction::Enemy)
		{
			continue;
		}
		for (AStrategyUnit* Unit : Squad->GetMembers())
		{
			if (IsValid(Unit))
			{
				Unit->SetActorHiddenInGame(!PlayerGrid.IsVisible(FVector2D(Unit->GetActorLocation().X, Unit->GetActorLocation().Y)));
			}
		}
	}
	for (AStrategyBuilding* Building : State->GetBuildings())
	{
		if (IsValid(Building) && Building->GetStrategyFaction() == EStrategyFaction::Enemy)
		{
			Building->SetActorHiddenInGame(!PlayerGrid.IsVisible(FVector2D(Building->GetActorLocation().X, Building->GetActorLocation().Y)));
		}
	}
	for (AStrategyControlPoint* Point : State->GetControlPoints())
	{
		if (IsValid(Point) && Point->GetStrategyFaction() == EStrategyFaction::Enemy)
		{
			Point->SetActorHiddenInGame(!PlayerGrid.IsVisible(FVector2D(Point->GetActorLocation().X, Point->GetActorLocation().Y)));
		}
	}
}

void AStrategyFogOfWar::UpdateTexture()
{
	if (!FogTexture || !FogTexture->GetPlatformData() || FogTexture->GetPlatformData()->Mips.IsEmpty())
	{
		return;
	}
	TArray<FColor> Pixels;
	Pixels.SetNumUninitialized(PlayerGrid.GetWidth() * PlayerGrid.GetHeight());
	for (int32 Index = 0; Index < Pixels.Num(); ++Index)
	{
		const uint8 Alpha = PlayerGrid.IsCellVisible(Index) ? 0 : PlayerGrid.IsCellExplored(Index) ? 140 : 250;
		Pixels[Index] = FColor(0, 0, 0, Alpha);
	}
	FTexture2DMipMap& Mip = FogTexture->GetPlatformData()->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
	Mip.BulkData.Unlock();
	FogTexture->UpdateResource();
}

AStrategyAICommander::AStrategyAICommander()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AStrategyAICommander::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	DecisionAccumulator += DeltaSeconds;
	if (DecisionAccumulator >= 1.0f)
	{
		DecisionAccumulator = 0.0f;
		RunDecision();
	}
}

void AStrategyAICommander::RunDecision()
{
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	if (!State || !State->IsMatchRunning())
	{
		return;
	}
	const FStrategyFactionState& FactionState = State->GetFactionState(EStrategyFaction::Enemy);
	if (++DecisionsSinceStatus >= 30)
	{
		DecisionsSinceStatus = 0;
		UE_LOG(LogTemp, Display, TEXT("StrategyAI Status Gold=%.0f Income=%.1f OwnedPoints=%d"),
			FactionState.Gold, FactionState.IncomePerSecond, FactionState.OwnedPoints);
	}

	if (AStrategyControlPoint* ThreatenedPoint = FindThreatenedPoint())
	{
		FStrategyOrder Order;
		Order.Type = EStrategyOrderType::AttackMove;
		Order.Destination = ThreatenedPoint->GetActorLocation();
		IssueAllSquads(Order);
		return;
	}

	UpdateGarrisonBehavior();

	FStrategyTownAIInputs TownInputs;
	AStrategyControlPoint* DisabledTown = nullptr;
	AStrategyControlPoint* UnspecializedTown = nullptr;
	bool bTownProjectActive = false;
	for (AStrategyControlPoint* Point : State->GetControlPoints())
	{
		if (!IsValid(Point) || Point->IsCapital() || Point->GetStrategyFaction() != EStrategyFaction::Enemy)
		{
			continue;
		}

		const FStrategyTownDevelopment& Development = Point->GetTownDevelopment();
		const bool bPreservedOrActive = Development.State == EStrategyTownDevelopmentState::Active
			|| Development.State == EStrategyTownDevelopmentState::DisabledAfterCapture;
		TownInputs.bHasTradeTown |= bPreservedOrActive
			&& Development.Specialization == EStrategyTownSpecialization::Trade;
		TownInputs.bHasRecruitmentTown |= bPreservedOrActive
			&& Development.Specialization == EStrategyTownSpecialization::Recruitment;
		bTownProjectActive |= Development.State == EStrategyTownDevelopmentState::Building
			|| Development.State == EStrategyTownDevelopmentState::Downgrading;
		if (Development.State == EStrategyTownDevelopmentState::DisabledAfterCapture)
		{
			InheritedTowns.Add(Point);
			DisabledTown = DisabledTown ? DisabledTown : Point;
		}
		else if (Development.State == EStrategyTownDevelopmentState::Unspecialized)
		{
			UnspecializedTown = UnspecializedTown ? UnspecializedTown : Point;
		}
	}

	if (DisabledTown)
	{
		FStrategyOrder Order;
		Order.Type = EStrategyOrderType::AttackMove;
		Order.Destination = DisabledTown->GetActorLocation();
		IssueAllSquads(Order, 1);
		return;
	}

	if (!bTownProjectActive && UnspecializedTown
		&& FactionState.Gold >= FStrategyTownDevelopmentRules::SpecializationCost)
	{
		TownInputs.bTownThreatened = IsTownThreatened(UnspecializedTown);
		const EStrategyTownSpecialization Specialization = FStrategyTownAIPlanner::ChooseSpecialization(TownInputs);
		if (State->TryStartTownSpecialization(EStrategyFaction::Enemy, UnspecializedTown, Specialization))
		{
			UE_LOG(LogTemp, Display, TEXT("StrategyAI TownSpecialization Choice=%d"),
				static_cast<int32>(Specialization));
			return;
		}
	}

	if (!bTownProjectActive && FactionState.Gold >= 500.0f)
	{
		for (auto TownIterator = InheritedTowns.CreateIterator(); TownIterator; ++TownIterator)
		{
			AStrategyControlPoint* Town = TownIterator->Get();
			if (!Town || Town->GetStrategyFaction() != EStrategyFaction::Enemy
				|| Town->GetTownDevelopment().State != EStrategyTownDevelopmentState::Active)
			{
				continue;
			}

			TownInputs.bTownThreatened = IsTownThreatened(Town);
			const EStrategyTownSpecialization Preferred = FStrategyTownAIPlanner::ChooseSpecialization(TownInputs);
			if (FStrategyTownAIPlanner::ShouldRespecialize(
				Town->GetTownDevelopment().Specialization, Preferred, FactionState.Gold)
				&& State->TryStartTownDowngrade(EStrategyFaction::Enemy, Town))
			{
				UE_LOG(LogTemp, Display, TEXT("StrategyAI TownDowngrade Current=%d Preferred=%d"),
					static_cast<int32>(Town->GetTownDevelopment().Specialization), static_cast<int32>(Preferred));
				TownIterator.RemoveCurrent();
				return;
			}
		}
	}

	FStrategyAIInputs Inputs;
	Inputs.bOwnedPointThreatened = false;
	Inputs.bNeutralPointAvailable = FindNeutralPoint() != nullptr;
	Inputs.bMissingProductionBuilding = FindMissingProductionBuilding() != EStrategyBuildingType::Capital;
	for (const AStrategySquad* Squad : State->GetSquads())
	{
		Inputs.AvailableSquads += IsValid(Squad) && Squad->GetFaction() == EStrategyFaction::Enemy
			&& !Squad->IsGarrisoned() && !Squad->IsGarrisonRequested();
	}

	if (FactionState.PopulationCap - FactionState.UsedPopulation - FactionState.ReservedPopulation < 8)
	{
		TryBuild(EStrategyBuildingType::House);
	}

	const EStrategyAIAction Action = FStrategyAIPlanner::ChooseAction(Inputs);
	if (Action != LastLoggedAction)
	{
		LastLoggedAction = Action;
		UE_LOG(LogTemp, Display, TEXT("StrategyAI Action=%d"), static_cast<int32>(Action));
	}
	switch (Action)
	{
	case EStrategyAIAction::Defend:
	{
		FStrategyOrder Order;
		Order.Type = EStrategyOrderType::AttackMove;
		Order.Destination = FindThreatenedPoint()->GetActorLocation();
		IssueAllSquads(Order);
		break;
	}
	case EStrategyAIAction::Capture:
	{
		FStrategyOrder Order;
		Order.Type = EStrategyOrderType::AttackMove;
		Order.Destination = FindNeutralPoint()->GetActorLocation();
		IssueAllSquads(Order, 1);
		break;
	}
	case EStrategyAIAction::Build:
		TryBuild(FindMissingProductionBuilding());
		break;
	case EStrategyAIAction::Attack:
	{
		AStrategyControlPoint* Target = nullptr;
		for (AStrategyControlPoint* Point : State->GetControlPoints())
		{
			if (IsValid(Point) && Point->GetStrategyFaction() == EStrategyFaction::Player && !Point->IsCapital())
			{
				Target = Point;
				break;
			}
		}
		Target = Target ? Target : State->FindCapital(EStrategyFaction::Player);
		if (Target)
		{
			FStrategyOrder Order;
			Order.Type = EStrategyOrderType::AttackMove;
			Order.Destination = Target->GetActorLocation();
			IssueAllSquads(Order);
		}
		break;
	}
	default:
		TrainCounterUnit();
		break;
	}
}

AStrategyControlPoint* AStrategyAICommander::FindThreatenedPoint() const
{
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	for (AStrategyControlPoint* Point : State->GetControlPoints())
	{
		if (!IsValid(Point) || Point->GetStrategyFaction() != EStrategyFaction::Enemy)
		{
			continue;
		}
		if (IsTownThreatened(Point))
		{
			return Point;
		}
	}
	return nullptr;
}

bool AStrategyAICommander::IsTownThreatened(const AStrategyControlPoint* Point) const
{
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	for (AStrategySquad* Squad : State->GetSquads())
	{
		if (IsValid(Squad) && Squad->GetFaction() == EStrategyFaction::Player
			&& !Squad->IsGarrisoned()
			&& State->IsVisibleToFaction(EStrategyFaction::Enemy, Squad->GetCenterLocation())
			&& FVector::DistSquared2D(Point->GetActorLocation(), Squad->GetCenterLocation()) < FMath::Square(2200.0f))
		{
			return true;
		}
	}
	return false;
}

AStrategyControlPoint* AStrategyAICommander::FindNeutralPoint() const
{
	for (AStrategyControlPoint* Point : GetWorld()->GetGameState<AStrategyGameState>()->GetControlPoints())
	{
		if (IsValid(Point) && Point->GetStrategyFaction() == EStrategyFaction::Neutral)
		{
			return Point;
		}
	}
	return nullptr;
}

EStrategyBuildingType AStrategyAICommander::FindMissingProductionBuilding() const
{
	bool Found[3] = {false, false, false};
	for (const AStrategyBuilding* Building : GetWorld()->GetGameState<AStrategyGameState>()->GetBuildings())
	{
		if (IsValid(Building) && Building->GetStrategyFaction() == EStrategyFaction::Enemy)
		{
			const int32 Index = static_cast<int32>(Building->GetBuildingType());
			if (Index >= 0 && Index < 3)
			{
				Found[Index] = true;
			}
		}
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (!Found[Index])
		{
			return static_cast<EStrategyBuildingType>(Index);
		}
	}
	return EStrategyBuildingType::Capital;
}

bool AStrategyAICommander::TryBuild(EStrategyBuildingType BuildingType)
{
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	for (int32 Attempt = 0; Attempt < 12; ++Attempt)
	{
		const FVector Location = GetNextBuildLocation();
		if (State->TryPlaceBuilding(EStrategyFaction::Enemy, BuildingType, Location))
		{
			UE_LOG(LogTemp, Display, TEXT("StrategyAI Build Type=%d"), static_cast<int32>(BuildingType));
			return true;
		}
	}
	return false;
}

void AStrategyAICommander::TrainCounterUnit()
{
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	EStrategyUnitType DesiredType = EStrategyUnitType::Infantry;
	for (const AStrategySquad* Squad : State->GetSquads())
	{
		if (IsValid(Squad) && Squad->GetFaction() == EStrategyFaction::Player && !Squad->IsGarrisoned()
			&& State->IsVisibleToFaction(EStrategyFaction::Enemy, Squad->GetCenterLocation()))
		{
			DesiredType = Squad->GetUnitType() == EStrategyUnitType::Infantry ? EStrategyUnitType::Archer : Squad->GetUnitType() == EStrategyUnitType::Archer ? EStrategyUnitType::Cavalry : EStrategyUnitType::Infantry;
			break;
		}
	}

	for (AStrategyBuilding* Building : State->GetBuildings())
	{
		if (IsValid(Building) && Building->GetStrategyFaction() == EStrategyFaction::Enemy && Building->QueueUnit(DesiredType))
		{
			UE_LOG(LogTemp, Display, TEXT("StrategyAI Train Type=%d"), static_cast<int32>(DesiredType));
			return;
		}
	}
	for (AStrategyBuilding* Building : State->GetBuildings())
	{
		if (!IsValid(Building) || Building->GetStrategyFaction() != EStrategyFaction::Enemy)
		{
			continue;
		}
		for (EStrategyUnitType Type : {EStrategyUnitType::Infantry, EStrategyUnitType::Archer, EStrategyUnitType::Cavalry})
		{
			if (Building->QueueUnit(Type))
			{
				UE_LOG(LogTemp, Display, TEXT("StrategyAI Train Type=%d"), static_cast<int32>(Type));
				return;
			}
		}
	}
}

void AStrategyAICommander::IssueAllSquads(const FStrategyOrder& Order, int32 MaximumSquads)
{
	int32 Issued = 0;
	for (AStrategySquad* Squad : GetWorld()->GetGameState<AStrategyGameState>()->GetSquads())
	{
		if (IsValid(Squad) && Squad->GetFaction() == EStrategyFaction::Enemy
			&& !Squad->IsGarrisoned() && !Squad->IsGarrisonRequested())
		{
			Squad->IssueOrder(Order);
			if (++Issued >= MaximumSquads)
			{
				return;
			}
		}
	}
}

void AStrategyAICommander::UpdateGarrisonBehavior()
{
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	TArray<AStrategyControlPoint*> Points;
	TArray<FStrategyGarrisonDestination> Destinations;
	for (AStrategyControlPoint* Point : State->GetControlPoints())
	{
		if (!IsValid(Point) || Point->GetStrategyFaction() != EStrategyFaction::Enemy)
		{
			continue;
		}
		const FStrategyTownDevelopment& Development = Point->GetTownDevelopment();
		Points.Add(Point);
		Destinations.Add({FVector2D(Point->GetActorLocation().X, Point->GetActorLocation().Y),
			Point->GetGarrisonCapacity() - Point->GetGarrisonedSquads().Num(),
			Development.Specialization == EStrategyTownSpecialization::Fortress
				&& Development.State == EStrategyTownDevelopmentState::Active,
			Point->IsCapital()});
	}

	const FVector ExitTarget = FindRecoveryExitTarget();
	for (AStrategySquad* Squad : State->GetSquads())
	{
		if (!IsValid(Squad) || Squad->GetFaction() != EStrategyFaction::Enemy)
		{
			continue;
		}
		if (Squad->IsGarrisoned())
		{
			if (FStrategyGarrisonRules::ShouldAILeave(Squad->GetHealthPercent()))
			{
				AStrategyControlPoint* Point = Squad->GetGarrisonPoint();
				FVector Direction = (ExitTarget - Point->GetActorLocation()).GetSafeNormal2D();
				Direction = Direction.IsNearlyZero() ? FVector::ForwardVector : Direction;
				Squad->ExitGarrison(Point->GetActorLocation() + Direction * 760.0f);
			}
			continue;
		}
		if (!Squad->IsGarrisonRequested() && FStrategyGarrisonRules::ShouldAIRetreat(Squad->GetHealthPercent()))
		{
			const FVector Location = Squad->GetCenterLocation();
			const int32 Index = FStrategyGarrisonRules::FindBestDestination(
				FVector2D(Location.X, Location.Y), Destinations);
			if (Points.IsValidIndex(Index))
			{
				Squad->RequestGarrison(Points[Index]);
				--Destinations[Index].AvailableSlots;
			}
		}
	}
}

FVector AStrategyAICommander::FindRecoveryExitTarget() const
{
	AStrategyGameState* State = GetWorld()->GetGameState<AStrategyGameState>();
	if (AStrategyControlPoint* Neutral = FindNeutralPoint())
	{
		return Neutral->GetActorLocation();
	}
	for (AStrategyControlPoint* Point : State->GetControlPoints())
	{
		if (IsValid(Point) && Point->GetStrategyFaction() == EStrategyFaction::Player && !Point->IsCapital())
		{
			return Point->GetActorLocation();
		}
	}
	if (AStrategyControlPoint* Capital = State->FindCapital(EStrategyFaction::Player))
	{
		return Capital->GetActorLocation();
	}
	return FVector::ZeroVector;
}

FVector AStrategyAICommander::GetNextBuildLocation() const
{
	const AStrategyControlPoint* Capital = GetWorld()->GetGameState<AStrategyGameState>()->FindCapital(EStrategyFaction::Enemy);
	check(Capital);
	const float Angle = FMath::DegreesToRadians(static_cast<float>((BuildIndex++ * 47) % 360));
	const float Radius = 1200.0f + (BuildIndex % 3) * 450.0f;
	return Capital->GetActorLocation() + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
}
