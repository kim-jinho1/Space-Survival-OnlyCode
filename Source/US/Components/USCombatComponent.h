#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USCombatComponent.generated.h"

class AUSWeapon;

DECLARE_MULTICAST_DELEGATE(FDelegateOnChangedWeapon);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class US_API UUSCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UUSCombatComponent();

protected:
	virtual void BeginPlay() override;

	/** 주인이 파괴되면 따로 스폰한 무기 액터도 치운다 (부하 교체·격추 자리 덮어쓰기 때 무기만 월드에 남던 문제, R-28) */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	FORCEINLINE AUSWeapon* GetMainWeapon() const { return MainWeapon; }

	void SetMainWeapon(AUSWeapon* NewWeapon);

	FORCEINLINE bool IsCombatEnabled() const { return bCombatEnabled; }
	FORCEINLINE void SetCombatEnabled(bool bEnabled) { bCombatEnabled = bEnabled; }

public:
	void BroadcastWeaponUpdate() const;

public:
	FDelegateOnChangedWeapon OnChangedWeapon;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<AUSWeapon> MainWeapon;

	/** 전투 활성화 여부 */
	UPROPERTY(EditAnywhere)
	bool bCombatEnabled = true;

};
