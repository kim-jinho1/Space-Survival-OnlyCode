#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USDamageReflectComponent.generated.h"

class UDamageType;
class AController;

/**
 * 피격 반사 (방패기 능력): 적에게 맞으면 받은 피해의 일부를 때린 적에게 되돌린다.
 * 소유자의 OnTakeAnyDamage를 구독한다. 비율이 0이면(능력을 안 배웠으면) 아무것도 하지 않는다. Tick 없음.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class US_API UUSDamageReflectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUSDamageReflectComponent();

	virtual void BeginPlay() override;

	/** 되돌리는 비율을 정한다 (0.3 = 30%) */
	void SetReflectFraction(float Fraction);

protected:
	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

private:
	/** 능력이 정한 반사 비율 (런타임 상태) */
	float ReflectFraction = 0.0f;
};
