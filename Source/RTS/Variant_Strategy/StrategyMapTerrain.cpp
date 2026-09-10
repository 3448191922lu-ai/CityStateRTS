#include "StrategyMapTerrain.h"

#include "StrategyGameState.h"
#include "StrategyMapDefinition.h"
#include "StrategyTypes.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AStrategyMapTerrain::AStrategyMapTerrain()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = SceneRoot;
}

void AStrategyMapTerrain::InitializePresentation(const FStrategySkirmishMapDefinition& Definition)
{
	const UStrategyPresentationDataAsset* Presentation = GetWorld()->GetGameState<AStrategyGameState>()->GetPresentationDefinition();
	if (Definition.bSpawnRiverValleyTerrain)
	{
		InitializeRiverValley(Presentation);
	}

	// 两张地图共用少量成对装饰，避开主城、据点和主通路。
	AddDecoration(TEXT("CommonTreesA"), Presentation->TreeMeshA, {
		FTransform(FRotator(0.0f, 20.0f, 0.0f), FVector(-8200.0f, 6100.0f, 0.0f), FVector(2.2f)),
		FTransform(FRotator(0.0f, 200.0f, 0.0f), FVector(8200.0f, -6100.0f, 0.0f), FVector(2.2f)),
		FTransform(FRotator(0.0f, 65.0f, 0.0f), FVector(-6200.0f, -6500.0f, 0.0f), FVector(1.8f)),
		FTransform(FRotator(0.0f, 245.0f, 0.0f), FVector(6200.0f, 6500.0f, 0.0f), FVector(1.8f))});
	AddDecoration(TEXT("CommonTreesB"), Presentation->TreeMeshB, {
		FTransform(FRotator(0.0f, 105.0f, 0.0f), FVector(-8800.0f, -5200.0f, 0.0f), FVector(1.7f)),
		FTransform(FRotator(0.0f, 285.0f, 0.0f), FVector(8800.0f, 5200.0f, 0.0f), FVector(1.7f))});
	AddDecoration(TEXT("CommonRocks"), Presentation->RockMeshA, {
		FTransform(FRotator(0.0f, 35.0f, 0.0f), FVector(-7600.0f, 6900.0f, 0.0f), FVector(2.0f)),
		FTransform(FRotator(0.0f, 215.0f, 0.0f), FVector(7600.0f, -6900.0f, 0.0f), FVector(2.0f)),
		FTransform(FRotator(0.0f, 80.0f, 0.0f), FVector(-9000.0f, -6500.0f, 0.0f), FVector(1.4f)),
		FTransform(FRotator(0.0f, 260.0f, 0.0f), FVector(9000.0f, 6500.0f, 0.0f), FVector(1.4f))});
}

void AStrategyMapTerrain::InitializeRiverValley(const UStrategyPresentationDataAsset* Presentation)
{
	const FLinearColor WaterColor(0.05f, 0.28f, 0.55f);
	const FLinearColor BridgeColor(0.36f, 0.20f, 0.08f);
	const FLinearColor FordColor(0.18f, 0.48f, 0.64f);

	// 保留原阻挡体的位置和尺寸，水面仅负责显示。
	for (const TTuple<FName, FVector, FVector>& River : {
		TTuple<FName, FVector, FVector>(TEXT("RiverSouth"), FVector(0.0f, -5450.0f, 40.0f), FVector(1100.0f, 3100.0f, 80.0f)),
		TTuple<FName, FVector, FVector>(TEXT("RiverSouthCenter"), FVector(0.0f, -1400.0f, 40.0f), FVector(1100.0f, 1400.0f, 80.0f)),
		TTuple<FName, FVector, FVector>(TEXT("RiverNorthCenter"), FVector(0.0f, 1400.0f, 40.0f), FVector(1100.0f, 1400.0f, 80.0f)),
		TTuple<FName, FVector, FVector>(TEXT("RiverNorth"), FVector(0.0f, 5450.0f, 40.0f), FVector(1100.0f, 3100.0f, 80.0f))})
	{
		AddCollisionBlock(*FString::Printf(TEXT("%sCollision"), *River.Get<0>().ToString()), River.Get<1>(), River.Get<2>());
		AddColoredVisual(*FString::Printf(TEXT("%sWater"), *River.Get<0>().ToString()), River.Get<1>(), River.Get<2>(), WaterColor);
	}

	if (Presentation->BridgeMesh)
	{
		AddMeshVisual(TEXT("SouthBridge"), Presentation->BridgeMesh, FVector(0.0f, -3000.0f, 12.0f), FVector(1400.0f, 1800.0f, 180.0f));
		AddMeshVisual(TEXT("NorthBridge"), Presentation->BridgeMesh, FVector(0.0f, 3000.0f, 12.0f), FVector(1400.0f, 1800.0f, 180.0f));
	}
	else
	{
		AddColoredVisual(TEXT("SouthBridge"), FVector(0.0f, -3000.0f, 12.0f), FVector(1400.0f, 1800.0f, 24.0f), BridgeColor);
		AddColoredVisual(TEXT("NorthBridge"), FVector(0.0f, 3000.0f, 12.0f), FVector(1400.0f, 1800.0f, 24.0f), BridgeColor);
	}
	AddColoredVisual(TEXT("CentralFord"), FVector(0.0f, 0.0f, 5.0f), FVector(1100.0f, 1200.0f, 10.0f), FordColor);

	// 成对布置阻挡物，保持双方路线机会对称。
	AddCollisionBlock(TEXT("NorthWestForestCollision"), FVector(-4200.0f, 5100.0f, 110.0f), FVector(1800.0f, 900.0f, 220.0f));
	AddCollisionBlock(TEXT("SouthEastForestCollision"), FVector(4200.0f, -5100.0f, 110.0f), FVector(1800.0f, 900.0f, 220.0f));
	AddCollisionBlock(TEXT("NorthEastRocksCollision"), FVector(3900.0f, 4550.0f, 130.0f), FVector(1100.0f, 700.0f, 260.0f));
	AddCollisionBlock(TEXT("SouthWestRocksCollision"), FVector(-3900.0f, -4550.0f, 130.0f), FVector(1100.0f, 700.0f, 260.0f));
	AddDecoration(TEXT("RiverForestTrees"), Presentation->TreeMeshA, {
		FTransform(FRotator(0.0f, 15.0f, 0.0f), FVector(-4550.0f, 5100.0f, 0.0f), FVector(2.0f)),
		FTransform(FRotator(0.0f, 75.0f, 0.0f), FVector(-3850.0f, 5100.0f, 0.0f), FVector(1.8f)),
		FTransform(FRotator(0.0f, 195.0f, 0.0f), FVector(4550.0f, -5100.0f, 0.0f), FVector(2.0f)),
		FTransform(FRotator(0.0f, 255.0f, 0.0f), FVector(3850.0f, -5100.0f, 0.0f), FVector(1.8f))});
	AddDecoration(TEXT("RiverRockClusters"), Presentation->RockMeshA, {
		FTransform(FRotator(0.0f, 25.0f, 0.0f), FVector(3650.0f, 4550.0f, 0.0f), FVector(2.1f)),
		FTransform(FRotator(0.0f, 70.0f, 0.0f), FVector(4150.0f, 4550.0f, 0.0f), FVector(1.6f)),
		FTransform(FRotator(0.0f, 205.0f, 0.0f), FVector(-3650.0f, -4550.0f, 0.0f), FVector(2.1f)),
		FTransform(FRotator(0.0f, 250.0f, 0.0f), FVector(-4150.0f, -4550.0f, 0.0f), FVector(1.6f))});
}

UStaticMeshComponent* AStrategyMapTerrain::AddCollisionBlock(const FName& Name, const FVector& Location, const FVector& Size)
{
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, Name);
	Component->SetupAttachment(SceneRoot);
	Component->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Component->SetRelativeLocation(Location);
	Component->SetRelativeScale3D(Size / 100.0f);
	Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Component->SetCollisionProfileName(TEXT("BlockAll"));
	Component->SetCanEverAffectNavigation(true);
	Component->SetCastShadow(false);
	Component->SetHiddenInGame(true);
	AddInstanceComponent(Component);
	Component->RegisterComponent();
	return Component;
}

UStaticMeshComponent* AStrategyMapTerrain::AddColoredVisual(const FName& Name, const FVector& Location, const FVector& Size,
	const FLinearColor& Color)
{
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, Name);
	Component->SetupAttachment(SceneRoot);
	Component->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
	Component->SetRelativeLocation(Location);
	Component->SetRelativeScale3D(Size / 100.0f);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetCanEverAffectNavigation(false);
	Component->SetCastShadow(false);
	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, Component);
	Material->SetVectorParameterValue(TEXT("Color"), Color);
	Component->SetMaterial(0, Material);
	AddInstanceComponent(Component);
	Component->RegisterComponent();
	return Component;
}

UStaticMeshComponent* AStrategyMapTerrain::AddMeshVisual(const FName& Name, UStaticMesh* Mesh, const FVector& Location,
	const FVector& DesiredSize, const FRotator& Rotation)
{
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this, Name);
	Component->SetupAttachment(SceneRoot);
	Component->SetStaticMesh(Mesh);
	Component->SetRelativeLocation(Location);
	Component->SetRelativeRotation(Rotation);
	const FVector MeshSize = Mesh->GetBounds().BoxExtent * 2.0f;
	Component->SetRelativeScale3D(DesiredSize / MeshSize);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetCanEverAffectNavigation(false);
	AddInstanceComponent(Component);
	Component->RegisterComponent();
	return Component;
}

void AStrategyMapTerrain::AddDecoration(const FName& Name, UStaticMesh* Mesh, const TArray<FTransform>& Transforms)
{
	UHierarchicalInstancedStaticMeshComponent* Component = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, Name);
	Component->SetupAttachment(SceneRoot);
	Component->SetStaticMesh(Mesh);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetCanEverAffectNavigation(false);
	for (const FTransform& Transform : Transforms)
	{
		Component->AddInstance(Transform);
	}
	AddInstanceComponent(Component);
	Component->RegisterComponent();
}
