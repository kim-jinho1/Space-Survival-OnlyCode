#include "Core/USGameMode.h"
#include "US.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

AUSGameMode::AUSGameMode()
{}

void AUSGameMode::InitializeHUDForPlayer_Implementation(APlayerController* NewPlayer)
{
	// 엔진은 HUDClass를 검사하지 않고 ClientSetHUD → SpawnActor<AHUD>로 넘긴다.
	// BP에서 HUD Class를 None으로 비우면 "SpawnActor failed because no class was specified"만 남아
	// 어느 애셋의 어느 값인지 알 수 없으므로, 스폰 직전에 게임모드와 프로퍼티 이름을 찍는다.
	// 동작은 바꾸지 않는다(Super 호출 유지) — HUD를 안 쓰는 게 의도라면 경고만 보고 넘기면 된다.
	if (HUDClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: HUDClass is None; the engine HUD spawn for %s will fail. Set HUD Class in the game mode BP (default: HUD)"),
			*GetNameSafe(GetClass()), *GetNameSafe(NewPlayer));
	}

	Super::InitializeHUDForPlayer_Implementation(NewPlayer);
}
