#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "USBTTask_MoveToSlot.generated.h"

/**
 * 블랙보드 SlotLocation을 향해 직진하며 진형을 유지하는 태스크.
 * NavMesh 없이 AddMovementInput만 쓴다 (적 추적과 같은 방식). 슬롯 안에 들어오면 멈춰 서고,
 * 끝내지 않고 계속 실행 중(InProgress)으로 남는다 — 상위 데코레이터/셀렉터가 끊을 때까지 진형을 지킨다.
 */
UCLASS()
class US_API UUSBTTask_MoveToSlot : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UUSBTTask_MoveToSlot();

protected:
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual FString GetStaticDescription() const override;

	/** 노드 인스턴스 메모리. BT 노드 객체는 여러 AI가 공유하므로 상태는 여기에 둔다 */
	struct FUSMoveToSlotMemory
	{
		/** 현재 이동 중인지 (히스테리시스용) */
		bool bMoving = false;
	};

	/** 슬롯 위치를 읽을 블랙보드 키 (Vector) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector SlotLocationKey;

	/** 이 거리 안이면 도착으로 보고 멈춘다(cm) */
	UPROPERTY(EditAnywhere, Category = "Move")
	float AcceptanceRadius = 60.0f;

	/** 도착 후 다시 움직이기 시작하는 거리(cm). AcceptanceRadius보다 커야 경계에서 떨림이 없다 */
	UPROPERTY(EditAnywhere, Category = "Move")
	float ResumeRadius = 110.0f;

	/** 대장과 이 거리(cm, 중심 간) 안이면 밀려난다. 대장 기체 폭에 맞춘다 */
	UPROPERTY(EditAnywhere, Category = "Separation", meta = (ClampMin = "0.0"))
	float LeaderSeparationRadius = 200.0f;

	/** 다른 부하와 이 거리(cm, 중심 간) 안이면 밀려난다. 부하 기체 폭에 맞춘다 */
	UPROPERTY(EditAnywhere, Category = "Separation", meta = (ClampMin = "0.0"))
	float AllySeparationRadius = 160.0f;

	/** 밀어내는 힘의 비중. 슬롯으로 가는 힘(1)과 더해진다 */
	UPROPERTY(EditAnywhere, Category = "Separation", meta = (ClampMin = "0.0"))
	float SeparationWeight = 1.5f;

private:
	/** 대장·다른 부하에게서 멀어지는 방향(크기 0~여러 개 합). 가까울수록 세다 */
	FVector ComputeSeparation(const APawn* Pawn) const;
};
