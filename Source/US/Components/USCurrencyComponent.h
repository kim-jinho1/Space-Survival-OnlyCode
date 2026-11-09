// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USCurrencyComponent.generated.h"

/** 골드 변경 알림 (NewGold) */
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateOnGoldChanged, int32);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class US_API UUSCurrencyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UUSCurrencyComponent();

public:
	/** 현재 골드 */
	FORCEINLINE int32 GetGold() const { return Gold; }

	/** 골드 획득 배율 (보급선 반복 강화로 곱해진다) */
	FORCEINLINE float GetGoldGainMultiplier() const { return GoldGainMultiplier; }

	/** 골드 추가 (배율 없이 그대로. 환불·보너스용) */
	void AddGold(int32 Amount);

	/** 전투에서 번 골드 추가. 골드 획득 배율을 곱하고, 소수점은 모아 두었다가 1이 넘으면 준다 */
	void AddEarnedGold(float BaseAmount);

	/** 골드 획득 배율에 Factor를 곱한다 (예: 1.1 = +10%) */
	void ScaleGoldGain(float Factor);

	/** 골드 소비 시도. 잔액이 충분하면 차감하고 true 반환 */
	bool TrySpend(int32 Amount);

public:
	/** 골드가 변경될 때 호출 */
	FDelegateOnGoldChanged OnGoldChanged;

protected:
	/** 보유 골드 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Currency)
	int32 Gold = 0;

	/** 골드 획득 배율 (런타임 상태) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Currency)
	float GoldGainMultiplier = 1.0f;

private:
	/** 배율을 곱하고 남은 1 미만 골드. 매번 반올림하면 5 × 1.1 = 5.5가 6이 되어 +10%가 +20%로 부풀기 때문에 모은다 */
	float PendingGoldFraction = 0.0f;
};
