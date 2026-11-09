#include "Components/USCombatComponent.h"
#include "Equipments/USWeapon.h"
#include "Equipments/USEquipment.h"

UUSCombatComponent::UUSCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUSCombatComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UUSCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 맵을 내릴 때는 월드가 무기까지 같이 치우므로 주인이 파괴될 때만 직접 지운다
	if (EndPlayReason == EEndPlayReason::Destroyed && IsValid(MainWeapon))
	{
		MainWeapon->Destroy();
	}
	MainWeapon = nullptr;

	Super::EndPlay(EndPlayReason);
}

void UUSCombatComponent::SetMainWeapon(AUSWeapon* NewWeapon)
{
	MainWeapon = NewWeapon;

	BroadcastWeaponUpdate();
}

void UUSCombatComponent::BroadcastWeaponUpdate() const
{
	if (OnChangedWeapon.IsBound())
	{
		OnChangedWeapon.Broadcast();
	}
}
