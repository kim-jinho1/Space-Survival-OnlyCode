#include "Equipments/USBullet.h"
#include "US.h"
#include "Core/USBulletPoolSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

AUSBullet::AUSBullet()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BulletMesh"));
	// 루트를 정하지 않으면 스폰마다 엔진이 "picking one arbitrarily" 경고를 내고 아무 컴포넌트나 루트로 고른다.
	// 궤적(Trail)이 메시에 붙어 있으니 메시를 루트로 삼아야 SetActorLocation으로 둘이 같이 움직인다.
	SetRootComponent(Mesh);

	TrailComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("Trail"));
	TrailComponent->SetupAttachment(Mesh);

	// 풀링하므로 엔진 수명(InitialLifeSpan)은 쓰지 않는다. 못 닿는 총알은 Tick의 MaxFlightTime이 멈춘다.
	InitialLifeSpan = 0.0f;
}

void AUSBullet::Launch(const FVector& Start, const FVector& Target)
{
	// 길쭉한 볼트 메시(+X가 앞)가 나는 방향을 보게 한다. 풀에서 꺼낸 탄은 지난 발사 방향을 들고 있다.
	SetActorLocationAndRotation(Start, (Target - Start).Rotation());
	TargetLocation = Target;
	FlightTime = 0.0f;

	// 고정 수명이면 속도를 낮출 때 도착 전에 끊겨 명중 연출이 사라진다. 이번 거리에 맞춰 제한을 정한다.
	const float Distance = FVector::Dist(Start, Target);
	MaxFlightTime = (Speed > 0.0f ? Distance / Speed : 0.0f) + FlightTimeMargin;

	bLaunched = true;
}

void AUSBullet::ActivateFromPool()
{
	bInPool = false;
	bLingering = false;
	FlightTime = 0.0f;
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);

	if (Mesh)
	{
		Mesh->SetVisibility(true);
	}

	// 풀이 이미 새 위치로 옮긴 뒤라, 여기서 리셋해야 이전 위치에서 끌려오는 꼬리가 생기지 않는다
	if (TrailComponent)
	{
		TrailComponent->ActivateSystem(true);
	}
}

void AUSBullet::DeactivateToPool()
{
	bInPool = true;
	bLaunched = false;
	bLingering = false;
	FlightTime = 0.0f;
	ImpactFX = nullptr;
	ImpactSound = nullptr;

	if (TrailComponent)
	{
		TrailComponent->DeactivateImmediate();
	}

	// 쉬는 총알은 도착 지점에 숨겨 둔다. 충돌도 꺼서 보이지 않는 총알이 트레이스를 막지 않게 한다.
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
}

void AUSBullet::BeginLinger()
{
	bLaunched = false;

	if (TrailLingerTime <= 0.0f)
	{
		ReturnToPool();
		return;
	}

	// 액터를 바로 반납하면 궤적 입자도 같이 사라져 꼬리가 뚝 끊긴다.
	// 새 입자만 멈추고(Deactivate) 메시는 숨긴 채, 남은 꼬리가 사라질 시간을 기다린다.
	if (TrailComponent)
	{
		TrailComponent->Deactivate();
	}
	if (Mesh)
	{
		Mesh->SetVisibility(false);
	}

	bLingering = true;
	LingerTimeLeft = TrailLingerTime;
}

void AUSBullet::ReturnToPool()
{
	UWorld* World = GetWorld();
	UUSBulletPoolSubsystem* Pool = World ? World->GetSubsystem<UUSBulletPoolSubsystem>() : nullptr;
	if (Pool == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("AUSBullet: no bullet pool; destroying %s"), *GetName());
		Destroy();
		return;
	}

	Pool->ReleaseBullet(this);
}

void AUSBullet::SetImpact(UParticleSystem* InImpactFX, USoundBase* InImpactSound, const FRotator& InImpactRotation)
{
	ImpactFX = InImpactFX;
	ImpactSound = InImpactSound;
	ImpactRotation = InImpactRotation;
}

void AUSBullet::PlayImpact() const
{
	if (ImpactFX)
	{
		// 명중마다 생기는 일회성 이펙트라 월드 파티클 풀에서 빌리고 재생이 끝나면 자동 반납한다
		UGameplayStatics::SpawnEmitterAtLocation(this, ImpactFX, TargetLocation, ImpactRotation, FVector(1.0f),
			true, EPSCPoolMethod::AutoRelease);
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, TargetLocation);
	}
}

void AUSBullet::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bLingering)
	{
		LingerTimeLeft -= DeltaSeconds;
		if (LingerTimeLeft <= 0.0f)
		{
			ReturnToPool();
		}
		return;
	}

	if (!bLaunched)
	{
		return;
	}

	FlightTime += DeltaSeconds;
	if (FlightTime > MaxFlightTime)
	{
		// 목표에 못 닿은 총알: 명중 연출 없이 꼬리만 사라지게 한다
		BeginLinger();
		return;
	}

	const FVector Cur = GetActorLocation();
	const FVector ToTarget = TargetLocation - Cur;
	const float Dist = ToTarget.Size();
	const float Step = Speed * DeltaSeconds;

	if (Dist <= Step || Dist < 1.0f)
	{
		// 목표점까지 마저 옮겨야 꼬리가 명중 지점에 닿는다
		SetActorLocation(TargetLocation);
		PlayImpact();
		BeginLinger();
		return;
	}

	SetActorLocation(Cur + ToTarget / Dist * Step);
}
