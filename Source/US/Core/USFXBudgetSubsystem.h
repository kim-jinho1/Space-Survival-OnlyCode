#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/ObjectKey.h"
#include "USFXBudgetSubsystem.generated.h"

class UParticleSystem;
class UParticleSystemComponent;

/**
 * 폭발처럼 한꺼번에 몰리는 일회성 Cascade 이펙트의 동시 재생 개수를 이펙트별로 제한한다.
 * 무리를 한꺼번에 잡으면 격추 폭발이 게임 스레드 시간의 절반을 먹었다 (R-41 10 측정).
 * Niagara Effect Type의 MaxSystemInstances와 같은 개념 — Cascade에는 없어 직접 센다. 상한은 US.FX.MaxConcurrentPerSystem.
 * 상한을 넘으면 UUSFXBudgetSettings::CullProxies의 가벼운 판으로 대신 터뜨린다 (Niagara Cull Proxy와 같은 개념)
 */
UCLASS()
class US_API UUSFXBudgetSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * 상한 안이면 월드 파티클 풀에서 빌려 재생하고(끝나면 자동 반납, LOD 0 고정) 컴포넌트를 돌려준다.
	 * 같은 이펙트가 이미 상한만큼 재생 중이면 가벼운 판(있으면, 따로 센 상한 안에서)을 대신 틀고, 그것도 넘치면 nullptr
	 */
	UParticleSystemComponent* SpawnBudgetedEmitter(UParticleSystem* FX, const FVector& Location, const FVector& Scale);

private:
	/** 상한 검사 + 재생. Limit 개수만큼 이미 재생 중이면 nullptr */
	UParticleSystemComponent* SpawnWithinLimit(UParticleSystem* FX, const FVector& Location, const FVector& Scale, int32 Limit);

	/** FX의 가벼운 판 (설정에서 처음 찾을 때 한 번 로드해 둔다). 없으면 nullptr */
	UParticleSystem* FindCullProxy(UParticleSystem* FX);

	/** 원본 → 가벼운 판 (없으면 nullptr로 기억해 매번 설정을 뒤지지 않는다) */
	UPROPERTY(Transient)
	TMap<TObjectPtr<UParticleSystem>, TObjectPtr<UParticleSystem>> CullProxyCache;

	/** 이펙트별 재생 중인 컴포넌트. 풀에 반납되면 비활성이 되므로 다음 생성 때 걸러 낸다 (Tick 없음) */
	TMap<TObjectKey<UParticleSystem>, TArray<TWeakObjectPtr<UParticleSystemComponent>>> ActiveByFX;
};
