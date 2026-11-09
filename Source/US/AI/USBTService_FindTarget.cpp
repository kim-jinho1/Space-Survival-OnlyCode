#include "AI/USBTService_FindTarget.h"
#include "AI/USAllyAIController.h"
#include "Agents/USAllyDroneCharacter.h"
#include "Agents/USEnemyCharacter.h"
#include "Agents/USPlayerCharacter.h"
#include "Components/USAimComponent.h"
#include "Core/USLocationUtils.h"
#include "Core/USTeamSubsystem.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "Engine/World.h"

UUSBTService_FindTarget::UUSBTService_FindTarget()
{
	NodeName = TEXT("US Find Target");
	Interval = 0.25f;
	RandomDeviation = 0.05f;

	TargetEnemyKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UUSBTService_FindTarget, TargetEnemyKey), AActor::StaticClass());
	TargetEnemyKey.SelectedKeyName = USAllyBlackboardKeys::TargetEnemy;
}

void UUSBTService_FindTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	const AAIController* AI = OwnerComp.GetAIOwner();
	const AUSAllyDroneCharacter* Ally = AI ? Cast<AUSAllyDroneCharacter>(AI->GetPawn()) : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (Ally == nullptr || Blackboard == nullptr)
	{
		return;
	}

	const UWorld* World = Ally->GetWorld();
	const UUSTeamSubsystem* Team = World ? World->GetSubsystem<UUSTeamSubsystem>() : nullptr;
	if (Team == nullptr)
	{
		Blackboard->ClearValue(TargetEnemyKey.GetSelectedKeyID());
		return;
	}

	// 가장 가까운 적을 고르되, 대장에게서 리쉬 밖인 적은 건너뛴다. 제곱 거리로 비교.
	// 표시된 적은 "점수 거리"(실제 거리 × MarkedDistanceScale)로 비교해 우선한다. 반경 판정은 실제 거리로 한다
	const FVector MyLocation = Ally->GetActorLocation();
	const AUSPlayerCharacter* Leader = Ally->GetLeader();
	const FVector LeaderLocation = Leader ? Leader->GetActorLocation() : MyLocation;
	const float EngageRangeSq = FMath::Square(EngageRange);
	const float LeaderLeashSq = FMath::Square(LeaderLeash);
	const float MarkedScaleSq = FMath::Square(MarkedDistanceScale);

	AActor* Best = nullptr;
	float BestScoreSq = TNumericLimits<float>::Max();

	// 화면 밖 적은 고르지 않는다 (공격이 항상 보이게, 대장 자동 조준과 같은 규칙)
	const FUSScreenBounds Screen(World);

	// 대장이 클릭으로 고정한 수동 표적은 거리와 상관없이 1순위 — 쏘던 적을 끝낼 때까지 기다리지 않고 다음 틱에 바꾼다
	// (사용자 2026-11-02, RTS 공격 명령처럼). 리쉬·사거리·화면·죽을 적 규칙은 그대로 지킨다
	const UUSAimComponent* LeaderAim = Leader ? Leader->GetAimComponent() : nullptr;
	if (AActor* Manual = LeaderAim ? LeaderAim->GetManualTarget() : nullptr; IsValid(Manual))
	{
		const AUSEnemyCharacter* ManualEnemy = Cast<AUSEnemyCharacter>(Manual);
		const FVector ManualLocation = Manual->GetActorLocation();
		const bool bUsable = (ManualEnemy == nullptr || (ManualEnemy->IsAlive() && !ManualEnemy->IsDoomed()))
			&& FVector::DistSquared(LeaderLocation, ManualLocation) <= LeaderLeashSq
			&& FVector::DistSquared(MyLocation, ManualLocation) <= EngageRangeSq
			&& Screen.Contains(ManualLocation);
		if (bUsable)
		{
			Blackboard->SetValue<UBlackboardKeyType_Object>(TargetEnemyKey.GetSelectedKeyID(), Manual);
			return;
		}
	}

	for (AActor* Enemy : Team->GetEnemies())
	{
		if (Enemy == nullptr || !IsValid(Enemy))
		{
			continue;
		}

		const FVector EnemyLocation = Enemy->GetActorLocation();
		if (FVector::DistSquared(LeaderLocation, EnemyLocation) > LeaderLeashSq)
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(MyLocation, EnemyLocation);
		if (DistSq > EngageRangeSq)
		{
			continue;
		}

		// 사격 태스크가 놓는 적(죽었거나 날아오는 볼트로 죽을 적)은 고르지 않는다.
		// 고르면 태스크가 계속 실패하고, 키가 비지 않아 교전 브랜치로 다시 못 들어간다 (플레이테스트 2026-10-20)
		const AUSEnemyCharacter* EnemyCharacter = Cast<AUSEnemyCharacter>(Enemy);
		if (EnemyCharacter && (!EnemyCharacter->IsAlive() || EnemyCharacter->IsDoomed()))
		{
			continue;
		}

		const float ScoreSq = (EnemyCharacter && EnemyCharacter->IsMarked()) ? DistSq * MarkedScaleSq : DistSq;
		if (ScoreSq <= BestScoreSq && Screen.Contains(EnemyLocation))
		{
			BestScoreSq = ScoreSq;
			Best = Enemy;
		}
	}

	if (Best)
	{
		Blackboard->SetValue<UBlackboardKeyType_Object>(TargetEnemyKey.GetSelectedKeyID(), Best);
	}
	else
	{
		Blackboard->ClearValue(TargetEnemyKey.GetSelectedKeyID());
	}
}

FString UUSBTService_FindTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("Nearest enemy within %.0f (leash %.0f, marked x%.2f) -> %s"), EngageRange, LeaderLeash, MarkedDistanceScale, *TargetEnemyKey.SelectedKeyName.ToString());
}

void UUSBTService_FindTarget::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* Blackboard = GetBlackboardAsset())
	{
		TargetEnemyKey.ResolveSelectedKey(*Blackboard);
	}
}
