#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "USBTService_FindTarget.generated.h"

/**
 * 교전 대상을 고르는 서비스. UUSTeamSubsystem에서 내 위치 기준 가장 가까운 적을 찾되,
 * 대장에게서 LeaderLeash보다 먼 적은 무시한다 (진형을 벗어나 추격하지 않게 — "추격 한계").
 * 대장이 클릭으로 고정한 수동 표적(UUSAimComponent::GetManualTarget)은 쏠 수 있으면 무조건 먼저 고른다.
 * 대장이 표적 지정한 적(AUSEnemyCharacter::IsMarked)은 거리를 MarkedDistanceScale배로 쳐서 우선한다 (D6 · P0-4 마킹 우선).
 * 결과를 블랙보드 TargetEnemy(Object)에 쓰고, 없으면 키를 비운다.
 */
UCLASS()
class US_API UUSBTService_FindTarget : public UBTService
{
	GENERATED_BODY()

public:
	UUSBTService_FindTarget();

protected:
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

	/** 교전 대상을 쓸 블랙보드 키 (Object) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetEnemyKey;

	/**
	 * 내 위치 기준 적 탐지 반경(cm). 사격 태스크의 FireRange보다 크면 그 사이 적은 몸만 돌리고 쏘지 않는다.
	 * 대장 자동 조준(1600)보다 너무 짧으면 부하 범위 안의 적은 이미 대장 탄에 죽을 적이 돼 노릴 게 없다 (플레이테스트 2026-10-21)
	 */
	UPROPERTY(EditAnywhere, Category = "Target")
	float EngageRange = 1400.0f;

	/** 대장 기준 추격 한계(cm). 이보다 먼 적은 대상에서 제외 */
	UPROPERTY(EditAnywhere, Category = "Target")
	float LeaderLeash = 1400.0f;

	/** 표적 지정된 적은 거리를 이 배율로 쳐서 비교한다 (0.5 = 두 배 먼 표시 적과 같은 순위, 1 = 우대 없음). 반경·추격 한계는 실제 거리로 본다 */
	UPROPERTY(EditAnywhere, Category = "Target", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float MarkedDistanceScale = 0.5f;
};
