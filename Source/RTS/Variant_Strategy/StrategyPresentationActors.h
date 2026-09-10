#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StrategyPresentationActors.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class UStaticMesh;
class UStaticMeshComponent;

/** 只负责飞行表现，不参与碰撞、命中或伤害计算。 */
UCLASS()
class AStrategyProjectileVisual : public AActor
{
	GENERATED_BODY()

public:
	AStrategyProjectileVisual();
	virtual void Tick(float DeltaSeconds) override;
	void Initialize(UStaticMesh* Mesh, UNiagaraSystem* TrailEffect, const FVector& Start, const FVector& End, float Duration);

private:
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> Trail;

	FVector StartLocation = FVector::ZeroVector;
	FVector EndLocation = FVector::ZeroVector;
	float TravelDuration = 0.05f;
	float Elapsed = 0.0f;
};
