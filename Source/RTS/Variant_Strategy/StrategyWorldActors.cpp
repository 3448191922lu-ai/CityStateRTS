#include "StrategyWorldActors.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NavigationSystem.h"
#include "StrategyGameState.h"
#include "StrategyRules.h"
#include "StrategyUnit.h"
#include "StrategyArtStyle.h"

namespace StrategyVisuals
{
	static void ApplyFactionMaterial(UStaticMeshComponent* Mesh, EStrategyFaction Faction)
	{
		UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, Mesh);
		Material->SetVectorParameterValue(TEXT("Color"), StrategyArtStyle::GetFactionColor(Faction));
		Mesh->SetMaterial(0, Material);
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
	if (!Members.IsEmpty())
	{
		SetActorLocation(GetCenterLocation());
	}
}

void AStrategySquad::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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

	TSubclassOf<AStrategyUnit> UnitClass = Definition->UnitClass;
	if (!UnitClass)
	{
		UnitClass = AStrategyUnit::StaticClass();
	}
	for (int32 Index = 0; Index < Definition->MemberCount; ++Index)
	{
		const int32 Row = Index / 2;
		const int32 Column = Index % 2;
		const FVector Offset((Row - 0.5f) * 140.0f, (Column - 0.5f) * 140.0f, 100.0f);
		AStrategyUnit* Unit = GetWorld()->SpawnActor<AStrategyUnit>(UnitClass, GetActorLocation() + Offset, FRotator::ZeroRotator);
		check(Unit);
		Unit->Initialize(this, Faction, Definition);
		Members.Add(Unit);
	}
}

void AStrategySquad::IssueOrder(const FStrategyOrder& Order)
{
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

void AStrategySquad::SetSelected(bool bSelected)
{
	for (AStrategyUnit* Unit : Members)
	{
		if (IsValid(Unit))
		{
			bSelected ? Unit->UnitSelected() : Unit->UnitDeselected();
		}
	}
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
	GetWorld()->GetGameState<AStrategyGameState>()->RegisterBuilding(this);
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
	return TrainingQueue.Enqueue(UnitType, UnitDefinition->TrainingTime, UnitDefinition->PopulationCost);
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

void AStrategyBuilding::ReceiveStrategyDamage(float Damage, EStrategyUnitType AttackerType, EStrategyFaction SourceFaction)
{
	if (SourceFaction == Faction || !IsStrategyAlive())
	{
		return;
	}
	Health -= Damage;
	if (Health <= 0.0f)
	{
		ApplyCleanup();
		Destroy();
	}
}

void AStrategyBuilding::CompleteConstruction()
{
	bConstructionComplete = true;
	UpdateAppearanceScale(1.0f);
	UpdateCollision();
	if (Definition->PopulationBonus > 0)
	{
		GetWorld()->GetGameState<AStrategyGameState>()->AdjustPopulationCap(Faction, Definition->PopulationBonus);
	}
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
		if (bConstructionComplete && Definition && Definition->PopulationBonus > 0)
		{
			State->AdjustPopulationCap(Faction, -Definition->PopulationBonus);
		}
		State->ReleaseReservedPopulation(Faction, TrainingQueue.GetReservedPopulation());
		State->UnregisterBuilding(this);
	}
}

void AStrategyBuilding::UpdateAppearance()
{
	const TCHAR* MeshPath = GetBuildingType() == EStrategyBuildingType::Tower
		? TEXT("/Engine/BasicShapes/Cylinder.Cylinder")
		: TEXT("/Engine/BasicShapes/Cube.Cube");
	UStaticMesh* BuildingMesh = Definition->VisualMesh.Get();
	if (!BuildingMesh)
	{
		BuildingMesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
	}
	Mesh->SetStaticMesh(BuildingMesh);
	GateLeftPost->SetStaticMesh(BuildingMesh);
	GateRightPost->SetStaticMesh(BuildingMesh);

	const bool bGate = GetBuildingType() == EStrategyBuildingType::Gate;
	GateLeftPost->SetVisibility(bGate);
	GateRightPost->SetVisibility(bGate);
	UpdateAppearanceScale(1.0f);
	if (Definition->VisualMaterial)
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Definition->VisualMaterial, this);
		Material->SetVectorParameterValue(TEXT("Color"), StrategyArtStyle::GetBuildingColor(GetBuildingType(), Faction));
		Mesh->SetMaterial(0, Material);
		GateLeftPost->SetMaterial(0, Material);
		GateRightPost->SetMaterial(0, Material);
	}
	else
	{
		StrategyVisuals::ApplyFactionMaterial(Mesh, Faction);
		StrategyVisuals::ApplyFactionMaterial(GateLeftPost, Faction);
		StrategyVisuals::ApplyFactionMaterial(GateRightPost, Faction);
	}
}

void AStrategyBuilding::UpdateAppearanceScale(float HeightAlpha)
{
	if (GetBuildingType() == EStrategyBuildingType::Gate)
	{
		const FVector GateScale = Definition->VisualScale;
		Mesh->SetRelativeScale3D(FVector(GateScale.X, GateScale.Y, 0.8f * HeightAlpha));
		Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, 260.0f * HeightAlpha));
		GateLeftPost->SetRelativeScale3D(FVector(0.8f, GateScale.Y, 2.2f * HeightAlpha));
		GateLeftPost->SetRelativeLocation(FVector(-160.0f, 0.0f, 110.0f * HeightAlpha));
		GateRightPost->SetRelativeScale3D(FVector(0.8f, GateScale.Y, 2.2f * HeightAlpha));
		GateRightPost->SetRelativeLocation(FVector(160.0f, 0.0f, 110.0f * HeightAlpha));
		return;
	}

	const bool bWall = GetBuildingType() == EStrategyBuildingType::Wall;
	const FVector FinalScale = bWall
		? Definition->VisualScale
		: StrategyArtStyle::GetBuildingSilhouetteScale(GetBuildingType()) * Definition->VisualScale;
	Mesh->SetRelativeScale3D(FVector(FinalScale.X, FinalScale.Y, FinalScale.Z * HeightAlpha));
	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, FinalScale.Z * 50.0f * HeightAlpha));
}

AStrategyControlPoint::AStrategyControlPoint()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CaptureArea = CreateDefaultSubobject<USphereComponent>(TEXT("CaptureArea"));
	CaptureArea->SetupAttachment(RootComponent);
	CaptureArea->SetSphereRadius(650.0f);
	CaptureArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AStrategyControlPoint::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bCapital)
	{
		return;
	}

	bool bPlayerPresent = false;
	bool bEnemyPresent = false;
	for (const AStrategySquad* Squad : GetWorld()->GetGameState<AStrategyGameState>()->GetSquads())
	{
		if (!IsValid(Squad) || FVector::DistSquared2D(GetActorLocation(), Squad->GetCenterLocation()) > FMath::Square(650.0f))
		{
			continue;
		}
		bPlayerPresent |= Squad->GetFaction() == EStrategyFaction::Player;
		bEnemyPresent |= Squad->GetFaction() == EStrategyFaction::Enemy;
	}

	const EStrategyFaction OldOwner = CaptureState.Owner;
	CaptureState.Update(DeltaSeconds, bPlayerPresent, bEnemyPresent);
	if (OldOwner != CaptureState.Owner)
	{
		GetWorld()->GetGameState<AStrategyGameState>()->ChangeControlPointOwner(this, OldOwner, CaptureState.Owner);
		UpdateAppearance();
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
	Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, bCapital ? TEXT("/Engine/BasicShapes/Cube.Cube") : TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
	Mesh->SetRelativeScale3D(bCapital ? FVector(5.0f, 5.0f, 5.0f) : FVector(3.0f, 3.0f, 0.6f));
	Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, bCapital ? 250.0f : 30.0f));
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
	StrategyVisuals::ApplyFactionMaterial(Mesh, CaptureState.Owner);
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
	PlayerGrid.Initialize(128, 96, FVector2D(-11000.0f, -9000.0f), FVector2D(11000.0f, 9000.0f));
	EnemyGrid.Initialize(128, 96, FVector2D(-11000.0f, -9000.0f), FVector2D(11000.0f, 9000.0f));
	GetWorld()->GetGameState<AStrategyGameState>()->SetFogOfWar(this);

	FogTexture = UTexture2D::CreateTransient(128, 96, PF_B8G8R8A8);
	FogTexture->SRGB = false;
	FogTexture->Filter = TF_Bilinear;
	FogTexture->UpdateResource();
	FogPlane->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	FogPlane->SetRelativeScale3D(FVector(220.0f, 180.0f, 1.0f));
	FogPlane->SetRelativeLocation(FVector(0.0f, 0.0f, 1200.0f));
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

	FStrategyAIInputs Inputs;
	Inputs.bOwnedPointThreatened = FindThreatenedPoint() != nullptr;
	Inputs.bNeutralPointAvailable = FindNeutralPoint() != nullptr;
	Inputs.bMissingProductionBuilding = FindMissingProductionBuilding() != EStrategyBuildingType::Capital;
	for (const AStrategySquad* Squad : State->GetSquads())
	{
		Inputs.AvailableSquads += IsValid(Squad) && Squad->GetFaction() == EStrategyFaction::Enemy;
	}

	const FStrategyFactionState& FactionState = State->GetFactionState(EStrategyFaction::Enemy);
	if (FactionState.PopulationCap - FactionState.UsedPopulation - FactionState.ReservedPopulation < 8)
	{
		TryBuild(EStrategyBuildingType::House);
	}

	switch (FStrategyAIPlanner::ChooseAction(Inputs))
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
		for (AStrategySquad* Squad : State->GetSquads())
		{
			if (IsValid(Squad) && Squad->GetFaction() == EStrategyFaction::Player && State->IsVisibleToFaction(EStrategyFaction::Enemy, Squad->GetCenterLocation()) && FVector::DistSquared2D(Point->GetActorLocation(), Squad->GetCenterLocation()) < FMath::Square(2200.0f))
			{
				return Point;
			}
		}
	}
	return nullptr;
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
		if (IsValid(Squad) && Squad->GetFaction() == EStrategyFaction::Player && State->IsVisibleToFaction(EStrategyFaction::Enemy, Squad->GetCenterLocation()))
		{
			DesiredType = Squad->GetUnitType() == EStrategyUnitType::Infantry ? EStrategyUnitType::Archer : Squad->GetUnitType() == EStrategyUnitType::Archer ? EStrategyUnitType::Cavalry : EStrategyUnitType::Infantry;
			break;
		}
	}

	for (AStrategyBuilding* Building : State->GetBuildings())
	{
		if (IsValid(Building) && Building->GetStrategyFaction() == EStrategyFaction::Enemy && Building->QueueUnit(DesiredType))
		{
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
		if (IsValid(Squad) && Squad->GetFaction() == EStrategyFaction::Enemy)
		{
			Squad->IssueOrder(Order);
			if (++Issued >= MaximumSquads)
			{
				return;
			}
		}
	}
}

FVector AStrategyAICommander::GetNextBuildLocation() const
{
	const AStrategyControlPoint* Capital = GetWorld()->GetGameState<AStrategyGameState>()->FindCapital(EStrategyFaction::Enemy);
	check(Capital);
	const float Angle = FMath::DegreesToRadians(static_cast<float>((BuildIndex++ * 47) % 360));
	const float Radius = 1200.0f + (BuildIndex % 3) * 450.0f;
	return Capital->GetActorLocation() + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
}
