#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "USBTService_UpdateSlot.generated.h"

/**
 * 분대 슬롯의 월드 위치를 블랙보드 SlotLocation에 갱신하는 서비스.
 * 대장이 움직이거나 진형 방향이 바뀌면 목표 지점이 바뀌므로 Interval마다 다시 쓴다.
 */
UCLASS()
class US_API UUSBTService_UpdateSlot : public UBTService
{
	GENERATED_BODY()

public:
	UUSBTService_UpdateSlot();

protected:
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

	/** 슬롯 위치를 쓸 블랙보드 키 (Vector) */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector SlotLocationKey;
};
