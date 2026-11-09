#include "Agents/USAllyDroneCharacter.h"
#include "AI/USAllyAIController.h"
#include "Components/USSquadComponent.h"
#include "Components/USAttributeComponent.h"
#include "Components/USUpgradeVisualComponent.h"
#include "Datas/USAllyTypeData.h"
#include "US.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AUSAllyDroneCharacter::AUSAllyDroneCharacter()
{
	// 스폰되면 AI가 바로 빙의하도록
	AIControllerClass = AUSAllyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// ACharacter 기본값은 true인데, 그러면 AIController가 SetFocus로 돌린 컨트롤 회전이 매 프레임 폰 yaw를 덮어
	// FireAtTarget의 제한 속도 회전과 애님 Aim Offset이 무의미해진다. 컨트롤 회전(조준)과 몸 회전을 분리한다.
	bUseControllerRotationYaw = false;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = MoveSpeed;
		Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
		// 따라갈 때는 이동 방향을 본다. 교전 중엔 FireAtTarget 태스크가 잠시 끈다.
		Movement->bOrientRotationToMovement = true;
		// 부하끼리 겹치지 않게 RVO 회피 (CharacterMovement 내장)
		Movement->bUseRVOAvoidance = true;
		Movement->AvoidanceConsiderationRadius = 200.0f;
	}

	// 대장이 자기 분대에 몸으로 막히지 않도록 폰끼리는 통과. 적의 접촉 데미지는 거리 판정이라 영향 없음.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}

	UpgradeVisual = CreateDefaultSubobject<UUSUpgradeVisualComponent>(TEXT("UpgradeVisual"));
}

void AUSAllyDroneCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 분대가 능력을 적용하는 시점(FinishSpawning 직후)보다 앞서 대상을 넘긴다. 이 시점의 BP 메시·크기가 등급 0 모습
	if (UpgradeVisual)
	{
		UpgradeVisual->SetVisualTargets(VisualRoot, VisualMesh);
	}
}

void AUSAllyDroneCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 컴포넌트 BeginPlay(체력 초기화)가 Super 안에서 먼저 끝나므로 그 뒤에 종류 스탯으로 덮어쓴다
	ApplyAllyTypeStats();
	RefreshMoveSpeed();
}

void AUSAllyDroneCharacter::SetAllyType(const UUSAllyTypeData* InAllyType)
{
	AllyType = InAllyType;
}

void AUSAllyDroneCharacter::ApplyAllyTypeStats()
{
	if (AllyType == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: no ally type; using blueprint default stats"), *GetName());
		return;
	}

	MoveSpeed = AllyType->GetMoveSpeed();
	RapidFireInterval = AllyType->GetRapidFireInterval();

	if (AttributeComponent)
	{
		AttributeComponent->SetMaxHealth(AllyType->GetMaxHealth());
		AttributeComponent->InitAttackDamageMultiplier(AllyType->GetAttackDamageMultiplier());
	}
}

void AUSAllyDroneCharacter::InitSquadMember(AUSPlayerCharacter* InLeader, UUSSquadComponent* InSquad, int32 InSlotIndex)
{
	Leader = InLeader;
	Squad = InSquad;
	SlotIndex = InSlotIndex;
}

FVector AUSAllyDroneCharacter::GetSlotLocation() const
{
	if (Squad == nullptr)
	{
		return GetActorLocation();
	}

	return Squad->GetSlotWorldLocation(SlotIndex);
}

void AUSAllyDroneCharacter::HandleDeath()
{
	Super::HandleDeath();

	if (Squad)
	{
		Squad->HandleMemberDown(this);
	}

	// BT 정지 + 빙의 해제. 기체는 부모(AUSDroneCharacter)가 숨기고 폭발시키며, 액터는 남겨 둔다.
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* Brain = AI->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("Dead"));
		}
		AI->UnPossess();
	}
}

void AUSAllyDroneCharacter::HandleStatsChanged()
{
	Super::HandleStatsChanged();
	RefreshMoveSpeed();
}

void AUSAllyDroneCharacter::RefreshMoveSpeed()
{
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement == nullptr)
	{
		return;
	}

	const float Multiplier = AttributeComponent ? AttributeComponent->GetMoveSpeedMultiplier() : 1.0f;
	Movement->MaxWalkSpeed = MoveSpeed * Multiplier;
}
