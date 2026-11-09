#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USAttributeComponent.generated.h"

class AController;

/** 체력 변경 알림 (CurrentHealth, MaxHealth) */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDelegateOnHealthChanged, float, float);

/** 데미지 알림 (실제로 깎인 양, 때린 컨트롤러 — 없을 수 있음) */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDelegateOnDamaged, float, AController*);

/** 사망 알림 */
DECLARE_MULTICAST_DELEGATE(FDelegateOnDeath);

/** 스탯(배율/최대체력) 변경 알림 */
DECLARE_MULTICAST_DELEGATE(FDelegateOnStatsChanged);

enum class EUSEffectType : uint8;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class US_API UUSAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UUSAttributeComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	
	/** 스탯 배율 getter (기본 1.0) */
	FORCEINLINE float GetMoveSpeedMultiplier() const { return MoveSpeedMultiplier; }
	FORCEINLINE float GetAttackDamageMultiplier() const { return AttackDamageMultiplier; }
	FORCEINLINE float GetAttackSpeedMultiplier() const { return AttackSpeedMultiplier; }

	void ApplyEffect(const EUSEffectType Type, float value);

	/** 생존 여부 */
	bool IsAlive() const;
	
	/** 체력 회복 */
	void Heal(float Amount);

	/** 최대 체력을 Factor배로 (증가분만큼 현재 체력도 함께 회복) */
	void ScaleMaxHealth(float Factor);

	/** 최대 체력을 직접 지정하고 현재 체력을 가득 채운다 (스폰 전 초기화용) */
	void SetMaxHealth(float NewMaxHealth);

	/** 이동 속도 배율 증가 */
	void AddMoveSpeedMultiplier(float Amount);

	/** 공격력 배율에 Factor를 곱한다 (강화는 곱셈으로 쌓여 숫자가 지수로 커진다) */
	void ScaleAttackDamageMultiplier(float Factor);

	/** 시작 공격력 배율을 지정한다 (스폰 직후 초기화용). 이후 강화는 ScaleAttackDamageMultiplier로 곱해진다 */
	void InitAttackDamageMultiplier(float StartMultiplier);

	/** 공격 속도 배율에 Factor를 곱한다 */
	void ScaleAttackSpeedMultiplier(float Factor);

public:
	/** 체력이 변경될 때 호출 */
	FDelegateOnHealthChanged OnHealthChanged;

	/** 데미지를 받아 체력이 줄었을 때 호출 (OnHealthChanged보다 먼저) */
	FDelegateOnDamaged OnDamaged;

	/** 체력이 0이 되어 사망할 때 호출 */
	FDelegateOnDeath OnDeath;

	/** 스탯(배율/최대체력)이 변경될 때 호출 */
	FDelegateOnStatsChanged OnStatsChanged;

protected:
	/** Owner가 데미지를 받을 때 호출 (OnTakeAnyDamage에 바인딩) */
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

protected:
	/** 최대 체력 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Attribute)
	float MaxHealth = 100.0f;
	
	/** 현재 체력 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Attribute)
	float CurrentHealth = 0.0f;
	
	/** 이동 속도 배율 (1.0 = 기본) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute|Stats")
	float MoveSpeedMultiplier = 1.0f;

	/** 공격력 배율 (1.0 = 기본) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute|Stats")
	float AttackDamageMultiplier = 1.0f;

	/** 공격 속도 배율 (1.0 = 기본) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Attribute|Stats")
	float AttackSpeedMultiplier = 1.0f;

	/** 사망 상태 플래그 */
	bool bIsDead = false;
};
