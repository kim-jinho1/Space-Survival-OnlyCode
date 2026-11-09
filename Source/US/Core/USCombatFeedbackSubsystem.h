#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "USCombatFeedbackSubsystem.generated.h"

/** 적이 받은 데미지 알림 (맞은 위치, 실제로 깎인 양, 대장이 때렸는가) */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FDelegateOnEnemyDamaged, const FVector&, float, bool);

/** 대장이 적을 처치했다는 알림 (처치 위치) */
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateOnPlayerKilledEnemy, const FVector&);

/** 적이 처치됐다는 알림 — 대장·부하 구분 없음. 자폭 등 처치가 아닌 죽음은 오지 않는다 */
DECLARE_MULTICAST_DELEGATE(FDelegateOnEnemyKilled);

/**
 * 전투 연출 중계소. 적이 "맞았다"(누가 때렸든)와 "처치됐다", "대장이 잡았다"를 보고하면
 * 연출·통계 쪽(데미지 숫자, 화면 흔들림, 처치 수·DPS)에 델리게이트로 알린다.
 * 적과 UI·컨트롤러가 서로를 직접 참조하지 않게 한다.
 */
UCLASS()
class US_API UUSCombatFeedbackSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void ReportEnemyDamaged(const FVector& Location, float Damage, bool bByPlayer);
	void ReportEnemyKilled();
	void ReportPlayerKilledEnemy(const FVector& Location);

	FDelegateOnEnemyDamaged OnEnemyDamaged;
	FDelegateOnEnemyKilled OnEnemyKilled;
	FDelegateOnPlayerKilledEnemy OnPlayerKilledEnemy;
};
