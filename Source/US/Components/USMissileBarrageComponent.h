#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USMissileBarrageComponent.generated.h"

class AUSMissile;
class UUSRingPulsePoolComponent;
class UParticleSystem;
class USoundBase;

/**
 * 감속 미사일 (대장 보조 능력, D6): Interval마다 가까운 적 TargetCount곳에 미사일(MissileClass)을 차례로 쏘고, 도착하면 폭발시킨다.
 * 폭발 반경 안의 적 전원을 SlowDuration초 동안 SlowMultiplier배로 늦추고, 약한 피해(대장 주무기 한 발 × DamageMultiplier)를 준다.
 * 대장 화력은 판 내내 고정이라 피해로는 후반 의미가 없다 — 역할은 몰려오는 적을 늦춰 편대가 쏠 시간을 버는 것.
 * 능력을 배우기 전(TargetCount 0)에는 타이머를 걸지 않는다. 적 목록은 UUSTeamSubsystem에서 거리로 거른다 (물리 오버랩 없음).
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class US_API UUSMissileBarrageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUSMissileBarrageComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 능력 단계를 적용한다 (표적 수, 감속 시간). 처음 배우면 주기 타이머를 건다 */
	void SetBarrage(int32 InTargetCount, float InSlowDuration);

	/** 감속 시간에 더할 초 (제압 빌드 완성 보너스). 능력 단계가 SlowDuration을 다시 정해도 따로 남는다 */
	void SetSlowDurationBonus(float Bonus);

	/** 폭발 자리에 감속 범위(ExplosionRadius) 고리를 그릴 풀. 소유자(대장)가 BeginPlay에서 넘긴다 */
	void SetSlowRing(UUSRingPulsePoolComponent* InSlowRing) { SlowRing = InSlowRing; }

protected:
	/** 표적을 골라 폭발을 차례로 예약한다 (주기 타이머) */
	void FireBarrage();

	/** 미사일 한 발을 Target으로 쏜다. 도착하면 Explode. 미사일 클래스가 없으면 바로 폭발 */
	void LaunchMissile(const FVector& Target, float Damage);

	/** Location에 폭발 연출 + 반경 감속·피해 */
	void Explode(const FVector& Location, float Damage);

	/** 소유자 사망 시 정지 */
	void HandleOwnerDeath();

	/** 일제 사격 간격(초) */
	UPROPERTY(EditDefaultsOnly, Category = "Missile", meta = (ClampMin = "0.5"))
	float Interval = 6.0f;

	/** 이 거리 안의 적만 노린다 (cm). 화면 밖 적을 치지 않게 */
	UPROPERTY(EditDefaultsOnly, Category = "Missile", meta = (ClampMin = "100.0"))
	float MaxRange = 1500.0f;

	/** 폭발 반경 (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "Missile", meta = (ClampMin = "10.0"))
	float ExplosionRadius = 200.0f;

	/** 폭발 피해 = 대장 주무기 한 발 × 이 배율. 보조 능력이라 약하게 (0이면 감속만) */
	UPROPERTY(EditDefaultsOnly, Category = "Missile", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.0f;

	/** 맞은 적의 이동 속도 배율 (0.5 = 절반 속도). 시간은 능력 단계가 정한다 */
	UPROPERTY(EditDefaultsOnly, Category = "Missile", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SlowMultiplier = 0.5f;

	/** 날아갈 미사일 (BP_USMissile: 몸체 메시·꼬리 연기). 비우면 Warning 후 날아가는 것 없이 바로 폭발 */
	UPROPERTY(EditDefaultsOnly, Category = "Missile")
	TSubclassOf<AUSMissile> MissileClass;

	/** 미사일 출발 높이 (대장 위치에서 위로, cm) */
	UPROPERTY(EditDefaultsOnly, Category = "Missile", meta = (ClampMin = "0.0"))
	float LaunchHeight = 80.0f;

	/** 발사 사이 간격(초). 한꺼번에 나가지 않고 "다다닥" 이어지게 */
	UPROPERTY(EditDefaultsOnly, Category = "Missile", meta = (ClampMin = "0.0"))
	float StaggerDelay = 0.12f;

	/** 폭발 이펙트 (Cascade). 비어 있으면 Warning 후 피해만 준다 */
	UPROPERTY(EditDefaultsOnly, Category = "Missile|FX")
	TObjectPtr<UParticleSystem> ExplosionFX;

	UPROPERTY(EditDefaultsOnly, Category = "Missile|FX", meta = (ClampMin = "0.01"))
	float ExplosionFXScale = 0.5f;

	/** 폭발음 (선택) */
	UPROPERTY(EditDefaultsOnly, Category = "Missile|FX")
	TObjectPtr<USoundBase> ExplosionSound;

private:
	FTimerHandle BarrageTimerHandle;

	UPROPERTY()
	TObjectPtr<UUSRingPulsePoolComponent> SlowRing;

	/** 능력이 정한 표적 수 (0 = 안 배움) */
	int32 TargetCount = 0;

	/** 능력이 정한 감속 시간(초) */
	float SlowDuration = 0.0f;

	/** 빌드 보너스 감속 시간(초) */
	float SlowDurationBonus = 0.0f;
};
