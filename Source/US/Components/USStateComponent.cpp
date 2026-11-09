#include "Components/USStateComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UUSStateComponent::UUSStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUSStateComponent::BeginPlay()
{
	Super::BeginPlay();

	// 스폰 시 깨끗한 초기 상태 보장 (BP 아카이타입에 굳은 값이 있어도 강제 정상화)
	CurrentState = FGameplayTag::EmptyTag;
	bMovementInputEnabled = true;
}

void UUSStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MovementUnlockTimer);
	}

	Super::EndPlay(EndPlayReason);
}

void UUSStateComponent::ClearCurrentState()
{
	if (CurrentState == USGameplayTags::Character_State_Death)
	{
		return;
	}

	CurrentState = FGameplayTag::EmptyTag;
}

bool UUSStateComponent::IsCurrentStateEqualToAny(const FGameplayTagContainer& TagsToCheck) const
{
	// 상태 미설정(빈 태그)이면 어떤 상태에도 해당하지 않음
	if (!CurrentState.IsValid())
	{
		return false;
	}

	return TagsToCheck.HasTagExact(CurrentState);
}

void UUSStateComponent::ToggleMovementInput(bool bEnabled, float Duration)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		bMovementInputEnabled = bEnabled;
		return;
	}

	FTimerManager& TimerManager = World->GetTimerManager();

	// 잠글 때는 예약되어 있던 해제 타이머부터 취소한다.
	if (bEnabled == false)
	{
		TimerManager.ClearTimer(MovementUnlockTimer);
		bMovementInputEnabled = false;
		return;
	}

	// Duration이 없으면(애니메이션 노티파이 등) 예약 없이 즉시 해제한다.
	if (Duration <= 0.f)
	{
		TimerManager.ClearTimer(MovementUnlockTimer);
		MovementInputEnableAction();
		return;
	}

	// 핸들이 남으므로 덮어쓰기/취소가 명확하다.
	TimerManager.SetTimer(
		MovementUnlockTimer,
		this, &UUSStateComponent::MovementInputEnableAction,
		Duration, false);
}

void UUSStateComponent::MovementInputEnableAction()
{
	bMovementInputEnabled = true;
	ClearCurrentState();
}

