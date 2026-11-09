// Fill out your copyright notice in the Description page of Project Settings.


#include "Pickups/USPickup.h"
#include "Agents/USPlayerCharacter.h"
#include "Components/USAttributeComponent.h"
#include "Components/USExperienceComponent.h"
#include "Components/USCurrencyComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AUSPickup::AUSPickup()
{
	PrimaryActorTick.bCanEverTick = true;

	// 획득 트리거 구체를 루트로 사용 (Pawn 오버랩만 감지)
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(50.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// 표시용 메시 (BP 없이도 보이도록 엔진 기본 구체를 지정 — 머티리얼/비주얼은 에디터에서 교체 가능)
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(CollisionSphere);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
		Mesh->SetRelativeScale3D(FVector(0.3f));
	}
}

void AUSPickup::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionSphere)
	{
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AUSPickup::OnSphereOverlap);
	}

	// 매 프레임 경과 시간을 재지 않고 타이머 한 번으로 끌림 상태를 켠다. 액터가 사라지면 타이머도 같이 정리된다
	if (AutoCollectDelay > 0.0f)
	{
		GetWorldTimerManager().SetTimer(AutoCollectTimerHandle, this, &AUSPickup::StartAutoCollect, AutoCollectDelay, false);
	}

	// 같은 자리에 떨어진 젬들이 한 점·한 줄로 겹치지 않게 수평 무작위 방향으로 튀어 나가게 한다 (Brotato 재료 드롭)
	ScatterStart = GetActorLocation();
	ScatterEnd = ScatterStart;
	ScatterElapsed = ScatterDuration;
	if (ScatterDuration > 0.0f && ScatterDistanceMax > 0.0f)
	{
		const float Angle = FMath::FRandRange(0.0f, UE_TWO_PI);
		const float Distance = FMath::FRandRange(ScatterDistanceMin, FMath::Max(ScatterDistanceMin, ScatterDistanceMax));
		ScatterEnd = ScatterStart + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Distance;
		ScatterElapsed = 0.0f;
	}
}

void AUSPickup::StartAutoCollect()
{
	bAttracted = true;
}

void AUSPickup::InitPickup(EUSPickupType InType, float InAmount)
{
	PickupType = InType;
	Amount = InAmount;
}

void AUSPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bCollected)
		return;

	// 메시만 돌려 충돌 구체는 그대로 둔다. 부모 기준 Yaw를 더하므로 기울인 자세를 유지한 채 수직축으로 돈다.
	// 화면 밖 젬은 돌려 봐야 안 보이므로 건너뛴다.
	if (Mesh && SpinSpeed != 0.0f && Mesh->WasRecentlyRendered(0.2f))
	{
		Mesh->AddRelativeRotation(FRotator(0.0f, SpinSpeed * DeltaTime, 0.0f));
	}

	// 튀어 나가는 동안은 자석을 무시한다 (끝나기 전에 끌리면 흩뿌린 게 안 보임). 끝으로 갈수록 느려지는 EaseOut
	if (ScatterElapsed < ScatterDuration)
	{
		ScatterElapsed = FMath::Min(ScatterElapsed + DeltaTime, ScatterDuration);
		const float Alpha = FMath::InterpEaseOut(0.0f, 1.0f, ScatterElapsed / ScatterDuration, 2.0f);
		SetActorLocation(FMath::Lerp(ScatterStart, ScatterEnd, Alpha));
		return;
	}

	if (CachedPlayerPawn == nullptr)
	{
		CachedPlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	}

	if (CachedPlayerPawn == nullptr)
		return;

	const FVector PlayerLocation = CachedPlayerPawn->GetActorLocation();
	const float Distance = FVector::Dist(GetActorLocation(), PlayerLocation);

	// 아직 끌리지 않았다면 획득반경 진입 여부 검사
	if (!bAttracted)
	{
		float MagnetRadius = FallbackMagnetRadius;
		if (const AUSPlayerCharacter* Player = Cast<AUSPlayerCharacter>(CachedPlayerPawn))
		{
			MagnetRadius = Player->GetMagnetRadius();
		}

		if (Distance <= MagnetRadius)
		{
			bAttracted = true;
		}
	}

	// 끌림 상태면 가속하며 플레이어 쪽으로 이동. 대장 이속(배율 포함)보다 느리면 영영 못 따라오므로 플레이어 속도 기준 하한을 둔다
	if (bAttracted)
	{
		CurrentMagnetSpeed = (CurrentMagnetSpeed <= 0.0f)
			? MagnetSpeed
			: FMath::Min(CurrentMagnetSpeed + MagnetAcceleration * DeltaTime, FMath::Max(MagnetSpeed, MaxMagnetSpeed));
		const float PlayerSpeed = CachedPlayerPawn->GetVelocity().Size2D();
		const float Speed = FMath::Max(CurrentMagnetSpeed, PlayerSpeed * PlayerSpeedFactor);

		const FVector NewLocation = FMath::VInterpConstantTo(GetActorLocation(), PlayerLocation, DeltaTime, Speed);
		SetActorLocation(NewLocation);
	}
}

void AUSPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep)
{
	// 플레이어만 획득 가능 (적도 Pawn이므로 타입으로 구분)
	if (AUSPlayerCharacter* Player = Cast<AUSPlayerCharacter>(OtherActor))
	{
		Collect(Player);
	}
}

void AUSPickup::Collect(AUSPlayerCharacter* Collector)
{
	if (bCollected || Collector == nullptr)
		return;

	if (PickupType == EUSPickupType::Experience)
	{
		if (UUSExperienceComponent* Experience = Collector->GetExperienceComponent())
		{
			Experience->AddPoints(Amount);
		}
	}
	else // Gold
	{
		if (UUSCurrencyComponent* Currency = Collector->GetCurrencyComponent())
		{
			Currency->AddEarnedGold(Amount);
		}
	}

	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
	}

	bCollected = true;
	Destroy();
}
