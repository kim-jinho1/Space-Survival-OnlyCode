#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USTauntComponent.generated.h"

/**
 * 도발: 반경 안의 적이 가장 가까운 아군 대신 소유자를 노리게 한다.
 * 적 등록부(UUSTeamSubsystem)에 도발 대상으로 올리기만 하고, 타겟 선택은 적이 주기적으로 한다. Tick 없음.
 * 소유자가 죽으면 스스로 등록을 내린다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class US_API UUSTauntComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUSTauntComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 도발 반경을 바꾼다 (방패기 "철벽 돌진" 능력). 도발 중이면 등록부의 반경도 바로 갱신한다 */
	void SetTauntRadius(float NewRadius);

protected:
	/** 소유자 사망 시 도발 해제 */
	void HandleOwnerDeath();

	/** 도발 등록 해제 */
	void StopTaunt();

	/** 이 거리 안의 적은 소유자를 우선 노린다 (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "Taunt", meta = (ClampMin = "0.0"))
	float TauntRadius = 500.0f;

private:
	/** 등록부에 올라가 있는가 (죽은 뒤 반경을 바꿔도 다시 등록하지 않게) */
	bool bTaunting = false;
};
