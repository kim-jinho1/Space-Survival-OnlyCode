#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Equipments/USEquipment.h"
#include "USGameplayTags.h"
#include "USWeapon.generated.h"

class UParticleSystem;

/** 이 무기의 공격이 액터를 맞혔다는 알림 (맞은 액터). 표적 지정처럼 "맞힌 적에게 무언가 거는" 능력이 구독한다 */
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateOnTargetHit, AActor*);

UCLASS()
class US_API AUSWeapon : public AUSEquipment
{
	GENERATED_BODY()
	
public:	
	AUSWeapon();
public:
	virtual void EquipItem() override;

public:
	float GetAttackDamage(FGameplayTag InAttackType = FGameplayTag::EmptyTag) const;

public:
	/**
	 * 히트스캔 발사. 명중 시 데미지를 적용한다. 탄약 제한은 없다 (연사 속도는 소유자의 발사 간격이 정한다).
	 * 머즐에서 AimLocation 쪽으로 수평 트레이스한다 (쿼터뷰: 총알은 항상 수평).
	 */
	void Fire(const FVector& AimLocation, FGameplayTag InAttackType = FGameplayTag::EmptyTag);

	/** 관통탄 능력: 총알이 적을 Count마리 더 뚫는다 (0 = 관통 없음) */
	void SetPierceCount(int32 Count);

	/** 확산 사격 능력: 한 번에 PelletCount발을 부채꼴로 쏘고 한 발 피해는 PelletDamageScale배 (1발 = 확산 없음) */
	void SetSpread(int32 PelletCount, float PelletDamageScale);

	/** 폭발탄 능력: 적 명중마다 Chance 확률로 명중 지점에서 폭발 (피해 = 명중 피해 × DamageRatio). Chance 0 = 끔 */
	void SetExplosiveRounds(float Chance, float DamageRatio);

public:
	// 공격이 명중한 Actor에 대한 처리. DamageScale = 확산·관통으로 줄어든 피해 배율
	void OnHitActor(const FHitResult& Hit, float DamageScale = 1.0f);

	/** 명중 알림. 피해를 준 뒤 방송한다 (맞은 적이 이 발에 죽었을 수도 있다) */
	FDelegateOnTargetHit OnTargetHit;

protected:
	/** 머즐 소켓 위치 (없으면 메시 위치로 폴백) */
	FVector GetMuzzleLocation() const;

	/** 장착 후 무기 메시를 절대 스케일 "캐릭터 메시 스케일 × WeaponScale"로 맞춘다 (소켓 본 스케일 무시) */
	void ApplyWeaponScale();

	/** 이번 발의 포구(ShotIndex)에 맞는 머즐 플래시를 무기에 붙여 재생한다 */
	void SpawnMuzzleFlash(int32 ShotIndex, const FVector& Location, const FRotator& Rotation);

	/** 맞은 대상에 맞는 명중 이펙트 — 폰이면 HitCharacterFX, 아니면 HitWorldFX */
	UParticleSystem* GetImpactFX(const FHitResult& Hit) const;

	/**
	 * 한 발: Direction으로 수평 트레이스해 피해를 주고, 관통이 남아 있으면 맞은 적을 무시하고 그 자리에서 다시 트레이스한다.
	 * 코스메틱 총알은 마지막 명중 지점까지 날아가고, 그 앞에서 뚫린 적의 명중 연출은 바로 재생한다
	 */
	void FireShot(const FVector& MuzzleLoc, const FVector& Direction, const FCollisionQueryParams& BaseParams, float DamageScale);

	/** 명중 이펙트·소리를 지금 재생한다 (총알 도착을 기다리지 않는 경우) */
	void PlayImpactNow(const FHitResult& Hit) const;

	/** 폭발탄: 적에게 맞았으면 확률을 굴려 연쇄 폭발 서브시스템에 즉시 폭발을 예약한다 */
	void TryExplosiveRound(const FHitResult& Hit, float HitDamage) const;

	/** 명중 한 건 처리: 피해·넉백·폭발탄·명중 알림. Direction = 탄이 날아온 수평 방향 */
	void ApplyHit(const FHitResult& Hit, float AttackDamage, const FVector& Direction);

	/** 명중을 Delay초 뒤(볼트 도착)에 처리한다. 그동안 적의 "날아오는 피해"에 올려 둔다 (AUSEnemyCharacter::IsDoomed) */
	void ScheduleHit(const FHitResult& Hit, float AttackDamage, const FVector& Direction, float Delay);

public:
	/** 무기 메시 스케일 — 캐릭터 메시 스케일에 대한 배율. 장착 시 ApplyWeaponScale에서 적용 (소켓 본 스케일은 무시) */
	UPROPERTY(EditAnywhere, Category = "Weapon | Visual")
	float WeaponScale = 0.35f;

	/** 장착 시 무기 모델을 숨긴다 (드론처럼 기체가 직접 쏘는 경우). 켜면 발사 위치는 붙은 지점(WeaponMount) 그 자체 */
	UPROPERTY(EditAnywhere, Category = "Weapon | Visual")
	bool bHideMeshWhenEquipped = false;

	/** 발사 시 머즐→명중지점으로 날아가는 코스메틱 총알 클래스 */
	UPROPERTY(EditAnywhere, Category = "Weapon | Visual")
	TSubclassOf<class AUSBullet> BulletClass;

	/** 발사 머즐 플래시. 포구가 여러 개면 발사 순서대로 돌아가며 쓴다 (좌우 포구와 짝) */
	UPROPERTY(EditAnywhere, Category = "Weapon | FX")
	TArray<TObjectPtr<UParticleSystem>> MuzzleFlashFX;

	/** 캐릭터(폰) 명중 이펙트. 코스메틱 총알이 도착할 때 재생 */
	UPROPERTY(EditAnywhere, Category = "Weapon | FX")
	TObjectPtr<UParticleSystem> HitCharacterFX;

	/** 벽·바닥 명중 이펙트. 코스메틱 총알이 도착할 때 재생 */
	UPROPERTY(EditAnywhere, Category = "Weapon | FX")
	TObjectPtr<UParticleSystem> HitWorldFX;

	/** 발사 섬광 판 풀 (재질이 있을 때만 그린다 — 파티클 MuzzleFlashFX와 따로). 포구 자리에서 짧게 번쩍 */
	UPROPERTY(VisibleAnywhere, Category = "Weapon | FX")
	TObjectPtr<class UUSRingPulsePoolComponent> MuzzleFlashPool;

	/** 명중 섬광 판 풀 (재질이 있을 때만). 볼트가 닿는 순간 명중 지점에서 번쩍 */
	UPROPERTY(VisibleAnywhere, Category = "Weapon | FX")
	TObjectPtr<class UUSRingPulsePoolComponent> ImpactFlashPool;

	/** 발사 섬광 반경 (uu) */
	UPROPERTY(EditAnywhere, Category = "Weapon | FX", meta = (ClampMin = "0.0"))
	float MuzzleFlashRadius = 40.0f;

	/** 명중 섬광 반경 (uu) */
	UPROPERTY(EditAnywhere, Category = "Weapon | FX", meta = (ClampMin = "0.0"))
	float ImpactFlashRadius = 70.0f;

	/** 적을 맞히면 탄 방향으로 밀어내는 속도 (uu/s, 순간 속도 변화). 0 = 넉백 없음. 아군은 밀지 않는다 */
	UPROPERTY(EditAnywhere, Category = "Weapon | Fire", meta = (ClampMin = "0.0"))
	float KnockbackSpeed = 300.0f;

	/** 기본 공격력 */
	UPROPERTY(EditAnywhere)
	float BaseDamage = 15.0f;

	/** 연사(Rapid) 공격 시 데미지 배율 */
	UPROPERTY(EditAnywhere, Category = "Weapon | Fire")
	float RapidDamageScale = 0.6f;

	/** 사거리 */
	UPROPERTY(EditAnywhere, Category = "Weapon | Fire")
	float FireRange = 10000.0f;

	/** 머즐 소켓 이름 */
	UPROPERTY(EditAnywhere, Category = "Weapon | Socket")
	FName MuzzleSocketName = TEXT("Muzzle");

	/** 발사음 */
	UPROPERTY(EditAnywhere, Category = "Weapon | Sound")
	TObjectPtr<class USoundBase> FireSound;

	/** 명중(임팩트)음 */
	UPROPERTY(EditAnywhere, Category = "Weapon | Sound")
	TObjectPtr<class USoundBase> ImpactSound;

	/** 관통할 때마다 다음 적에게 곱해지는 피해 배율 (기획서: -20%) */
	UPROPERTY(EditAnywhere, Category = "Weapon | Ability", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PierceDamageFalloff = 0.8f;

	/** 확산 사격의 부채꼴 전체 각도 (도) */
	UPROPERTY(EditAnywhere, Category = "Weapon | Ability", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float SpreadAngle = 30.0f;

	/** 폭발탄 폭발 반경 (평면 거리). 연쇄 폭발(250)보다 작게 — 자주 터지는 대신 좁게 */
	UPROPERTY(EditAnywhere, Category = "Weapon | Ability", meta = (ClampMin = "0.0"))
	float ExplosiveRadius = 150.0f;

	/** 폭발탄 이펙트 크기 — 연쇄 폭발 이펙트 크기(FXScale)에 곱한다 */
	UPROPERTY(EditAnywhere, Category = "Weapon | Ability", meta = (ClampMin = "0.01"))
	float ExplosiveFXScale = 0.6f;

	/** 폭발탄 범위 고리. 폭발 이펙트가 적 사망 폭발과 같은 그림이라 이 고리(주황)로 폭발탄임을 구분한다 */
	UPROPERTY(VisibleAnywhere, Category = "Weapon | Ability")
	TObjectPtr<class UUSRingPulsePoolComponent> ExplosiveRing;

protected:
	/** 현재 발사 중인 공격 타입 (데미지 계산용) */
	FGameplayTag CurrentAttackType;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment | Socket")
	FName EquipSocketName = "WeaponSocket";

protected:
	UPROPERTY()
	TObjectPtr<class UUSCombatComponent> CombatComponent;

	/** 소유자의 속성 컴포넌트 (EquipItem 시점에 캐싱, 공격력 배율 조회에 사용) */
	UPROPERTY()
	TObjectPtr<class UUSAttributeComponent> AttributeComponent;

private:
	/** 발사 횟수. 소유자 포구가 여러 개일 때 번갈아 쏘는 인덱스로 쓴다 */
	int32 ShotCount = 0;

	/** 관통탄 능력 단계가 정한 추가 관통 수 (런타임 상태라 UPROPERTY 아님. 적 무기는 늘 0) */
	int32 PierceCount = 0;

	/** 확산 사격 능력이 정한 발 수 (1 = 확산 없음) */
	int32 SpreadPelletCount = 1;

	/** 확산 사격 한 발의 피해 배율 */
	float SpreadDamageScale = 1.0f;

	/** 폭발탄 능력이 정한 명중당 폭발 확률 (0 = 폭발탄 없음) */
	float ExplosiveChance = 0.0f;

	/** 폭발탄 피해 배율 (명중 피해 기준) */
	float ExplosiveDamageRatio = 1.0f;
};
