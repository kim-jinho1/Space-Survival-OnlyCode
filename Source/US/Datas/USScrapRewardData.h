#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "USScrapRewardData.generated.h"

/** 런이 어떻게 끝났나 */
UENUM(BlueprintType)
enum class EUSRunEndReason : uint8
{
	/** 대장 격추 */
	Destroyed,
	/** 클리어 후 "나가기" 또는 출격 시간 끝 */
	Returned,
	/** 일시정지 창 "출격 포기" (스크랩 공식은 격추와 같다) */
	Abandoned
};

/** 한 런의 결산. 결산 화면이 그대로 그리고, D5에서 스크랩 저장에도 쓴다 */
USTRUCT(BlueprintType)
struct FUSRunResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	EUSRunEndReason EndReason = EUSRunEndReason::Destroyed;

	/** 목표 시간을 버텼는가 (계속 가기 중 격추돼도 true) */
	UPROPERTY(BlueprintReadOnly, Category = "Result")
	bool bCleared = false;

	/** 이번 출격에서 깬 스테이지 수 (스테이지 판, 2026-10-26). 0이면 스테이지 없는 판 */
	UPROPERTY(BlueprintReadOnly, Category = "Result")
	int32 StagesCleared = 0;

	/** 이번 출격 스테이지 번호 (1부터, 스테이지 없는 판은 0). 스테이지 클리어 보너스가 번호를 따라 커진다 */
	UPROPERTY(BlueprintReadOnly, Category = "Result")
	int32 StageNumber = 0;

	/** 행성(보스 스테이지)을 깼는가 — 스테이지 판에서 ClearBonus는 이때만 */
	UPROPERTY(BlueprintReadOnly, Category = "Result")
	bool bPlanetCleared = false;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	int32 KillCount = 0;

	/** 생존 시간(초). 일시정지 시간은 빠진다 */
	UPROPERTY(BlueprintReadOnly, Category = "Result")
	float SurvivedSeconds = 0.0f;

	/** 계속 가기로 더 버틴 시간(초) */
	UPROPERTY(BlueprintReadOnly, Category = "Result")
	float ContinueSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	int32 Level = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	int32 KillScrap = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	int32 TimeScrap = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	int32 ClearScrap = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	int32 ContinueScrap = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Result")
	int32 TotalScrap = 0;
};

/**
 * 결산 스크랩 공식. 스크랩 = 처치 × ScrapPerKill + 생존 분 × ScrapPerMinute + 클리어 보너스 + 계속 가기 구간마다 보너스.
 * 스테이지 판의 클리어 보너스 = 깬 스테이지 × StageClearBonus × (1 + StageClearBonusGrowth × (번호 - 1)) + (행성 클리어면 ClearBonus).
 * 계속 가기 중 격추돼도 페널티는 없다 (데모 결정 2026-09-30). 값은 D6 밸런스에서 조정한다.
 */
UCLASS(BlueprintType)
class US_API UUSScrapRewardData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * 런 기록으로 스크랩을 계산해 결과를 채운다 (EndReason·bCleared·KillCount·SurvivedSeconds·ContinueSeconds가 채워진 상태로 넘긴다).
	 * Multiplier(영구 업그레이드 스크랩 획득량)는 줄마다 곱해 결산 화면의 줄 합이 합계와 맞게 한다
	 */
	void CalculateScrap(FUSRunResult& InOutResult, float Multiplier = 1.0f) const;

	FORCEINLINE int32 GetClearBonus() const { return ClearBonus; }
	FORCEINLINE int32 GetContinueBonusPerStep() const { return ContinueBonusPerStep; }
	FORCEINLINE float GetContinueStepSeconds() const { return ContinueStepSeconds; }

protected:
	/** 처치 1마다 스크랩 */
	UPROPERTY(EditDefaultsOnly, Category = "Scrap", meta = (ClampMin = "0.0"))
	float ScrapPerKill = 1.0f;

	/** 생존 1분마다 스크랩 (초 단위로 비례, 내림) */
	UPROPERTY(EditDefaultsOnly, Category = "Scrap", meta = (ClampMin = "0.0"))
	float ScrapPerMinute = 10.0f;

	/** 목표 시간을 버티면 받는 보너스. 스테이지 판에서는 행성(보스) 클리어 보너스 */
	UPROPERTY(EditDefaultsOnly, Category = "Scrap", meta = (ClampMin = "0"))
	int32 ClearBonus = 100;

	/** 스테이지 하나를 깰 때마다 받는 보너스 (스테이지 판). 행성 보너스를 스테이지마다 주면 트리가 너무 빨리 찬다 */
	UPROPERTY(EditDefaultsOnly, Category = "Scrap", meta = (ClampMin = "0"))
	int32 StageClearBonus = 60;

	/**
	 * 스테이지 번호가 하나 오를 때마다 StageClearBonus에 더하는 비율 (0.25 = 번호당 +25%). 판마다 1:30 고정이라 이게 없으면
	 * 어느 스테이지를 깨도 결산이 거의 같다 (빌드 테스트 2026-11-01, Hades·Slay the Spire처럼 뒤 지역일수록 보상이 크게)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Scrap", meta = (ClampMin = "0.0"))
	float StageClearBonusGrowth = 0.25f;

	/** 계속 가기로 ContinueStepSeconds를 더 버틸 때마다 받는 보너스 */
	UPROPERTY(EditDefaultsOnly, Category = "Scrap", meta = (ClampMin = "0"))
	int32 ContinueBonusPerStep = 25;

	/** 계속 가기 보너스 구간 길이(초) */
	UPROPERTY(EditDefaultsOnly, Category = "Scrap", meta = (ClampMin = "1.0"))
	float ContinueStepSeconds = 30.0f;
};
