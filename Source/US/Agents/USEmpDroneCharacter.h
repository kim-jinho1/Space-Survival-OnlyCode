#pragma once

#include "CoreMinimal.h"
#include "Agents/USAllyDroneCharacter.h"
#include "USEmpDroneCharacter.generated.h"

class UUSEmpPulseComponent;

/**
 * EMP기 (슬로우). 총 없이 대장 옆에서 주기적으로 EMP 펄스를 퍼뜨려 주변 적을 느리게 한다.
 */
UCLASS()
class US_API AUSEmpDroneCharacter : public AUSAllyDroneCharacter
{
	GENERATED_BODY()

public:
	AUSEmpDroneCharacter();

protected:
	/** 감속 펄스 + 바닥 고리 표시 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSEmpPulseComponent> EmpPulse;
};
