#pragma once

#include "CoreMinimal.h"
#include "Agents/USAllyDroneCharacter.h"
#include "USShieldDroneCharacter.generated.h"

class UUSTauntComponent;
class UUSRamComponent;
class UUSDamageReflectComponent;

/**
 * 방패기 (탱커). 총 없이 진형 앞에서 적을 끌어당겨(도발) 몸으로 막고, 붙은 적을 충각으로 친다.
 * 행동은 컴포넌트가 맡고 이 클래스는 조합만 한다.
 */
UCLASS()
class US_API AUSShieldDroneCharacter : public AUSAllyDroneCharacter
{
	GENERATED_BODY()

public:
	AUSShieldDroneCharacter();

protected:
	/** 반경 안의 적이 이 기체를 노리게 한다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSTauntComponent> Taunt;

	/** 붙은 적에게 주기적 근접 데미지 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSRamComponent> Ram;

	/** 피격 반사 능력 (배우기 전엔 비율 0이라 아무것도 하지 않는다) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSDamageReflectComponent> DamageReflect;
};
