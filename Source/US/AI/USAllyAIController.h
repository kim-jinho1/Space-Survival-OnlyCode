#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "USAllyAIController.generated.h"

/** BT_USAlly / BB_USAlly가 쓰는 블랙보드 키 이름. 애셋의 키 이름과 정확히 같아야 한다 */
namespace USAllyBlackboardKeys
{
	/** Object: 현재 교전 대상 적. 없으면 미설정 */
	inline const FName TargetEnemy(TEXT("TargetEnemy"));

	/** Vector: 내 진형 슬롯의 월드 위치 */
	inline const FName SlotLocation(TEXT("SlotLocation"));
}

/**
 * 부하 드론 AI 컨트롤러. 빙의하면 부하가 들고 있는 BehaviorTree를 실행한다.
 * 판단 로직은 전부 BT 노드(AI/USBT*)에 있다.
 */
UCLASS()
class US_API AUSAllyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AUSAllyAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
};
