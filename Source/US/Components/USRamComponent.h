#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USRamComponent.generated.h"

class UUSAttributeComponent;

/**
 * 충각: 몸에 붙은 적에게 주기적으로 근접 데미지를 준다 (방패기).
 * 데미지는 소유자의 공격력 배율(격납고·보급선 강화)과 "철벽 돌진" 능력 배율을 따른다.
 * 적 목록은 UUSTeamSubsystem에서 거리로 거른다 — 물리 오버랩을 쓰지 않는다. 타이머 방식이라 Tick 없음.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class US_API UUSRamComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUSRamComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 충각 피해 배율을 정한다 (방패기 "철벽 돌진" 능력, 1 = 기본) */
	void SetDamageScale(float Scale);

protected:
	/** 반경 안의 살아 있는 적에게 데미지 (타이머) */
	void ApplyRamDamage();

	/** 소유자 사망 시 타이머 정지 */
	void HandleOwnerDeath();

	/** 한 번에 주는 기본 데미지 (공격력 배율 1 기준) */
	UPROPERTY(EditDefaultsOnly, Category = "Ram", meta = (ClampMin = "0.0"))
	float RamDamage = 8.0f;

	/**
	 * 이 거리 안의 적을 친다 (cm). 140(접촉 거리 바로 바깥)이면 적이 대장 화력에 먼저 죽어 거의 안 닿았다 →
	 * Vampire Survivors "마늘"처럼 몸 주변 오라로 넓힘 (빌드 테스트 2026-11-01 "방패기 쓸모없음")
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Ram", meta = (ClampMin = "0.0"))
	float RamRadius = 300.0f;

	/** 칠 때마다 방패기 바깥쪽으로 밀어내는 속도 (uu/s, 순간 속도 변화, 무기 넉백과 같은 방식). 0 = 밀지 않음 */
	UPROPERTY(EditDefaultsOnly, Category = "Ram", meta = (ClampMin = "0.0"))
	float KnockbackSpeed = 900.0f;

	/** 데미지 간격(초) */
	UPROPERTY(EditDefaultsOnly, Category = "Ram", meta = (ClampMin = "0.05"))
	float RamInterval = 0.5f;

private:
	FTimerHandle RamTimerHandle;

	/** 능력이 정한 충각 피해 배율 (런타임 상태) */
	float DamageScale = 1.0f;

	/** 소유자 속성 (BeginPlay에서 한 번 캐싱. 공격력 배율을 읽는다) */
	UPROPERTY()
	TObjectPtr<UUSAttributeComponent> OwnerAttribute;
};
