#pragma once

#include "CoreMinimal.h"
#include "Agents/USEnemyCharacter.h"
#include "USMineEnemyCharacter.generated.h"

/**
 * 부유 기뢰. 느리게 다가와 대상에 닿으면 주변 아군(대장·부하)에게 폭발 피해를 주고 자폭한다.
 * 자폭은 처치가 아니므로 보상 젬을 떨어뜨리지 않는다. 총에 맞아 격추되면 일반 적처럼 보상을 준다.
 * 어떻게 죽든 큰 연쇄 폭발로 주변 적을 쓸어 버린다 (폭발 드럼통 역할).
 */
UCLASS()
class US_API AUSMineEnemyCharacter : public AUSEnemyCharacter
{
	GENERATED_BODY()

public:
	AUSMineEnemyCharacter();

protected:
	/** 접촉 데미지 대신 자폭한다 */
	virtual void ApplyContactDamage() override;

	/** 자폭은 처치가 아니다 (보상·처치 수 없음) */
	virtual bool ShouldCountAsKill() const override;

	/** 연쇄 확률과 상관없이 반드시 큰 폭발을 예약한다 */
	virtual void RequestChainExplosion() override;

	/** 반경 안 아군에게 폭발 피해를 주고 스스로 죽는다 */
	void Detonate();

	/** 폭발 피해 */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Mine", meta = (ClampMin = "0.0"))
	float ExplosionDamage = 35.0f;

	/** 폭발 반경 (3D 거리) */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Mine", meta = (ClampMin = "0.0"))
	float ExplosionRadius = 400.0f;

	/** 죽을 때 주변 적을 쓸어 버리는 폭발 반경 (평면 거리). 일반 연쇄 폭발보다 크게 */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Mine", meta = (ClampMin = "0.0"))
	float BlastRadius = 800.0f;

	/** 적에게 주는 폭발 피해 = 기뢰 최대 체력 × 이 값 */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Mine", meta = (ClampMin = "0.0"))
	float BlastDamageRatio = 1.0f;

	/** 폭발 이펙트 크기 = 연쇄 폭발 이펙트 크기 × 이 값 */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Mine", meta = (ClampMin = "0.01"))
	float BlastFXScaleMultiplier = 4.0f;

	/**
	 * 큰 폭발 범위(BlastRadius)를 그대로 그리는 퍼지는 고리 (판 1장, 재질은 BP에서 — 폭발탄 범위 고리와 같은 재질).
	 * 파티클은 배율을 키워도 반경 800을 다 덮지 못해 범위가 안 보였다 (플레이테스트 2026-10-21)
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UUSRingPulsePoolComponent> BlastRing;

private:
	/** 자폭으로 죽었는가 (처치 판단) */
	bool bDetonated = false;
};
