#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USHitFlashComponent.generated.h"

class UPrimitiveComponent;

/**
 * 피격 발광. 맞은 시각을 대상 메시의 Custom Primitive Data에 적어 두면
 * 머티리얼이 (현재 시각 - 맞은 시각)으로 발광을 스스로 줄여 간다.
 * 적마다 동적 머티리얼을 만들지 않아 같은 머티리얼끼리 배칭이 유지되고, Tick도 필요 없다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class US_API UUSHitFlashComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUSHitFlashComponent();

	/** 발광시킬 메시를 지정한다 (소유 액터의 PostInitializeComponents에서) */
	void SetFlashTarget(UPrimitiveComponent* InTarget);

	/** 지금 시각을 맞은 시각으로 기록해 발광을 시작한다 */
	void PlayFlash();

protected:
	/**
	 * 맞은 시각을 적을 Custom Primitive Data 인덱스. 머티리얼의 CustomPrimitiveData 노드 인덱스와 같아야 한다.
	 * 기체 재질(M_Colorize·M_ColorizeAdvance)의 HitTime이 1번 — 0번은 부하 티어(UUSUpgradeVisualComponent)가 쓴다
	 */
	UPROPERTY(EditDefaultsOnly, Category = "HitFlash")
	int32 CustomDataIndex = 1;

private:
	/** 발광 대상 메시 */
	UPROPERTY()
	TObjectPtr<UPrimitiveComponent> FlashTarget;
};
