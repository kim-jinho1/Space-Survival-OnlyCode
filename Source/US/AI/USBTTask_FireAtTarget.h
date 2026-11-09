#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "USBTTask_FireAtTarget.generated.h"

/**
 * 블랙보드 TargetEnemy를 향해 몸을 돌리고 사거리 안이면 발사하는 태스크.
 * 실행 중 이동 방향 자동 회전을 끄고(교전 중엔 적을 봐야 하므로), 컨트롤러 포커스를 적에게 둬
 * 애님 Aim Offset이 적을 향하게 한다. 타겟이 사라지거나 죽으면 Failed로 끝나 Follow로 돌아간다.
 */
UCLASS()
class US_API UUSBTTask_FireAtTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UUSBTTask_FireAtTarget();

protected:
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual FString GetStaticDescription() const override;

	/** 교전 대상을 읽을 블랙보드 키 (Object) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetEnemyKey;

	/** 발사 사거리(cm). 무기 사거리(FireRange)보다 짧게 잡아 빗나감을 줄인다 */
	UPROPERTY(EditAnywhere, Category = "Fire")
	float FireRange = 1400.0f;

	/** 몸이 적 방향에서 이 각도(도) 안에 들어와야 발사 */
	UPROPERTY(EditAnywhere, Category = "Fire")
	float FireConeHalfAngle = 25.0f;

	/** 적을 향해 몸을 돌리는 속도(도/초) */
	UPROPERTY(EditAnywhere, Category = "Fire")
	float TurnRate = 540.0f;
};
