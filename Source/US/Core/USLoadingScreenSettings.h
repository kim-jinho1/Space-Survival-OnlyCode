#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "USLoadingScreenSettings.generated.h"

class UFont;
class UParticleSystem;

/**
 * 맵 전환(페이드·로딩 화면)과 이펙트 미리 틀기 설정 (프로젝트 설정 → Game → US Loading Screen, 값은 DefaultGame.ini).
 * UUSLevelTravelSubsystem이 읽는다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "US Loading Screen"))
class US_API UUSLoadingScreenSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UUSLoadingScreenSettings();

	/** 떠나기 전 검게 어두워지는 시간(초, 실제 시간) */
	UPROPERTY(Config, EditAnywhere, Category = "Fade", meta = (ClampMin = "0.0"))
	float FadeOutSeconds = 0.35f;

	/** 새 맵에서 검은 화면을 유지하는 시간(초). 이 동안 이펙트를 미리 틀어 셰이더 상태(PSO)를 준비한다 */
	UPROPERTY(Config, EditAnywhere, Category = "Fade", meta = (ClampMin = "0.0"))
	float HoldSeconds = 0.6f;

	/** 새 맵에서 밝아지는 시간(초) */
	UPROPERTY(Config, EditAnywhere, Category = "Fade", meta = (ClampMin = "0.0"))
	float FadeInSeconds = 0.5f;

	/** 로딩 화면 글꼴 (엔진 기본 글꼴엔 한글이 없어 Pretendard) */
	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen")
	TSoftObjectPtr<UFont> Font;

	/** 로딩 화면이 최소한 떠 있는 시간(초). 로드가 아주 짧을 때 깜빡이지 않게 */
	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen", meta = (ClampMin = "0.0"))
	float MinimumDisplaySeconds = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Loading Screen")
	FText LoadingLabel;

	/** 로딩 화면 아래 팁 (매번 하나를 무작위로) */
	UPROPERTY(EditAnywhere, Category = "Loading Screen")
	TArray<FText> Tips;

	/**
	 * 새 맵이 검은 동안 대장 위치에서 한 번씩 틀 이펙트.
	 * UE5는 셰이더 상태(PSO)가 준비될 때까지 처음 보는 이펙트를 그리지 않거나 멈칫한다 → 화면이 가려진 동안 미리 써 둔다.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Warmup")
	TArray<TSoftObjectPtr<UParticleSystem>> WarmupEffects;
};
