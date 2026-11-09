#pragma once

#include "CoreMinimal.h"
#include "USProgressionTypes.generated.h"

/** 스탯 종류 */
UENUM(BlueprintType)
enum class EUSEffectType : uint8
{
	HealHealth,
	MaxHealth,
	AttackDamage,
	AttackSpeed,
	MoveSpeed
};
