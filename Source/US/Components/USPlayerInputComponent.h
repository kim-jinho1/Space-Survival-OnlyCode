#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USPlayerInputComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class US_API UUSPlayerInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUSPlayerInputComponent();

	/** APawn::SetupPlayerInputComponent에서 호출 — Enhanced Input 액션을 소유 캐릭터에 바인딩 */
	void BindInputActions(class UInputComponent* PlayerInputComponent);

	/** NotifyControllerChanged에서 호출 — 로컬 플레이어에 기본 매핑 컨텍스트를 추가 */
	void AddMappingContext();

protected:
	UPROPERTY(EditAnywhere, Category = InputSystem)
	TObjectPtr<class UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, Category = InputSystem)
	TObjectPtr<class UInputAction> MoveAction;		// 이동

	UPROPERTY(EditAnywhere, Category = InputSystem)
	TObjectPtr<class UInputAction> ZoomAction;		// 카메라 줌 (마우스 휠, Axis1D)

	UPROPERTY(EditAnywhere, Category = InputSystem)
	TObjectPtr<class UInputAction> RunAction;		// 달리기

	UPROPERTY(EditAnywhere, Category = InputSystem)
	TObjectPtr<class UInputAction> FireAction;		// 발사

	UPROPERTY(EditAnywhere, Category = InputSystem)
	TObjectPtr<class UInputAction> InteractAction;	// 상호작용
};
