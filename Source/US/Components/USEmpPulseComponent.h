#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "USEmpPulseComponent.generated.h"

class UUSAttributeComponent;

/**
 * EMP 펄스 (EMP기): 일정 간격으로 반경 안의 적을 감속시키고, 퍼지는 고리를 그린다.
 * 간격은 소유자의 공격 속도 배율로 나눈다. "EMP 충격" 능력을 배우면 펄스가 피해도 준다 (소유자 공격력 배율 적용).
 * 이 컴포넌트 자체가 바닥에 깔린 판 메시다. 고리가 퍼지는 모양은 머티리얼이 "펄스 시각"(Custom Primitive Data)과
 * 현재 시간으로 계산하므로 C++은 펄스마다 시각만 적는다 — Tick·위젯·파티클 없음.
 * 적 목록은 UUSTeamSubsystem에서 거리로 거른다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class US_API UUSEmpPulseComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UUSEmpPulseComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 펄스 한 번의 기본 피해를 정한다 ("EMP 충격" 능력, 0 = 피해 없음) */
	void SetPulseDamage(float Damage);

	/** 펄스 반경과 감속 지속 시간을 바꾼다 ("광역 감속" 능력). 바닥 고리 크기도 맞춘다 */
	void SetPulseArea(float Radius, float Duration);

	/** 감속 시간에 더할 초 (제압 빌드 완성 보너스). 능력 단계가 SlowDuration을 다시 정해도 따로 남는다 */
	void SetSlowDurationBonus(float Bonus);

protected:
	/** 반경 안의 적 감속 + 고리 표시 (타이머) */
	void Pulse();

	/** 소유자 사망 시 정지 */
	void HandleOwnerDeath();

	/** 다음 펄스 타이머를 건다 (간격 = PulseInterval ÷ 공격 속도 배율) */
	void ScheduleNextPulse();

	/** 판 메시 지름을 PulseRadius × 2에 맞춘다 */
	void ApplyRadiusScale();

	/** 기본 펄스 간격(초, 공격 속도 배율 1 기준) */
	UPROPERTY(EditDefaultsOnly, Category = "EMP", meta = (ClampMin = "0.1"))
	float PulseInterval = 3.0f;

	/** 감속 반경 (cm). 판 메시(엔진 Plane 100uu)를 이 반경에 맞게 키운다 */
	UPROPERTY(EditDefaultsOnly, Category = "EMP", meta = (ClampMin = "10.0"))
	float PulseRadius = 450.0f;

	/** 감속 배율 (0.5 = 절반 속도) */
	UPROPERTY(EditDefaultsOnly, Category = "EMP", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SlowMultiplier = 0.5f;

	/** 감속 지속 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "EMP", meta = (ClampMin = "0.0"))
	float SlowDuration = 2.0f;

	/** 펄스 시각을 적을 Custom Primitive Data 인덱스. 머티리얼 파라미터의 Primitive Data Index와 같아야 한다 */
	UPROPERTY(EditDefaultsOnly, Category = "EMP")
	int32 PulseTimeDataIndex = 0;

private:
	FTimerHandle PulseTimerHandle;

	/** 능력이 정한 펄스 피해 (런타임 상태) */
	float PulseDamage = 0.0f;

	/** 빌드 보너스 감속 시간(초) */
	float SlowDurationBonus = 0.0f;

	/** 소유자 속성 (BeginPlay에서 한 번 캐싱. 공격 속도 배율을 읽는다) */
	UPROPERTY()
	TObjectPtr<UUSAttributeComponent> OwnerAttribute;
};
