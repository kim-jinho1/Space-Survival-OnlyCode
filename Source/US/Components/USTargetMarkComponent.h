#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USTargetMarkComponent.generated.h"

class AUSWeapon;

/**
 * 표적 지정 (대장 보조 능력, D6 · 기획서 P0-4 마킹): 대장 주무기가 맞힌 적에게 Duration초 표시를 건다.
 * 표시된 적은 부하에게 받는 피해가 ×(1 + DamageBonus) (계산은 AUSEnemyCharacter::TakeDamage).
 * 대장 화력이 아니라 편대 화력을 키우는 카드라 후반에도 고를 이유가 남는다.
 * 능력을 배우기 전(DamageBonus 0)에는 무기를 구독하지 않는다. Tick 없음.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class US_API UUSTargetMarkComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUSTargetMarkComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 능력 단계를 적용한다 (부하 피해 보너스, 지속 시간). 처음 배우면 주무기 명중 알림을 구독한다 */
	void SetMark(float InDamageBonus, float InDuration);

protected:
	/** 주무기 명중 알림 (AUSWeapon::OnTargetHit) */
	void HandleTargetHit(AActor* HitActor);

	/** 주무기 명중 알림 구독을 푼다 */
	void UnbindWeapon();

private:
	/** 구독 중인 주무기 (무기가 먼저 사라질 수 있어 약한 참조) */
	TWeakObjectPtr<AUSWeapon> BoundWeapon;

	FDelegateHandle TargetHitHandle;

	/** 능력이 정한 부하 피해 보너스 (0 = 안 배움) */
	float DamageBonus = 0.0f;

	/** 능력이 정한 표시 지속 시간(초) */
	float Duration = 0.0f;
};
