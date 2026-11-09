#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "USFXBudgetSettings.generated.h"

class UParticleSystem;

/**
 * 동시 재생 상한을 넘은 폭발을 대신할 가벼운 이펙트 (프로젝트 설정 → Game → US FX Budget, 값은 DefaultGame.ini).
 * Niagara Effect Type의 Cull Proxy와 같은 개념 — 상한을 넘으면 생략하지 않고 불꽃 수를 줄인 판으로 터뜨린다.
 * UUSFXBudgetSubsystem이 읽는다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "US FX Budget"))
class US_API UUSFXBudgetSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 원본 이펙트 → 상한을 넘었을 때 대신 틀 가벼운 이펙트. 없는 원본은 상한을 넘으면 예전처럼 생략 */
	UPROPERTY(Config, EditAnywhere, Category = "Budget")
	TMap<TSoftObjectPtr<UParticleSystem>, TSoftObjectPtr<UParticleSystem>> CullProxies;
};
