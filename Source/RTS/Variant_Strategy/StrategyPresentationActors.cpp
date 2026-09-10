#include "StrategyPresentationActors.h"

#include "Components/StaticMeshComponent.h"
#include "NiagaraComponent.h"

AStrategyProjectileVisual::AStrategyProjectileVisual()
{
	PrimaryActorTick.bCanEverTick = true;
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Projectile Mesh"));
	RootComponent = ProjectileMesh;
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Trail = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Trail"));
	Trail->SetupAttachment(RootComponent);
	Trail->SetAutoActivate(false);
}

void AStrategyProjectileVisual::Initialize(UStaticMesh* Mesh, UNiagaraSystem* TrailEffect,
	const FVector& Start, const FVector& End, float Duration)
{
	ProjectileMesh->SetStaticMesh(Mesh);
	Trail->SetAsset(TrailEffect);
	Trail->Activate();
	StartLocation = Start;
	EndLocation = End;
	TravelDuration = FMath::Max(Duration, 0.05f);
	SetActorLocationAndRotation(StartLocation, (EndLocation - StartLocation).Rotation());
}

void AStrategyProjectileVisual::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Elapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(Elapsed / TravelDuration, 0.0f, 1.0f);
	SetActorLocation(FMath::Lerp(StartLocation, EndLocation, Alpha));
	if (Alpha >= 1.0f)
	{
		Destroy();
	}
}
