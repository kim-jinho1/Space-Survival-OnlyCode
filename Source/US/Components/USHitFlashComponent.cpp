#include "Components/USHitFlashComponent.h"
#include "US.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"

UUSHitFlashComponent::UUSHitFlashComponent()
{
	// 발광 감쇠는 머티리얼이 시간으로 계산하므로 틱이 필요 없다
	PrimaryComponentTick.bCanEverTick = false;
}

void UUSHitFlashComponent::SetFlashTarget(UPrimitiveComponent* InTarget)
{
	FlashTarget = InTarget;
}

void UUSHitFlashComponent::PlayFlash()
{
	if (FlashTarget == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: HitFlash has no target; call SetFlashTarget first"), *GetNameSafe(GetOwner()));
		return;
	}

	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// 머티리얼 Time 노드(일시정지 반영)와 같은 기준인 월드 시간을 적는다
	FlashTarget->SetCustomPrimitiveDataFloat(CustomDataIndex, World->GetTimeSeconds());
}
