#include "Core/USTitleGameMode.h"
#include "Agents/USTitlePlayerController.h"

AUSTitleGameMode::AUSTitleGameMode()
{
	// 타이틀에는 조종할 기체가 없다. 기본값(ADefaultPawn)을 그대로 두면 날아다니는 관전 폰이 생긴다
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AUSTitlePlayerController::StaticClass();
}
