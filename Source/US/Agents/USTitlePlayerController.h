#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "USTitlePlayerController.generated.h"

class UUSTitleWidget;
class UWorld;

/**
 * 타이틀 맵 전용 컨트롤러. 조종할 폰 없이 타이틀 위젯만 띄우고, 버튼에 따라 게임 맵을 열거나 종료한다.
 * 게임 맵의 UI는 AUSPlayerController가, 타이틀 맵의 UI는 이 컨트롤러가 만든다 (맵마다 UI를 만드는 곳은 한 곳).
 */
UCLASS()
class US_API AUSTitlePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AUSTitlePlayerController();

	virtual void BeginPlay() override;

protected:
	/** "시작" → 게임 맵 열기 */
	void HandleStartRequested();

	/** "종료" → 게임 종료 */
	void HandleQuitRequested();

	/** 타이틀 위젯 (WBP, StartButton·QuitButton) */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSubclassOf<UUSTitleWidget> TitleWidgetClass;

	/** "시작" 버튼이 여는 게임 맵 */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSoftObjectPtr<UWorld> GameMap;

private:
	UPROPERTY()
	TObjectPtr<UUSTitleWidget> TitleWidget;
};
