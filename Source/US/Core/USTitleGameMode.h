#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "USTitleGameMode.generated.h"

/**
 * 타이틀 맵 게임모드. 폰을 스폰하지 않고 AUSTitlePlayerController만 둔다.
 * TitleMap 월드 세팅의 게임모드 덮어쓰기로 지정한다 (게임 맵은 기존 게임모드 그대로).
 */
UCLASS()
class US_API AUSTitleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AUSTitleGameMode();
};
