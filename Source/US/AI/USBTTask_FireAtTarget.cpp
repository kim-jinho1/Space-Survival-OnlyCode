#include "AI/USBTTask_FireAtTarget.h"
#include "AI/USAllyAIController.h"
#include "USGameplayTags.h"
#include "Agents/USAllyDroneCharacter.h"
#include "Agents/USEnemyCharacter.h"
#include "Components/USCombatComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "GameFramework/CharacterMovementComponent.h"

UUSBTTask_FireAtTarget::UUSBTTask_FireAtTarget()
{
	NodeName = TEXT("US Fire At Target");
	bNotifyTick = true;
	bNotifyTaskFinished = true;

	TargetEnemyKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UUSBTTask_FireAtTarget, TargetEnemyKey), AActor::StaticClass());
	TargetEnemyKey.SelectedKeyName = USAllyBlackboardKeys::TargetEnemy;
}

EBTNodeResult::Type UUSBTTask_FireAtTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	AUSAllyDroneCharacter* Ally = AI ? Cast<AUSAllyDroneCharacter>(AI->GetPawn()) : nullptr;
	if (Ally == nullptr)
	{
		return EBTNodeResult::Failed;
	}

	// 교전 중엔 이동 방향이 아니라 적을 본다. 회전은 TickTask가 직접 돌린다.
	if (UCharacterMovementComponent* Movement = Ally->GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
	}

	return EBTNodeResult::InProgress;
}

void UUSBTTask_FireAtTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AI = OwnerComp.GetAIOwner();
	AUSAllyDroneCharacter* Ally = AI ? Cast<AUSAllyDroneCharacter>(AI->GetPawn()) : nullptr;
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (Ally == nullptr || Blackboard == nullptr)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	AActor* Target = Cast<AActor>(Blackboard->GetValue<UBlackboardKeyType_Object>(TargetEnemyKey.GetSelectedKeyID()));
	const AUSEnemyCharacter* Enemy = Cast<AUSEnemyCharacter>(Target);
	if (Target == nullptr || !IsValid(Target))
	{
		// 타겟 소실 — Follow로 돌아간다 (FindTarget 서비스가 다음 타겟을 채우면 데코레이터가 다시 진입)
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// 날아오는 볼트만으로 죽을 적(IsDoomed)은 쏘지 않는다 — 피해는 볼트 도착 때 들어가므로 계속 쏘면 탄 낭비.
	// 실패로 끝내면 키가 다른 적으로 바뀌어도(Set → Set) 데코레이터가 다시 진입하지 않아 진형만 따라다닌다.
	// 그래서 태스크는 유지하고, FindTarget 서비스(죽을 적은 거른다)가 다음 표적으로 바꿔 주기를 기다린다
	if (Enemy && (!Enemy->IsAlive() || Enemy->IsDoomed()))
	{
		return;
	}

	// 컨트롤 회전을 적에게 (애님 Aim Offset). SetFocus는 매 틱 불러도 같은 대상이면 비용이 없다.
	AI->SetFocus(Target);

	// 몸 yaw를 적 방향으로 제한 속도로 회전
	FVector ToTarget = Target->GetActorLocation() - Ally->GetActorLocation();
	ToTarget.Z = 0.0f;
	const float TargetYaw = ToTarget.Rotation().Yaw;
	const float CurrentYaw = Ally->GetActorRotation().Yaw;
	const float NewYaw = FMath::FixedTurn(CurrentYaw, TargetYaw, TurnRate * DeltaSeconds);
	Ally->SetActorRotation(FRotator(0.0f, NewYaw, 0.0f));

	// 사거리 안 + 몸이 충분히 돌았을 때만 발사
	const float DistSq = ToTarget.SizeSquared();
	const float YawError = FMath::Abs(FMath::FindDeltaAngleDegrees(NewYaw, TargetYaw));
	if (DistSq > FMath::Square(FireRange) || YawError > FireConeHalfAngle)
	{
		return;
	}

	Ally->TryFire(Target->GetActorLocation(), USGameplayTags::Character_Attack_Rapid);
}

void UUSBTTask_FireAtTarget::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	AAIController* AI = OwnerComp.GetAIOwner();
	AUSAllyDroneCharacter* Ally = AI ? Cast<AUSAllyDroneCharacter>(AI->GetPawn()) : nullptr;

	if (AI)
	{
		AI->ClearFocus(EAIFocusPriority::Gameplay);
	}

	// 따라갈 때는 다시 이동 방향을 본다
	if (Ally)
	{
		if (UCharacterMovementComponent* Movement = Ally->GetCharacterMovement())
		{
			Movement->bOrientRotationToMovement = true;
		}
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

FString UUSBTTask_FireAtTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("Fire at %s within %.0f"), *TargetEnemyKey.SelectedKeyName.ToString(), FireRange);
}

void UUSBTTask_FireAtTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* Blackboard = GetBlackboardAsset())
	{
		TargetEnemyKey.ResolveSelectedKey(*Blackboard);
	}
}
