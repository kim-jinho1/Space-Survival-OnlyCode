#include "AI/USBTTask_MoveToSlot.h"
#include "AI/USAllyAIController.h"
#include "Agents/USAllyDroneCharacter.h"
#include "Agents/USPlayerCharacter.h"
#include "Components/USSquadComponent.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

UUSBTTask_MoveToSlot::UUSBTTask_MoveToSlot()
{
	NodeName = TEXT("US Move To Slot");
	// 매 프레임 AddMovementInput을 넣어야 CharacterMovement가 움직이므로 틱이 필요하다
	bNotifyTick = true;

	SlotLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UUSBTTask_MoveToSlot, SlotLocationKey));
	SlotLocationKey.SelectedKeyName = USAllyBlackboardKeys::SlotLocation;
}

EBTNodeResult::Type UUSBTTask_MoveToSlot::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FUSMoveToSlotMemory* Memory = reinterpret_cast<FUSMoveToSlotMemory*>(NodeMemory);
	Memory->bMoving = false;

	// 끝내지 않는다. 진형 유지는 "계속 실행 중"이 정상 상태.
	return EBTNodeResult::InProgress;
}

void UUSBTTask_MoveToSlot::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AI = OwnerComp.GetAIOwner();
	APawn* Pawn = AI ? AI->GetPawn() : nullptr;
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (Pawn == nullptr || Blackboard == nullptr)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FUSMoveToSlotMemory* Memory = reinterpret_cast<FUSMoveToSlotMemory*>(NodeMemory);

	const FVector SlotLocation = Blackboard->GetValue<UBlackboardKeyType_Vector>(SlotLocationKey.GetSelectedKeyID());
	FVector ToSlot = SlotLocation - Pawn->GetActorLocation();
	ToSlot.Z = 0.0f;
	const float DistSq = ToSlot.SizeSquared();

	// 히스테리시스: 도착하면 ResumeRadius 밖으로 벗어날 때까지 가만히 있는다
	if (Memory->bMoving)
	{
		if (DistSq <= FMath::Square(AcceptanceRadius))
		{
			Memory->bMoving = false;
		}
	}
	else if (DistSq > FMath::Square(ResumeRadius))
	{
		Memory->bMoving = true;
	}

	// 분리 조향(boids separation): 진형이 돌 때 슬롯까지 직선으로 가면 대장·다른 부하를 가로질러 겹친다.
	// 가까운 기체에게서 멀어지는 힘을 더해 돌아 나가게 하고, 슬롯에 서 있어도 누가 겹쳐 오면 비켜 준다.
	const FVector Separation = ComputeSeparation(Pawn) * SeparationWeight;
	const FVector Seek = Memory->bMoving ? ToSlot.GetSafeNormal() : FVector::ZeroVector;
	const FVector Desired = (Seek + Separation).GetClampedToMaxSize(1.0f);
	if (!Desired.IsNearlyZero(0.05f))
	{
		Pawn->AddMovementInput(Desired.GetSafeNormal(), Desired.Size());
	}
}

FVector UUSBTTask_MoveToSlot::ComputeSeparation(const APawn* Pawn) const
{
	const AUSAllyDroneCharacter* Ally = Cast<AUSAllyDroneCharacter>(Pawn);
	if (Ally == nullptr)
	{
		return FVector::ZeroVector;
	}

	const FVector SelfLocation = Pawn->GetActorLocation();
	FVector Push = FVector::ZeroVector;

	auto AddPush = [&SelfLocation, &Push](const AActor* Other, float Radius)
	{
		if (Other == nullptr || Other->IsHidden() || Radius <= 0.0f)
		{
			return;
		}

		FVector Away = SelfLocation - Other->GetActorLocation();
		Away.Z = 0.0f;
		const float Distance = Away.Size();
		if (Distance >= Radius)
		{
			return;
		}

		// 완전히 겹치면 방향이 없으니 임의의 옆 방향으로 민다
		const FVector Direction = Distance > KINDA_SMALL_NUMBER ? Away / Distance : FVector::RightVector;
		Push += Direction * (1.0f - Distance / Radius);
	};

	AddPush(Ally->GetLeader(), LeaderSeparationRadius);

	// 진형 인원은 최대 8명이라 매 틱 훑어도 싸다 (월드 검색 없이 분대 목록만 본다)
	if (const UUSSquadComponent* Squad = Ally->GetSquad())
	{
		for (const FUSSquadMember& Member : Squad->GetMembers())
		{
			if (Member.Ally != Ally)
			{
				AddPush(Member.Ally, AllySeparationRadius);
			}
		}
	}

	return Push;
}

uint16 UUSBTTask_MoveToSlot::GetInstanceMemorySize() const
{
	return sizeof(FUSMoveToSlotMemory);
}

FString UUSBTTask_MoveToSlot::GetStaticDescription() const
{
	return FString::Printf(TEXT("Hold %s (accept %.0f / resume %.0f)"), *SlotLocationKey.SelectedKeyName.ToString(), AcceptanceRadius, ResumeRadius);
}

void UUSBTTask_MoveToSlot::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* Blackboard = GetBlackboardAsset())
	{
		SlotLocationKey.ResolveSelectedKey(*Blackboard);
	}
}
