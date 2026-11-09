#pragma once

#include "CoreMinimal.h"
#include "Agents/USEnemyCharacter.h"
#include "USBossEnemyCharacter.generated.h"

class AUSEnemySpawner;
class UUSBossData;
class UUSRingPulsePoolComponent;
class UWidgetComponent;

/** 보스 체력 변화 (현재, 최대) — 보스 체력바가 듣는다 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDelegateOnBossHealthChanged, float, float);

/**
 * 데모 1구역 보스 "모선" (출시 전 체크 R-36).
 * 일반 적처럼 대장을 쫓고 접촉 피해를 주며, 체력·외형·보상은 UUSEnemyTypeData를 그대로 쓴다.
 * 그 위에 패턴 타이머로 ① 방사 탄막(경고 고리 → 원형 탄)과 ② 졸개 사출(스포너의 바닥 X 예고 → 졸개)을 번갈아 쓴다.
 * 체력이 UUSBossData::PhaseTwoHealthRatio 아래면 2페이즈: 간격이 짧아지고 탄막이 두 겹이 된다.
 * 머리 위에 보급선 이름표와 같은 모양의 표시(MarkerWidget, WBP_USBossMarker: NameText + HealthBar)를 띄운다 — 사용자 요청.
 * 스포너(AUSEnemySpawner::StartBossPhase)가 스폰하고, 컨트롤러가 OnEnemyDeath로 격파를 듣는다.
 */
UCLASS()
class US_API AUSBossEnemyCharacter : public AUSEnemyCharacter
{
	GENERATED_BODY()

public:
	AUSBossEnemyCharacter();

	/** 졸개를 내보낼 스포너 (스폰 직후 스포너가 넣는다) */
	void SetSpawner(AUSEnemySpawner* InSpawner);

	/** 체력바용: 현재·최대 체력 */
	float GetCurrentHealth() const;
	float GetMaxHealth() const;

	FDelegateOnBossHealthChanged OnBossHealthChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 패턴을 멈추고 몸 주변에서 연쇄 폭발을 이어 터뜨린다 */
	virtual void HandleDeath() override;

	/** 다음 패턴을 고르고 다음 예약을 건다 */
	void RunNextPattern();

	/** ① 탄막: 경고 고리를 띄우고 BarrageTelegraphTime 뒤 FireRing */
	void StartBarrage();

	/** 고리 하나 발사. RingIndex가 홀수면 반 칸 돌려 앞 고리 틈을 메운다 */
	void FireRing(int32 RingIndex);

	/** ② 졸개 사출 (스포너가 바닥 X 예고 뒤 스폰) */
	void StartMinions();

	/** 체력 변화 → 2페이즈 판정 + 체력바 알림 */
	void HandleHealthChanged(float CurrentHealth, float MaxHealth);

	/** 격파 연쇄 폭발 한 번 (타이머) */
	void PlayDeathExplosionStep();

	/** 연쇄 폭발 마무리 큰 폭발음 (BossData->FinalDeathSound, 비어 있으면 아무것도 안 함) */
	void PlayFinalDeathSound() const;

	/** 대장과의 거리가 ArrivalDistance 안이면 도착 연출 한 번 (타이머, 도착 뒤 멈춘다) */
	void CheckArrival();

	/** 대장 화면을 Scale배로 흔든다 (설정 창 "화면 흔들림" 존중) */
	void PlayBossShake(float Scale) const;

	/** 패턴 수치 (DA_Boss_*) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	TObjectPtr<const UUSBossData> BossData;

	/** 머리 위 이름표 + 체력 막대 (화면 공간 위젯, 위젯 이름 NameText·HealthBar는 이름으로 찾는다) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> MarkerWidget;

	/** 이름표 높이 (캡슐 중심 기준 uu) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss", meta = (ClampMin = "0.0"))
	float MarkerHeight = 320.0f;

	/** 이름표 체력 막대를 지금 체력으로 */
	void RefreshMarker(float CurrentHealth, float MaxHealth) const;

	/** 탄막 경고 고리 (판 1장, 재질은 BP — 점선 고리) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSRingPulsePoolComponent> TelegraphPool;

private:
	TWeakObjectPtr<AUSEnemySpawner> Spawner;

	FTimerHandle PatternTimerHandle;
	FTimerHandle BarrageTimerHandle;
	FTimerHandle DeathExplosionTimerHandle;
	FTimerHandle ArrivalTimerHandle;

	/** 지금까지 쓴 패턴 수 (졸개 사출 차례 계산) */
	int32 PatternCount = 0;

	/** 이번 탄막의 첫 고리 각도 (라디안). 두 번째 고리가 반 칸 돌리는 기준 */
	float BarrageBaseAngle = 0.0f;

	/** 이번 탄막에서 남은 고리 수 */
	int32 RingsRemaining = 0;

	bool bPhaseTwo = false;

	/** 남은 격파 폭발 수 */
	int32 DeathExplosionsRemaining = 0;
};
