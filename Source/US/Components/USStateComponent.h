#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"
#include "USGameplayTags.h"
#include "USStateComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class US_API UUSStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UUSStateComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** 현재 상태 Getter/Setter */
	FORCEINLINE void SetCurrentState(const FGameplayTag NewState)
	{
		if (CurrentState == USGameplayTags::Character_State_Death)
		{
			return;
		}

		CurrentState = NewState;
	}

	FORCEINLINE FGameplayTag GetCurrentState() const { return CurrentState; }

	/** 현재 상태 초기화 */
	void ClearCurrentState();

	/** 현재 상태가 주어진 태그 중 하나와 일치하는지 확인 (OR) */
	bool IsCurrentStateEqualToAny(const FGameplayTagContainer& TagsToCheck) const;

public:
	FORCEINLINE bool MovementInputEnabled() { return bMovementInputEnabled; }

	/** 이동 입력 토글 */
	void ToggleMovementInput(bool bEnabled, float Duration = 0.f);

	void MovementInputEnableAction();

protected:
	/** 캐릭터의 현재 상태 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = State)
	FGameplayTag CurrentState;

	/** 이동키 입력 가능 여부 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = State)
	bool bMovementInputEnabled = true;

	/** 이동 입력 잠금 해제 예약 타이머 핸들 */
	FTimerHandle MovementUnlockTimer;
};
