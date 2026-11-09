#include "AI/USAllyAIController.h"
#include "US.h"
#include "Agents/USAllyDroneCharacter.h"
#include "BehaviorTree/BehaviorTree.h"

AUSAllyAIController::AUSAllyAIController()
{
	// 교전 중엔 태스크가 SetFocus(적)를 걸어 컨트롤 회전이 적을 향하고(애님 Aim Offset),
	// 포커스가 없을 땐 기본값대로 폰 방향을 따라간다. 폰 yaw 자체는 태스크가 직접 돌린다.
}

void AUSAllyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	const AUSAllyDroneCharacter* Ally = Cast<AUSAllyDroneCharacter>(InPawn);
	if (Ally == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("AUSAllyAIController possessed %s which is not an AUSAllyDroneCharacter"), *GetNameSafe(InPawn));
		return;
	}

	UBehaviorTree* Tree = Ally->GetBehaviorTree();
	if (Tree == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("AUSAllyAIController: %s has no BehaviorTree assigned"), *GetNameSafe(InPawn));
		return;
	}

	// RunBehaviorTree가 BT에 지정된 Blackboard 애셋을 같이 올린다.
	RunBehaviorTree(Tree);
}
