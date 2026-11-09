#include "Agents/USShieldDroneCharacter.h"
#include "Components/USDamageReflectComponent.h"
#include "Components/USRamComponent.h"
#include "Components/USTauntComponent.h"

AUSShieldDroneCharacter::AUSShieldDroneCharacter()
{
	Taunt = CreateDefaultSubobject<UUSTauntComponent>(TEXT("Taunt"));
	Ram = CreateDefaultSubobject<UUSRamComponent>(TEXT("Ram"));
	DamageReflect = CreateDefaultSubobject<UUSDamageReflectComponent>(TEXT("DamageReflect"));

	// 방패기는 총을 들지 않는다
	DefaultWeaponClass = nullptr;
}
