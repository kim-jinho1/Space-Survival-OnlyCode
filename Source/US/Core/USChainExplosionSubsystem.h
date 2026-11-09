#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "USChainExplosionSubsystem.generated.h"

class UParticleSystem;
class USoundBase;

/** 연쇄 폭발 설정. 기본값은 컨트롤러 BP가 들고, 판 시작 때 격납고 강화를 더해 서브시스템에 넘긴다 */
USTRUCT(BlueprintType)
struct FUSChainExplosionSettings
{
	GENERATED_BODY()

	/** 적이 죽을 때 연쇄 폭발이 일어날 확률 (0 ~ 1) */
	UPROPERTY(EditAnywhere, Category = "ChainExplosion", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Chance = 0.1f;

	/** 폭발 반경 (평면 거리) */
	UPROPERTY(EditAnywhere, Category = "ChainExplosion", meta = (ClampMin = "0.0"))
	float Radius = 250.0f;

	/** 모든 폭발 반경 배율 (QueueExplosion을 거치는 폭발 + 감속 미사일). 판 시작 때 격납고 "폭발 범위" 칸으로 정해진다 */
	UPROPERTY(EditAnywhere, Category = "ChainExplosion", meta = (ClampMin = "0.1"))
	float RadiusMultiplier = 1.0f;

	/** 폭발 피해 = 죽은 적 최대 체력 × 이 값. 적 체력이 시간에 따라 커지므로 피해도 같이 커진다 */
	UPROPERTY(EditAnywhere, Category = "ChainExplosion", meta = (ClampMin = "0.0"))
	float DamageRatio = 1.0f;

	/** 죽은 뒤 터지기까지의 지연. 연쇄가 한꺼번에가 아니라 물결처럼 퍼져 보이게 한다 */
	UPROPERTY(EditAnywhere, Category = "ChainExplosion", meta = (ClampMin = "0.0"))
	float Delay = 0.1f;

	/** 한 번 처리할 때 터뜨리는 최대 개수. 넘치면 다음 처리로 미뤄 이펙트·피해 계산이 한 프레임에 몰리지 않게 한다 */
	UPROPERTY(EditAnywhere, Category = "ChainExplosion", meta = (ClampMin = "1"))
	int32 MaxExplosionsPerStep = 8;

	/** 폭발 이펙트 (Cascade). 비어 있으면 Warning 후 피해만 준다 */
	UPROPERTY(EditAnywhere, Category = "ChainExplosion|FX")
	TObjectPtr<UParticleSystem> FX;

	/** 폭발 이펙트 크기 배율 */
	UPROPERTY(EditAnywhere, Category = "ChainExplosion|FX", meta = (ClampMin = "0.01"))
	float FXScale = 0.15f;

	/** 폭발음 (선택). 동시 재생 제한은 사운드 애셋의 Concurrency로 건다 */
	UPROPERTY(EditAnywhere, Category = "ChainExplosion|FX")
	TObjectPtr<USoundBase> Sound;
};

/**
 * 적 연쇄 폭발. 적이 죽으면 확률로 그 자리에 폭발을 예약하고, 폭발에 죽은 적이 다시 확률을 굴려 연쇄가 이어진다.
 * 범위 판정은 등록부(UUSTeamSubsystem)의 적 목록 + 평면 거리라 물리 검사가 없다.
 * 기뢰처럼 반드시 터지는 적은 QueueExplosion으로 직접 예약한다.
 */
UCLASS()
class US_API UUSChainExplosionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 판 시작 때 컨트롤러가 기본값 + 격납고 강화를 넘긴다 */
	void Configure(const FUSChainExplosionSettings& InSettings);

	/** 적이 죽었을 때 부른다. 확률에 걸리면 Delay 뒤 폭발을 예약한다 */
	void TryChainExplosion(const FVector& Location, float SourceMaxHealth);

	/** 확률 없이 폭발을 예약한다. Radius에는 RadiusMultiplier가 곱해진다 (범위 고리는 ScaleRadius로 같은 값을 그린다). FXScaleMultiplier는 설정의 FXScale에 곱한다 */
	void QueueExplosion(const FVector& Location, float Damage, float Radius, float FXScaleMultiplier, float Delay);

	FORCEINLINE const FUSChainExplosionSettings& GetSettings() const { return Settings; }

	/** 격납고 폭발 범위 배율을 곱한 반경. 서브시스템 밖에서 판정하는 폭발(감속 미사일)과 범위 고리가 쓴다 */
	FORCEINLINE float ScaleRadius(float Radius) const { return Radius * Settings.RadiusMultiplier; }

private:
	struct FPendingExplosion
	{
		FVector Location = FVector::ZeroVector;
		float Damage = 0.0f;
		float Radius = 0.0f;
		float FXScaleMultiplier = 1.0f;
		double ExplodeTime = 0.0;
	};

	/** 시간이 된 폭발을 최대 MaxExplosionsPerStep개 터뜨린다 (타이머로 주기 호출, 대기열이 비면 멈춤) */
	void ProcessQueue();

	/** 이펙트·소리 재생 후 반경 안 살아 있는 적에게 피해 */
	void Explode(const FPendingExplosion& Explosion);

	UPROPERTY()
	FUSChainExplosionSettings Settings;

	TArray<FPendingExplosion> Pending;

	FTimerHandle ProcessTimerHandle;

	/** FX 미지정 경고를 판마다 한 번만 */
	bool bWarnedMissingFX = false;
};
