#include "Agents/USAgent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/USAttributeComponent.h"
#include "AI/Navigation/AvoidanceManager.h"
#include "GameFramework/CharacterMovementComponent.h"

AUSAgent::AUSAgent()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// 컨트롤러 회전은 사용하지 않고, 이동 방향으로 메시가 회전하게 한다.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->bOrientRotationToMovement = true;
	}
	
	AttributeComponent = CreateDefaultSubobject<UUSAttributeComponent>(TEXT("Attribute"));
}

void AUSAgent::BeginPlay()
{
	Super::BeginPlay();
}

void AUSAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AUSAgent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AUSAgent::HandleDeath()
{
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();

		// 이동 모드가 None이어도 CharacterMovement는 틱마다 RVO 등록을 갱신하므로, 그대로 두면 죽은 자리에
		// 보이지 않는 장애물이 남아 산 에이전트가 돌아간다. 회피를 끄고 관리자 항목도 바로 지운다
		// (SetAvoidanceEnabled(false)만으로는 자동 정리 모드에서 항목이 DefaultTimeToLive 동안 남는다)
		if (MovementComponent->bUseRVOAvoidance)
		{
			const int32 AvoidanceUID = MovementComponent->GetRVOAvoidanceUID();
			MovementComponent->SetAvoidanceEnabled(false);

			UWorld* World = GetWorld();
			UAvoidanceManager* AvoidanceManager = World ? World->GetAvoidanceManager() : nullptr;
			if (AvoidanceManager)
			{
				AvoidanceManager->RemoveAvoidanceObject(AvoidanceUID);
			}
		}
	}
	
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision); // 사망시 충돌 비활성화
	}
}

USceneComponent* AUSAgent::GetEquipmentAttachParent() const
{
	return GetMesh();
}

bool AUSAgent::GetMuzzleWorldLocation(int32 ShotIndex, FVector& OutLocation) const
{
	return false;
}
