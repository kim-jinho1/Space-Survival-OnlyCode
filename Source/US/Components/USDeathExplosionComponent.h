#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USDeathExplosionComponent.generated.h"

class UParticleSystem;
class USoundBase;

/**
 * 사망 폭발 연출 컴포넌트. 지정한 위치에서 폭발 이펙트와 사망음을 한 번 재생한다.
 * 무엇을 숨기고 멈출지는 소유 액터가 안다 — 이 컴포넌트는 "터지는 것"만 맡는다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class US_API UUSDeathExplosionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUSDeathExplosionComponent();

	/** Location에서 폭발 이펙트와 사망음을 재생한다 */
	void PlayExplosion(const FVector& Location) const;

protected:
	/** 폭발 이펙트 (Cascade). 비어 있으면 Warning 후 소리만 낸다 */
	UPROPERTY(EditDefaultsOnly, Category = "Death")
	TObjectPtr<UParticleSystem> DeathFX;

	/** 폭발 크기 배율. 수류탄용 이펙트라 줄여 쓴다. 카메라 600 시절 0.2 → 2000으로 멀어져 0.6 (플레이테스트 2026-10-20) */
	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (ClampMin = "0.01"))
	float DeathFXScale = 0.6f;

	/** 사망음 (선택) */
	UPROPERTY(EditDefaultsOnly, Category = "Death")
	TObjectPtr<USoundBase> DeathSound;

	/** 사망음 음 높이 흔들기 폭 (0.1 = 0.9~1.1배) */
	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float DeathSoundPitchVariation = 0.1f;
};
