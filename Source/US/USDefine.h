#pragma once

#include "CoreMinimal.h"
#include "USDefine.generated.h"

UENUM(BlueprintType)
enum class EUSAttributeType : uint8
{
	Health,
};

UENUM(BlueprintType)
enum class EHitDirection : uint8
{
	Front,
	Back,
	Left,
	Right,
};