// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/USCurrencyComponent.h"
#include "US.h"

// Sets default values for this component's properties
UUSCurrencyComponent::UUSCurrencyComponent()
{
	// 골드 처리는 이벤트 기반이므로 매 프레임 Tick은 필요하지 않다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UUSCurrencyComponent::AddGold(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	Gold += Amount;
	OnGoldChanged.Broadcast(Gold);

	// 젬마다 불리므로 Verbose. 다시 보려면 콘솔 `log LogUS Verbose`
	UE_LOG(LogUS, Verbose, TEXT("[USCurrency] AddGold %d -> %d"), Amount, Gold);
}

void UUSCurrencyComponent::AddEarnedGold(float BaseAmount)
{
	if (BaseAmount <= 0.0f)
	{
		return;
	}

	const float Total = BaseAmount * GoldGainMultiplier + PendingGoldFraction;
	const int32 Whole = FMath::FloorToInt(Total);
	PendingGoldFraction = Total - static_cast<float>(Whole);

	AddGold(Whole);
}

void UUSCurrencyComponent::ScaleGoldGain(float Factor)
{
	if (Factor <= 0.0f)
	{
		UE_LOG(LogUS, Warning, TEXT("[USCurrency] ScaleGoldGain ignored non-positive factor %f"), Factor);
		return;
	}

	GoldGainMultiplier *= Factor;
}

bool UUSCurrencyComponent::TrySpend(int32 Amount)
{
	if (Amount <= 0 || Gold < Amount)
	{
		return false;
	}

	Gold -= Amount;
	OnGoldChanged.Broadcast(Gold);

	UE_LOG(LogUS, Log, TEXT("[USCurrency] Spend %d -> %d"), Amount, Gold);
	return true;
}
