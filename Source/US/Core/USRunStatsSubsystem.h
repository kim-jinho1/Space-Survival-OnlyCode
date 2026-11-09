#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "USRunStatsSubsystem.generated.h"

class UUSCombatFeedbackSubsystem;

/** 처치 수 변경 알림 (누적 처치 수) */
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateOnKillCountChanged, int32);

/** 편대 DPS 갱신 알림 (대장+부하 초당 피해, 그중 부하의 초당 피해). 비율은 HUD가 GetSafeRatio로 계산한다 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDelegateOnSquadDpsChanged, float, float);

/**
 * 한 판의 전투 통계(처치 수, 편대 DPS). 전투 중계소(UUSCombatFeedbackSubsystem)의 보고를 받아 모은다.
 * HUD가 커지는 숫자를 보여 주고, 결산 화면(D4)이 처치 수를 스크랩으로 바꿀 때 쓴다.
 */
UCLASS()
class US_API UUSRunStatsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	FORCEINLINE int32 GetKillCount() const { return KillCount; }
	FORCEINLINE float GetSquadDps() const { return SquadDps; }
	FORCEINLINE float GetAllyDps() const { return AllyDps; }

	FDelegateOnKillCountChanged OnKillCountChanged;
	FDelegateOnSquadDpsChanged OnSquadDpsChanged;

protected:
	/** 게임 월드에서만 만든다 (에디터 미리보기 월드 제외) */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void HandleEnemyDamaged(const FVector& Location, float Damage, bool bByPlayer);
	void HandleEnemyKilled();

	/** 1초 칸을 지금 시각까지 밀고, 지나간 칸은 비운다 */
	void AdvanceBuckets(float Now);

	/** 최근 DpsWindowSeconds초의 피해로 DPS를 다시 계산해 알린다 (타이머) */
	void RefreshDps();

	/** DPS를 평균 내는 구간(초) = 칸 수 */
	static constexpr int32 DpsWindowSeconds = 5;

	/** HUD에 DPS를 알리는 간격(초). 매 프레임 알리지 않는다 */
	static constexpr float DpsRefreshInterval = 0.5f;

	/** 초 단위 칸별 피해 (원형 버퍼, 월드 시간 초 % 칸 수) */
	float TotalDamageBuckets[DpsWindowSeconds] = {};
	float AllyDamageBuckets[DpsWindowSeconds] = {};

	/** 마지막으로 채운 칸의 월드 시간(초, 내림) */
	int32 CurrentBucketSecond = 0;

	int32 KillCount = 0;
	float SquadDps = 0.0f;
	float AllyDps = 0.0f;

	FTimerHandle DpsTimerHandle;

	UPROPERTY()
	TObjectPtr<UUSCombatFeedbackSubsystem> CombatFeedback;
};
