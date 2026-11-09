#include "Components/USWorldHealthBarComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UUSWorldHealthBarComponent::UUSWorldHealthBarComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 표시 전용: 충돌·그림자·내비 영향 없음
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCanEverAffectNavigation(false);
	CastShadow = false;
	SetGenerateOverlapEvents(false);

	// 적이 돌아도 바는 카메라를 향한 채로 둔다
	SetUsingAbsoluteRotation(true);

	// 맞기 전에는 숨겨 둔다 (화면이 바로 가득 차지 않게)
	SetHiddenInGame(true);
}

void UUSWorldHealthBarComponent::ShowHealthRatio(float Ratio)
{
	if (Ratio <= 0.0f)
	{
		HideBar();
		return;
	}

	SetCustomPrimitiveDataFloat(RatioDataIndex, FMath::Clamp(Ratio, 0.0f, 1.0f));
	SetHiddenInGame(false);

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// 다시 맞으면 타이머를 새로 걸어 표시 시간을 연장한다
	World->GetTimerManager().SetTimer(HideTimerHandle, this, &UUSWorldHealthBarComponent::HideBar, VisibleDuration, false);
}

void UUSWorldHealthBarComponent::HideBar()
{
	SetHiddenInGame(true);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
	}
}
