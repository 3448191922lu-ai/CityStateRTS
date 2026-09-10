#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyMapTerrain.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;
struct FStrategySkirmishMapDefinition;
class UStrategyPresentationDataAsset;

UCLASS()
class AStrategyMapTerrain : public AActor
{
	GENERATED_BODY()

public:
	AStrategyMapTerrain();
	void InitializePresentation(const FStrategySkirmishMapDefinition& Definition);

private:
	void InitializeRiverValley(const UStrategyPresentationDataAsset* Presentation);
	UStaticMeshComponent* AddCollisionBlock(const FName& Name, const FVector& Location, const FVector& Size);
	UStaticMeshComponent* AddColoredVisual(const FName& Name, const FVector& Location, const FVector& Size,
		const FLinearColor& Color);
	UStaticMeshComponent* AddMeshVisual(const FName& Name, UStaticMesh* Mesh, const FVector& Location,
		const FVector& DesiredSize, const FRotator& Rotation = FRotator::ZeroRotator);
	void AddDecoration(const FName& Name, UStaticMesh* Mesh, const TArray<FTransform>& Transforms);

	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot;
};
