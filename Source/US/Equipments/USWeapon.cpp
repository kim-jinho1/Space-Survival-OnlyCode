#include "Equipments/USWeapon.h"
#include "US.h"
#include "Agents/USAgent.h"
#include "USGameplayTags.h"
#include "Components/USCombatComponent.h"
#include "Components/USAttributeComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Controller.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "Equipments/USBullet.h"
#include "Core/USTeamSubsystem.h"
#include "Core/USBulletPoolSubsystem.h"
#include "Core/USChainExplosionSubsystem.h"
#include "Components/USRingPulsePoolComponent.h"
#include "Agents/USEnemyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"

AUSWeapon::AUSWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// 에디터 미리보기용 초기 스케일. 실제 크기는 장착 시 ApplyWeaponScale이 절대 스케일로 다시 정한다.
	if (Mesh)
	{
		Mesh->SetRelativeScale3D(FVector(WeaponScale));
	}

	// 기본 코스메틱 총알 (BP에서 교체 가능)
	BulletClass = AUSBullet::StaticClass();

	// 판은 처음 터질 때 만들어지므로 폭발탄이 없는 무기(부하·적)는 비용이 없다
	ExplosiveRing = CreateDefaultSubobject<UUSRingPulsePoolComponent>(TEXT("ExplosiveRing"));
	ExplosiveRing->SetupAttachment(GetRootComponent());

	// 섬광 판도 같은 풀을 쓴다 (재질만 다름). 재질을 안 넣은 무기(적 무기 등)는 그리지 않는다
	MuzzleFlashPool = CreateDefaultSubobject<UUSRingPulsePoolComponent>(TEXT("MuzzleFlashPool"));
	MuzzleFlashPool->SetupAttachment(GetRootComponent());
	ImpactFlashPool = CreateDefaultSubobject<UUSRingPulsePoolComponent>(TEXT("ImpactFlashPool"));
	ImpactFlashPool->SetupAttachment(GetRootComponent());
}

void AUSWeapon::EquipItem()
{
	Super::EquipItem();

	// 소유자의 컴포넌트를 장착 시점에 한 번만 캐싱 (매 발사마다 재검색하지 않도록)
	AttributeComponent = GetOwner()->GetComponentByClass<UUSAttributeComponent>();

	CombatComponent = GetOwner()->GetComponentByClass<UUSCombatComponent>();
	if (CombatComponent)
	{
		CombatComponent->SetMainWeapon(this);
		AttachToOwner(EquipSocketName);
		ApplyWeaponScale();

		if (bHideMeshWhenEquipped && Mesh)
		{
			Mesh->SetVisibility(false);
		}
	}
}

void AUSWeapon::ApplyWeaponScale()
{
	if (Mesh == nullptr)
	{
		return;
	}

	// 소켓이 붙은 집게 본(Arm_4_R)에 애니메이션 스케일(약 4배)이 걸려 있어 상대 스케일로 붙이면 크기가 튄다.
	// 절대 스케일로 전환하고 "캐릭터 메시 스케일 × WeaponScale"을 월드 스케일로 직접 지정한다.
	// 생성자는 C++ 기본 WeaponScale로 실행되므로 BP 오버라이드 값도 여기서야 반영된다.
	float OwnerMeshScale = 1.0f;
	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		if (const USkeletalMeshComponent* OwnerMesh = OwnerCharacter->GetMesh())
		{
			OwnerMeshScale = OwnerMesh->GetComponentScale().GetAbsMax();
		}
	}

	Mesh->SetUsingAbsoluteScale(true);
	Mesh->SetWorldScale3D(FVector(WeaponScale * OwnerMeshScale));
}

float AUSWeapon::GetAttackDamage(const FGameplayTag InAttackType) const
{
	float Damage = BaseDamage;

	if (InAttackType == USGameplayTags::Character_Attack_Rapid)
	{
		Damage *= RapidDamageScale;
	}

	// 소유자의 공격력 배율을 반영 (무기 교체에도 강화 유지). 컴포넌트는 EquipItem에서 캐싱됨.
	if (AttributeComponent)
	{
		Damage *= AttributeComponent->GetAttackDamageMultiplier();
	}

	return Damage;
}

void AUSWeapon::Fire(const FVector& AimLocation, FGameplayTag InAttackType)
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	CurrentAttackType = InAttackType;

	const FVector MuzzleLoc = GetMuzzleLocation();
	// 발사 위치를 고른 번호와 같은 번호로 플래시를 골라야 쏜 포구와 짝이 맞는다
	const int32 ShotIndex = ShotCount;
	++ShotCount;

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, MuzzleLoc);
	}

	// 쿼터뷰: 카메라가 아니라 머즐에서 출발해 조준점 방향으로 수평 트레이스한다.
	// Z를 버리는 이유는 조준점이 캐릭터 몸 높이 수평면 위에 있어 머즐 높이와 약간 다르기 때문 — 총알은 항상 수평.
	// 방향은 포구가 아니라 소유자 중심에서 잰다. 포구마다 재면 좌우 포구 탄이 조준점 한 점으로 모여 V자가 되고,
	// 중심에서 한 번 재면 모든 포구가 같은 방향으로 평행하게 쏜다.
	FVector FireDirection = AimLocation - OwnerActor->GetActorLocation();
	FireDirection.Z = 0.0f;
	if (FireDirection.IsNearlyZero())
	{
		// 조준점이 소유자 바로 위 — 소유자 전방으로 폴백
		FireDirection = OwnerActor->GetActorForwardVector();
		FireDirection.Z = 0.0f;
	}
	FireDirection = FireDirection.GetSafeNormal();
	const FRotator ViewRotation = FireDirection.Rotation();

	SpawnMuzzleFlash(ShotIndex, MuzzleLoc, ViewRotation);
	if (MuzzleFlashPool && MuzzleFlashPool->HasMaterial())
	{
		MuzzleFlashPool->PlayPulse(MuzzleLoc, MuzzleFlashRadius);
	}

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(OwnerActor);
	Params.bTraceComplex = true;

	// 아군 오사 방지: 대장·부하 전원을 트레이스에서 제외한다 (최대 9명이라 비용 무시 가능)
	if (const UUSTeamSubsystem* Team = GetWorld()->GetSubsystem<UUSTeamSubsystem>())
	{
		for (AActor* Ally : Team->GetAllies())
		{
			Params.AddIgnoredActor(Ally);
		}
	}

	// 확산 사격: 발 수만큼 부채꼴로 고르게 나눠 쏜다. 발사음·머즐 플래시는 위에서 한 번만 (한 번의 사격이므로)
	const int32 PelletCount = FMath::Max(1, SpreadPelletCount);
	const float PelletDamageScale = PelletCount > 1 ? SpreadDamageScale : 1.0f;
	const float HalfSpread = SpreadAngle * 0.5f;

	for (int32 Pellet = 0; Pellet < PelletCount; ++Pellet)
	{
		const float YawOffset = PelletCount > 1
			? FMath::Lerp(-HalfSpread, HalfSpread, static_cast<float>(Pellet) / static_cast<float>(PelletCount - 1))
			: 0.0f;
		const FVector PelletDirection = FireDirection.RotateAngleAxis(YawOffset, FVector::UpVector);
		FireShot(MuzzleLoc, PelletDirection, Params, PelletDamageScale);
	}
}

void AUSWeapon::SetPierceCount(int32 Count)
{
	PierceCount = FMath::Max(0, Count);
}

void AUSWeapon::SetSpread(int32 PelletCount, float PelletDamageScale)
{
	SpreadPelletCount = FMath::Max(1, PelletCount);
	SpreadDamageScale = FMath::Max(0.0f, PelletDamageScale);
}

void AUSWeapon::SetExplosiveRounds(float Chance, float DamageRatio)
{
	ExplosiveChance = FMath::Clamp(Chance, 0.0f, 1.0f);
	ExplosiveDamageRatio = FMath::Max(0.0f, DamageRatio);
}

void AUSWeapon::FireShot(const FVector& MuzzleLoc, const FVector& Direction, const FCollisionQueryParams& BaseParams, float DamageScale)
{
	FCollisionQueryParams ShotParams = BaseParams;
	FVector TraceStart = MuzzleLoc;
	const FVector TraceEnd = MuzzleLoc + Direction * FireRange;

	// LineTraceMulti는 첫 "막는(Block)" 충돌에서 멈추고 적은 Visibility를 막으므로 관통에 못 쓴다.
	// 대신 한 마리 맞을 때마다 그 적을 무시 목록에 넣고 명중 지점에서 다시 쏜다 (관통 최대 3이라 트레이스 4번)
	TArray<FHitResult, TInlineAllocator<4>> Hits;
	FVector BulletEnd = TraceEnd;
	float HitDamageScale = DamageScale;

	// 피해는 볼트가 닿는 순간(거리 / 탄속 뒤)에 준다. 판정 순간에 주면 데미지 숫자·번쩍임·사망이 볼트보다 먼저 보였다.
	// 판정(트레이스)은 지금 하므로 관통·확산 계산은 그대로다
	const AUSBullet* BulletDefaults = BulletClass ? BulletClass->GetDefaultObject<AUSBullet>() : nullptr;
	const float TravelSpeed = BulletDefaults ? BulletDefaults->Speed : 0.0f;
	const float BaseAttackDamage = GetAttackDamage(CurrentAttackType);

	for (int32 HitIndex = 0; HitIndex <= PierceCount; ++HitIndex)
	{
		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, ShotParams))
		{
			break;
		}

		BulletEnd = Hit.ImpactPoint;
		AActor* HitActor = Hit.GetActor();
		if (HitActor == nullptr)
		{
			break;
		}

		Hits.Add(Hit);
		const float Delay = TravelSpeed > 0.0f ? FVector::Dist(MuzzleLoc, Hit.ImpactPoint) / TravelSpeed : 0.0f;
		ScheduleHit(Hit, BaseAttackDamage * HitDamageScale, Direction, Delay);

		// 벽·바닥은 뚫지 않는다
		if (Cast<APawn>(HitActor) == nullptr)
		{
			break;
		}

		HitDamageScale *= PierceDamageFalloff;
		ShotParams.AddIgnoredActor(HitActor);
		TraceStart = Hit.ImpactPoint;
	}

	// 마지막 명중이 총알이 멈춘 자리라면 그 연출만 총알 도착 때 재생한다.
	// 명중 연출(이펙트·사운드)을 판정 순간에 터뜨리면 총알이 아직 날아가는 중인데 표적에서 불꽃이 먼저 튀기 때문
	const bool bBulletStopsAtLastHit = Hits.Num() > 0 && Hits.Last().ImpactPoint.Equals(BulletEnd);
	const int32 NumImmediate = bBulletStopsAtLastHit ? Hits.Num() - 1 : Hits.Num();
	for (int32 Index = 0; Index < NumImmediate; ++Index)
	{
		PlayImpactNow(Hits[Index]);
	}

	// 코스메틱 총알(트레이서) 발사 — 데미지는 위 히트스캔이 이미 처리함.
	// 발사마다 생성·파괴하지 않도록 월드 총알 풀에서 빌린다 (도착하면 총알이 스스로 반납).
	UUSBulletPoolSubsystem* BulletPool = GetWorld()->GetSubsystem<UUSBulletPoolSubsystem>();
	if (BulletClass && BulletPool == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: no bullet pool subsystem; firing without a cosmetic bullet"), *GetName());
	}

	bool bImpactDeferred = false;
	if (BulletClass && BulletPool)
	{
		if (AUSBullet* Bullet = BulletPool->AcquireBullet(BulletClass, MuzzleLoc, Direction.Rotation(), this, Cast<APawn>(GetOwner())))
		{
			Bullet->Launch(MuzzleLoc, BulletEnd);
			if (bBulletStopsAtLastHit)
			{
				const FHitResult& LastHit = Hits.Last();
				Bullet->SetImpact(GetImpactFX(LastHit), ImpactSound.Get(), LastHit.ImpactNormal.Rotation());
			}
			else
			{
				Bullet->SetImpact(nullptr, nullptr, FRotator::ZeroRotator);
			}
			bImpactDeferred = true;
		}
	}

	// 총알이 없으면(BulletClass 비움·스폰 실패) 기다릴 대상이 없으니 바로 재생
	if (bBulletStopsAtLastHit && !bImpactDeferred)
	{
		PlayImpactNow(Hits.Last());
	}
}

void AUSWeapon::PlayImpactNow(const FHitResult& Hit) const
{
	if (UParticleSystem* ImpactFX = GetImpactFX(Hit))
	{
		UGameplayStatics::SpawnEmitterAtLocation(this, ImpactFX, Hit.ImpactPoint, Hit.ImpactNormal.Rotation(), FVector(1.0f),
			true, EPSCPoolMethod::AutoRelease);
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Hit.ImpactPoint);
	}
}

void AUSWeapon::SpawnMuzzleFlash(int32 ShotIndex, const FVector& Location, const FRotator& Rotation)
{
	// 이펙트를 안 쓰는 무기(적 무기 등)도 정상 설정이라 경고 없이 넘어간다
	if (MuzzleFlashFX.IsEmpty())
	{
		return;
	}

	UParticleSystem* FlashFX = MuzzleFlashFX[ShotIndex % MuzzleFlashFX.Num()];
	if (FlashFX == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: MuzzleFlashFX[%d] is empty"), *GetName(), ShotIndex % MuzzleFlashFX.Num());
		return;
	}

	// 무기에 붙여야 플래시가 떠 있는 동안 기체가 움직이고 기울어도 포구를 따라간다.
	// KeepWorldPosition은 지정한 월드 위치·회전·스케일(1)을 그대로 유지하므로 무기 메시의 절대 스케일을 물려받지 않는다.
	// 발사마다 생기는 일회성 이펙트라 월드 파티클 풀에서 빌리고, 재생이 끝나면 자동 반납(AutoRelease)한다.
	UGameplayStatics::SpawnEmitterAttached(FlashFX, GetRootComponent(), NAME_None, Location, Rotation,
		FVector(1.0f), EAttachLocation::KeepWorldPosition, true, EPSCPoolMethod::AutoRelease);
}

UParticleSystem* AUSWeapon::GetImpactFX(const FHitResult& Hit) const
{
	return Cast<APawn>(Hit.GetActor()) ? HitCharacterFX.Get() : HitWorldFX.Get();
}

FVector AUSWeapon::GetMuzzleLocation() const
{
	// 숨긴 모델의 소켓에 기대면 모델 크기에 따라 발사점이 달라진다. 붙은 지점을 그대로 발사점으로 쓴다.
	if (bHideMeshWhenEquipped)
	{
		// 기체에 포구가 여러 개면 소유자가 이번 발의 포구를 알려 준다 (좌우 번갈아 발사)
		FVector OwnerMuzzle;
		const AUSAgent* OwnerAgent = Cast<AUSAgent>(GetOwner());
		if (OwnerAgent && OwnerAgent->GetMuzzleWorldLocation(ShotCount, OwnerMuzzle))
		{
			return OwnerMuzzle;
		}
		return GetActorLocation();
	}

	if (Mesh && Mesh->DoesSocketExist(MuzzleSocketName))
	{
		return Mesh->GetSocketLocation(MuzzleSocketName);
	}

	return Mesh ? Mesh->GetComponentLocation() : GetActorLocation();
}

void AUSWeapon::OnHitActor(const FHitResult& Hit, float DamageScale)
{
	ApplyHit(Hit, GetAttackDamage(CurrentAttackType) * DamageScale, GetOwner()->GetActorForwardVector());
}

void AUSWeapon::ScheduleHit(const FHitResult& Hit, float AttackDamage, const FVector& Direction, float Delay)
{
	UWorld* World = GetWorld();
	if (World == nullptr || Delay <= KINDA_SMALL_NUMBER)
	{
		ApplyHit(Hit, AttackDamage, Direction);
		return;
	}

	TWeakObjectPtr<AUSEnemyCharacter> Enemy = Cast<AUSEnemyCharacter>(Hit.GetActor());
	if (Enemy.IsValid())
	{
		Enemy->AddPendingDamage(AttackDamage);
	}

	// 무기(소유자 사망 등)나 적이 그사이 사라지면 아무것도 하지 않는다. 핸들은 한 번 쓰고 버린다 (같은 무기에 여러 발이 겹쳐 날아간다)
	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Hit, AttackDamage, Direction, Enemy]()
	{
		if (Enemy.IsValid())
		{
			Enemy->AddPendingDamage(-AttackDamage);
		}
		if (IsValid(Hit.GetActor()) && GetOwner())
		{
			ApplyHit(Hit, AttackDamage, Direction);
		}
	}), Delay, false);
}

void AUSWeapon::ApplyHit(const FHitResult& Hit, float AttackDamage, const FVector& Direction)
{
	AActor* TargetActor = Hit.GetActor();

	UGameplayStatics::ApplyPointDamage(
		TargetActor,
		AttackDamage,
		Direction,
		Hit,
		GetOwner()->GetInstigatorController(),
		this,
		nullptr);

	// 넉백: 살짝 밀려 "맞았다"가 몸으로 보이게. Z 없는 순간 속도라 걷기 상태 그대로 — 마찰이 금방 멈춘다
	AUSEnemyCharacter* Enemy = Cast<AUSEnemyCharacter>(TargetActor);
	if (Enemy && Enemy->IsAlive() && KnockbackSpeed > 0.0f)
	{
		if (UCharacterMovementComponent* Movement = Enemy->GetCharacterMovement())
		{
			Movement->AddImpulse(Direction.GetSafeNormal2D() * KnockbackSpeed, true);
		}
	}

	if (ImpactFlashPool && ImpactFlashPool->HasMaterial())
	{
		ImpactFlashPool->PlayPulse(Hit.ImpactPoint, ImpactFlashRadius);
	}

	TryExplosiveRound(Hit, AttackDamage);

	OnTargetHit.Broadcast(TargetActor);
}

void AUSWeapon::TryExplosiveRound(const FHitResult& Hit, float HitDamage) const
{
	if (ExplosiveChance <= 0.0f || FMath::FRand() >= ExplosiveChance)
	{
		return;
	}

	// 적에게 맞았을 때만 (벽·바닥 명중은 제외). 이 발에 이미 죽었어도 터진다 — 시체 자리에서 주변을 쓸어야 연쇄가 이어진다
	if (Cast<AUSEnemyCharacter>(Hit.GetActor()) == nullptr)
	{
		return;
	}

	// 범위 판정·이펙트·소리 동시 재생 제한은 연쇄 폭발과 같은 경로를 쓴다. 폭발로 죽은 적은 연쇄 확률을 다시 굴린다
	if (UUSChainExplosionSubsystem* Chain = GetWorld() ? GetWorld()->GetSubsystem<UUSChainExplosionSubsystem>() : nullptr)
	{
		Chain->QueueExplosion(Hit.ImpactPoint, HitDamage * ExplosiveDamageRatio, ExplosiveRadius, ExplosiveFXScale, 0.0f);

		if (ExplosiveRing)
		{
			ExplosiveRing->PlayPulse(Hit.ImpactPoint, Chain->ScaleRadius(ExplosiveRadius));
		}
	}
}
