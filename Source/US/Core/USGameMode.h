#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "USGameMode.generated.h"

/**
 * 기본 게임모드.
 */
UCLASS()
class US_API AUSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AUSGameMode();

protected:
	/** HUDClass가 비어 있으면 엔진의 HUD 스폰이 조용히 실패하므로 스폰 전에 경고를 남긴다 */
	virtual void InitializeHUDForPlayer_Implementation(APlayerController* NewPlayer) override;
	
};
