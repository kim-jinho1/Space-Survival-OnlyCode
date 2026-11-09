#include "Components/USPlayerInputComponent.h"
#include "Agents/USPlayerCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "GameFramework/PlayerController.h"

UUSPlayerInputComponent::UUSPlayerInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUSPlayerInputComponent::BindInputActions(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	AUSPlayerCharacter* Character = Cast<AUSPlayerCharacter>(GetOwner());

	if (EnhancedInputComponent == nullptr || Character == nullptr)
		return;

	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, Character, &AUSPlayerCharacter::Input_Move);
	EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, Character, &AUSPlayerCharacter::Input_Zoom);

	EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Triggered, Character, &AUSPlayerCharacter::Input_Run);
	EnhancedInputComponent->BindAction(RunAction, ETriggerEvent::Completed, Character, &AUSPlayerCharacter::StopRunning);

	// 누른 순간 = 표적 선택(자동 조준), 누르고 있는 동안 = 연사(자동 사격이 꺼져 있을 때만)
	EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, Character, &AUSPlayerCharacter::Input_SelectTarget);
	EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, Character, &AUSPlayerCharacter::Input_Fire);

	EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, Character, &AUSPlayerCharacter::Input_Interact);
}

void UUSPlayerInputComponent::AddMappingContext()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr)
		return;

	APlayerController* PlayerController = Cast<APlayerController>(OwnerPawn->GetController());
	if (PlayerController == nullptr)
		return;

	auto* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer());
	if (Subsystem)
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}
