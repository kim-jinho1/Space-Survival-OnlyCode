#include "Equipments/USEnemyProjectile.h"
#include "US.h"
#include "Core/USTeamSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

AUSEnemyProjectile::AUSEnemyProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	// 우리 편은 모두 Pawn이라 Pawn 겹침만 받는다. 적(보스·졸개)도 Pawn이지만 HandleOverlap이 아군 등록부로 거른다
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(30.0f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AUSEnemyProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (Collision)
	{
		Collision->OnComponentBeginOverlap.AddDynamic(this, &AUSEnemyProjectile::HandleOverlap);
	}
}

void AUSEnemyProjectile::Launch(const FVector& Direction, float InSpeed, float InDamage, float LifeSeconds)
{
	// 탑뷰 게임이라 수평으로만 난다 (보스 기체 높이에서 쏴도 편대와 같은 높이를 지나가게 Z는 쏘는 쪽이 맞춘다)
	const FVector Flat = FVector(Direction.X, Direction.Y, 0.0f).GetSafeNormal();
	Velocity = Flat * InSpeed;
	Damage = InDamage;
	SetActorRotation(Flat.Rotation());
	SetLifeSpan(FMath::Max(LifeSeconds, 0.1f));
}

void AUSEnemyProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 쓸어 가며(sweep) 옮기지 않아도 이동 뒤 겹침 갱신으로 BeginOverlap이 온다. 탄이 느려(600 cm/s) 한 프레임에 대장을 건너뛰지 않는다
	AddActorWorldOffset(Velocity * DeltaSeconds);
}

void AUSEnemyProjectile::HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep)
{
	if (bHit || OtherActor == nullptr || OtherActor == GetOwner())
	{
		return;
	}

	const UUSTeamSubsystem* Team = GetWorld() ? GetWorld()->GetSubsystem<UUSTeamSubsystem>() : nullptr;
	if (Team == nullptr || !Team->GetAllies().Contains(OtherActor))
	{
		return;
	}

	bHit = true;
	UGameplayStatics::ApplyDamage(OtherActor, Damage, GetInstigatorController(), this, UDamageType::StaticClass());
	Destroy();
}
