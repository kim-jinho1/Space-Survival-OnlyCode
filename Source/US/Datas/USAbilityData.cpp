#include "Datas/USAbilityData.h"
#include "US.h"
#include "Components/USCombatComponent.h"
#include "Components/USDamageReflectComponent.h"
#include "Components/USEmpPulseComponent.h"
#include "Components/USMissileBarrageComponent.h"
#include "Components/USRamComponent.h"
#include "Components/USTargetMarkComponent.h"
#include "Components/USTauntComponent.h"
#include "Components/USUpgradeVisualComponent.h"
#include "Equipments/USWeapon.h"
#include "GameFramework/Actor.h"

namespace
{
	AUSWeapon* FindMainWeapon(const AActor* Target)
	{
		const UUSCombatComponent* Combat = Target->FindComponentByClass<UUSCombatComponent>();
		return Combat ? Combat->GetMainWeapon() : nullptr;
	}
}

const FUSAbilityLevel* UUSAbilityData::GetLevelData(int32 Level) const
{
	return Levels.IsValidIndex(Level - 1) ? &Levels[Level - 1] : nullptr;
}

bool UUSAbilityData::ApplyTo(AActor* Target, int32 Level) const
{
	const FUSAbilityLevel* Data = GetLevelData(Level);
	if (Target == nullptr || Data == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: cannot apply level %d to %s"), *GetName(), Level, *GetNameSafe(Target));
		return false;
	}

	bool bApplied = false;

	switch (AbilityType)
	{
	case EUSAbilityType::MissileBarrage:
		if (UUSMissileBarrageComponent* Barrage = Target->FindComponentByClass<UUSMissileBarrageComponent>())
		{
			Barrage->SetBarrage(FMath::RoundToInt(Data->Value), Data->SecondaryValue);
			bApplied = true;
		}
		break;

	case EUSAbilityType::Pierce:
		if (AUSWeapon* Weapon = FindMainWeapon(Target))
		{
			Weapon->SetPierceCount(FMath::RoundToInt(Data->Value));
			bApplied = true;
		}
		break;

	case EUSAbilityType::Spread:
		if (AUSWeapon* Weapon = FindMainWeapon(Target))
		{
			Weapon->SetSpread(FMath::RoundToInt(Data->Value), Data->SecondaryValue);
			bApplied = true;
		}
		break;

	case EUSAbilityType::Reflect:
		if (UUSDamageReflectComponent* Reflect = Target->FindComponentByClass<UUSDamageReflectComponent>())
		{
			Reflect->SetReflectFraction(Data->Value);
			bApplied = true;
		}
		break;

	case EUSAbilityType::Bulwark:
	{
		UUSTauntComponent* Taunt = Target->FindComponentByClass<UUSTauntComponent>();
		UUSRamComponent* Ram = Target->FindComponentByClass<UUSRamComponent>();
		if (Taunt)
		{
			Taunt->SetTauntRadius(Data->Value);
		}
		if (Ram)
		{
			Ram->SetDamageScale(Data->SecondaryValue);
		}
		bApplied = Taunt != nullptr || Ram != nullptr;
		break;
	}

	case EUSAbilityType::EmpShock:
		if (UUSEmpPulseComponent* Pulse = Target->FindComponentByClass<UUSEmpPulseComponent>())
		{
			Pulse->SetPulseDamage(Data->Value);
			bApplied = true;
		}
		break;

	case EUSAbilityType::EmpField:
		if (UUSEmpPulseComponent* Pulse = Target->FindComponentByClass<UUSEmpPulseComponent>())
		{
			Pulse->SetPulseArea(Data->Value, Data->SecondaryValue);
			bApplied = true;
		}
		break;

	case EUSAbilityType::TargetMark:
		if (UUSTargetMarkComponent* Mark = Target->FindComponentByClass<UUSTargetMarkComponent>())
		{
			Mark->SetMark(Data->Value, Data->SecondaryValue);
			bApplied = true;
		}
		break;

	case EUSAbilityType::ExplosiveRounds:
		if (AUSWeapon* Weapon = FindMainWeapon(Target))
		{
			Weapon->SetExplosiveRounds(Data->Value, Data->SecondaryValue);
			bApplied = true;
		}
		break;

	default:
		break;
	}

	if (!bApplied)
	{
		// 능력 애셋의 OwnerType과 받는 기체가 어긋났다 (예: 총 없는 방패기에 관통탄)
		UE_LOG(LogUS, Warning, TEXT("%s: %s has nothing to receive this ability"), *GetName(), *GetNameSafe(Target));
	}
	else if (UUSUpgradeVisualComponent* UpgradeVisual = Target->FindComponentByClass<UUSUpgradeVisualComponent>())
	{
		// 카드로 배울 때와 부활·합류로 다시 받을 때 모두 여기를 지나므로 등급 외형도 같이 맞춘다 (외형 컴포넌트가 없는 대장은 건너뜀)
		UpgradeVisual->SetAbilityLevel(this, Level);
	}

	return bApplied;
}
