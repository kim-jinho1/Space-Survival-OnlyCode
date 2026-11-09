#include "Equipments/USMissile.h"
#include "US.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"

AUSMissile::AUSMissile()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	// 표시 전용: 충돌·그림자·내비 영향 없음
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->CastShadow = false;

	Trail = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Trail"));
	Trail->SetupAttachment(Root);
}

void AUSMissile::Launch(const FVector& Start, const FVector& Target)
{
	StartLocation = Start;
	TargetLocation = Target;
	ControlLocation = (Start + Target) * 0.5f + FVector(0.0f, 0.0f, ArcHeight);
	Elapsed = 0.0f;
	bFlying = true;

	SetActorLocation(Start);

	if (Trail && Trail->Template == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: missile has no trail template; it flies without smoke"), *GetName());
	}
}

void AUSMissile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bFlying)
	{
		return;
	}

	Elapsed += DeltaSeconds;
	const float Alpha = FMath::Min(Elapsed / FlightTime, 1.0f);

	// 2차 베지어: B(t) = (1-t)²P0 + 2(1-t)t P1 + t²P2, 진행 방향은 B'(t) = 2(1-t)(P1-P0) + 2t(P2-P1)
	const float InvAlpha = 1.0f - Alpha;
	const FVector Location = InvAlpha * InvAlpha * StartLocation + 2.0f * InvAlpha * Alpha * ControlLocation + Alpha * Alpha * TargetLocation;
	const FVector Direction = 2.0f * InvAlpha * (ControlLocation - StartLocation) + 2.0f * Alpha * (TargetLocation - ControlLocation);

	SetActorLocationAndRotation(Location, Direction.IsNearlyZero() ? GetActorRotation() : Direction.Rotation());

	if (Alpha >= 1.0f)
	{
		Arrive();
	}
}

void AUSMissile::Arrive()
{
	bFlying = false;
	SetActorTickEnabled(false);

	OnArrived.ExecuteIfBound(TargetLocation);
	OnArrived.Unbind();

	// 몸은 바로 숨기고, 이미 뿜은 연기 입자는 새로 만들지 않은 채 사라지게 둔다
	if (Mesh)
	{
		Mesh->SetVisibility(false);
	}
	if (Trail)
	{
		Trail->DeactivateSystem();
	}
	SetLifeSpan(FMath::Max(TrailLingerTime, 0.01f));
}
