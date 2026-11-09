#include "AI/USBTService_UpdateSlot.h"
#include "AI/USAllyAIController.h"
#include "Agents/USAllyDroneCharacter.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

UUSBTService_UpdateSlot::UUSBTService_UpdateSlot()
{
	NodeName = TEXT("US Update Slot Location");
	Interval = 0.1f;
	RandomDeviation = 0.02f;

	// 에디터에서 Vector 키만 고를 수 있게 제한
	SlotLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UUSBTService_UpdateSlot, SlotLocationKey));
	SlotLocationKey.SelectedKeyName = USAllyBlackboardKeys::SlotLocation;
}

void UUSBTService_UpdateSlot::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	const AAIController* AI = OwnerComp.GetAIOwner();
	const AUSAllyDroneCharacter* Ally = AI ? Cast<AUSAllyDroneCharacter>(AI->GetPawn()) : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (Ally == nullptr || Blackboard == nullptr)
	{
		return;
	}

	Blackboard->SetValue<UBlackboardKeyType_Vector>(SlotLocationKey.GetSelectedKeyID(), Ally->GetSlotLocation());
}

FString UUSBTService_UpdateSlot::GetStaticDescription() const
{
	return FString::Printf(TEXT("Slot -> %s"), *SlotLocationKey.SelectedKeyName.ToString());
}

void UUSBTService_UpdateSlot::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	// 키 셀렉터는 BT 애셋의 블랙보드에서 키 ID를 해석해 둬야 GetSelectedKeyID가 유효하다
	if (UBlackboardData* Blackboard = GetBlackboardAsset())
	{
		SlotLocationKey.ResolveSelectedKey(*Blackboard);
	}
}
