#include "Agents/USTitlePlayerController.h"
#include "Core/USLevelTravelSubsystem.h"
#include "US.h"
#include "UI/USTitleWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

AUSTitlePlayerController::AUSTitlePlayerController()
{
	bShowMouseCursor = true;
}

void AUSTitlePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (TitleWidgetClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] TitleWidgetClass is not set. Title screen will be empty."), *GetName());
		return;
	}

	TitleWidget = CreateWidget<UUSTitleWidget>(this, TitleWidgetClass);
	if (TitleWidget == nullptr)
	{
		return;
	}

	TitleWidget->OnStartRequested.BindUObject(this, &AUSTitlePlayerController::HandleStartRequested);
	TitleWidget->OnQuitRequested.BindUObject(this, &AUSTitlePlayerController::HandleQuitRequested);
	TitleWidget->AddToViewport();

	// 타이틀에는 조종할 폰이 없으므로 입력을 전부 UI로 보낸다.
	// UserWidget은 기본이 포커스 불가라, 허용하지 않고 포커스를 주면 "Attempting to focus Non-Focusable widget" 에러가 난다
	TitleWidget->SetIsFocusable(true);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(TitleWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AUSTitlePlayerController::HandleStartRequested()
{
	if (GameMap.IsNull())
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] GameMap is not set. Cannot start the game."), *GetName());
		return;
	}

	UUSLevelTravelSubsystem::TravelFrom(this, GameMap);
}

void AUSTitlePlayerController::HandleQuitRequested()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
