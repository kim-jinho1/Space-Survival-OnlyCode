#include "Agents/USPlayerCharacter.h"
#include "USGameplayTags.h"
#include "US.h"
#include "Components/USStateComponent.h"
#include "Components/USCombatComponent.h"
#include "Components/USAttributeComponent.h"
#include "Components/USCurrencyComponent.h"
#include "Components/USAimComponent.h"
#include "Components/USPlayerInputComponent.h"
#include "Components/USExperienceComponent.h"
#include "Components/USSquadComponent.h"
#include "Components/USMissileBarrageComponent.h"
#include "Components/USRingPulsePoolComponent.h"
#include "Components/USTargetMarkComponent.h"
#include "Components/USTargetRingComponent.h"
#include "Components/USThrusterFXComponent.h"
#include "Equipments/USWeapon.h"
#include "Shop/USSupplyShip.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"

AUSPlayerCharacter::AUSPlayerCharacter()
{
	// 쿼터뷰 고정 카메라: 컨트롤 회전·폰 회전 어느 쪽도 따라가지 않고 월드 기준 각도를 유지한다.
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = NormalArmLength;
	SpringArm->SetRelativeRotation(FRotator(CameraPitch, 0.0f, 0.0f));
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bInheritRoll = false;
	// 위에서 내려다보는 카메라가 천장·벽에 밀려 튀지 않도록 충돌 테스트를 끈다 (기획서 "카메라 충돌 무시").
	SpringArm->bDoCollisionTest = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm);

	// Character Movement — 캐릭터 회전은 이동 방향이 아니라 조준 방향(AimComponent)이 담당한다.
	GetCharacterMovement()->MaxWalkSpeed = NormalSpeed;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	GetCharacterMovement()->bOrientRotationToMovement = false;

	CurrencyComponent = CreateDefaultSubobject<UUSCurrencyComponent>(TEXT("Currency"));
	AimComponent = CreateDefaultSubobject<UUSAimComponent>(TEXT("Aim"));
	PlayerInputComponent = CreateDefaultSubobject<UUSPlayerInputComponent>(TEXT("PlayerInput"));
	ExperienceComponent = CreateDefaultSubobject<UUSExperienceComponent>(TEXT("Experience"));
	SquadComponent = CreateDefaultSubobject<UUSSquadComponent>(TEXT("Squad"));
	MissileBarrageComponent = CreateDefaultSubobject<UUSMissileBarrageComponent>(TEXT("MissileBarrage"));
	TargetMarkComponent = CreateDefaultSubobject<UUSTargetMarkComponent>(TEXT("TargetMark"));

	TargetRing = CreateDefaultSubobject<UUSTargetRingComponent>(TEXT("TargetRing"));
	TargetRing->SetupAttachment(GetRootComponent());

	// 6초에 최대 4발이라 4장이면 겹쳐도 끊기지 않는다
	SlowRing = CreateDefaultSubobject<UUSRingPulsePoolComponent>(TEXT("SlowRing"));
	SlowRing->SetupAttachment(GetRootComponent());
	SlowRing->SetPoolSize(4);
}

void AUSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	MissileBarrageComponent->SetSlowRing(SlowRing);

	// 카메라 거리·각도는 BP의 SpringArm 컴포넌트 값이 아니라 캐릭터 프로퍼티(NormalArmLength / CameraPitch)를 단일 소스로 쓴다.
	// BP가 컴포넌트 값을 오버라이드하고 있어도 여기서 덮어쓰므로 튜닝 지점이 한 곳으로 모인다.
	if (SpringArm)
	{
		SpringArm->TargetArmLength = NormalArmLength;
		SpringArm->SetRelativeRotation(FRotator(CameraPitch, 0.0f, 0.0f));
	}

	if (ExperienceComponent)
	{
		ExperienceComponent->OnLevelUp.AddUObject(this, &AUSPlayerCharacter::HandleLevelUp);
	}

	// 스탯 배율(기본 1.0)을 반영한 초기 이속 적용
	RefreshMoveSpeed();

	// 배경 음악 시작 (루프 사운드면 계속 재생)
	if (BackgroundMusic)
	{
		UGameplayStatics::SpawnSound2D(this, BackgroundMusic);
	}
}

void AUSPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateCameraLead(DeltaTime);

	// 버튼을 누르고 있을 때(Triggered)와 같은 경로라, 매 프레임 불러도 TryFire가 연사 간격대로만 쏜다.
	// 자동 조준인데 표적이 없으면 쏘지 않는다 (빈 곳에 총소리·이펙트를 내지 않게)
	const bool bHasTarget = AimComponent == nullptr || !AimComponent->IsAutoAim() || AimComponent->HasValidAim();
	if (bAutoFire && bHasTarget && !IsDeath())
	{
		RapidAttack();
	}
}

void AUSPlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// 기본 매핑 컨텍스트 등록은 입력 컴포넌트에 위임
	if (PlayerInputComponent)
	{
		PlayerInputComponent->AddMappingContext();
	}
}

void AUSPlayerCharacter::SetupPlayerInputComponent(UInputComponent* InPlayerInputComponent)
{
	Super::SetupPlayerInputComponent(InPlayerInputComponent);

	// 액션 바인딩은 입력 컴포넌트에 위임
	if (PlayerInputComponent)
	{
		PlayerInputComponent->BindInputActions(InPlayerInputComponent);
	}
}

bool AUSPlayerCharacter::CanToggleCombat() const
{
	if (StateComponent == nullptr)
	{
		return false;
	}

	FGameplayTagContainer CheckTags;
	CheckTags.AddTag(USGameplayTags::Character_State_Attacking);
	CheckTags.AddTag(USGameplayTags::Character_State_Rolling);
	CheckTags.AddTag(USGameplayTags::Character_State_GeneralAction);

	return StateComponent->IsCurrentStateEqualToAny(CheckTags) == false;
}

void AUSPlayerCharacter::Input_Move(const FInputActionValue& InputValue)
{
	// 사망/이동 입력 잠금 상태에서는 이동 불가
	if (IsDeath())
	{
		return;
	}

	if (StateComponent && StateComponent->MovementInputEnabled() == false)
	{
		return;
	}

	const FVector2D MovementVector = InputValue.Get<FVector2D>();

	// 컨트롤 회전은 이제 조준 방향이라 이동 기준으로 쓰면 W가 "커서 쪽"이 된다.
	// 카메라 컴포넌트의 yaw를 기준으로 삼는다 — 카메라가 고정이면 월드 축과 같고, 맵마다 카메라를 틀어도 화면 기준 이동이 유지된다.
	// 주의: SpringArm 컴포넌트 자체의 회전은 bInheritYaw=false여도 부모(캡슐=조준 방향)를 따라 돈다.
	// 분리되는 것은 스프링암 끝 소켓(카메라가 붙는 곳)의 회전뿐이므로 반드시 Camera에서 읽어야 한다.
	const float CameraYaw = Camera ? Camera->GetComponentRotation().Yaw : 0.0f;
	const FRotator YawRotation(0.0f, CameraYaw, 0.0f);

	const FVector ForwardVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// 이동은 입력 방향으로 바로 가속한다 (회피 반응 우선, 뱀서라이크 표준)
	AddMovementInput(ForwardVector, MovementVector.X);
	AddMovementInput(RightVector, MovementVector.Y);

	// 표적이 있으면 UUSAimComponent가 몸을 조준 방향으로 돌린다. 없을 때만 기수를 이동 방향으로 MoveTurnRate씩 돌린다
	// (즉시 꺾지 않아 선회가 곡선으로 보이고, 뱅킹은 yaw 각속도를 보는 UUSHoverVisualComponent가 붙인다)
	if (AimComponent && AimComponent->HasValidAim())
	{
		return;
	}

	const FVector DesiredInput = ForwardVector * MovementVector.X + RightVector * MovementVector.Y;
	if (DesiredInput.IsNearlyZero())
	{
		return;
	}

	const float DesiredYaw = DesiredInput.Rotation().Yaw;
	const float NewYaw = (MoveTurnRate > 0.0f)
		? FMath::FixedTurn(GetActorRotation().Yaw, DesiredYaw, MoveTurnRate * GetWorld()->GetDeltaSeconds())
		: DesiredYaw;
	SetActorRotation(FRotator(0.0f, NewYaw, 0.0f));
}

void AUSPlayerCharacter::SetCameraFrozen(bool bFrozen)
{
	if (Camera == nullptr || SpringArm == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] No camera or spring arm. Camera freeze is skipped."), *GetName());
		return;
	}

	if (bFrozen)
	{
		Camera->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}
	else
	{
		Camera->AttachToComponent(SpringArm, FAttachmentTransformRules::SnapToTargetNotIncludingScale, USpringArmComponent::SocketName);
	}
}

FVector AUSPlayerCharacter::GetScreenUpDirection() const
{
	const FVector Forward = Camera ? Camera->GetForwardVector() : FVector::ForwardVector;
	const FVector Flat(Forward.X, Forward.Y, 0.0f);
	return Flat.IsNearlyZero() ? FVector::ForwardVector : Flat.GetSafeNormal();
}

void AUSPlayerCharacter::SetCameraFovOffset(float Degrees)
{
	if (Camera == nullptr)
	{
		return;
	}

	// 처음 바꿀 때의 값을 원래 값으로 기억한다 (BP에서 정한 시야각을 덮어쓰지 않게)
	if (!bBaseFieldOfViewCached)
	{
		BaseFieldOfView = Camera->FieldOfView;
		bBaseFieldOfViewCached = true;
	}
	Camera->SetFieldOfView(BaseFieldOfView + Degrees);
}

void AUSPlayerCharacter::UpdateCameraLead(float DeltaTime)
{
	if (SpringArm == nullptr)
	{
		return;
	}

	// 가는 쪽 화면을 더 보여 줘서 속도감과 시야를 같이 준다 (탑다운 슈터의 흔한 "카메라 리드").
	// 속도 그대로 쓰면 방향 전환마다 화면이 튀므로 보간한다. 죽으면 가운데로 돌아온다
	FVector TargetLead = FVector::ZeroVector;
	if (!IsDeath() && CameraLeadDistance > 0.0f && RunSpeed > 0.0f)
	{
		const FVector Velocity2D(GetVelocity().X, GetVelocity().Y, 0.0f);
		TargetLead = Velocity2D / RunSpeed * CameraLeadDistance;
		TargetLead = TargetLead.GetClampedToMaxSize(CameraLeadDistance);
	}

	CameraLeadOffset = FMath::VInterpTo(CameraLeadOffset, TargetLead, DeltaTime, CameraLeadInterpSpeed);
	SpringArm->TargetOffset = CameraLeadOffset;
}

void AUSPlayerCharacter::Input_Zoom(const FInputActionValue& InputValue)
{
	if (SpringArm == nullptr)
	{
		return;
	}

	// 휠 위(+) = 가까이, 휠 아래(-) = 멀리
	const float Axis = InputValue.Get<float>();
	const float NewLength = SpringArm->TargetArmLength - Axis * ZoomStep;
	SpringArm->TargetArmLength = FMath::Clamp(NewLength, MinArmLength, MaxArmLength);
}

void AUSPlayerCharacter::Input_Run()
{
	if (IsDeath() || Camera == nullptr)
	{
		return;
	}

	bIsRunning = true;
	RefreshMoveSpeed();

	// 조준 중에는 조준 FOV를 유지한다.
	if (!IsAiming())
	{
		Camera->FieldOfView = RunFieldOfVision;
	}
}

void AUSPlayerCharacter::StopRunning()
{
	if (Camera == nullptr)
	{
		return;
	}

	bIsRunning = false;
	RefreshMoveSpeed();

	if (!IsAiming())
	{
		Camera->FieldOfView = NormalFieldOfVision;
	}
}

void AUSPlayerCharacter::TryFireAtAim(const FGameplayTag& AttackType)
{
	// 조준점이 아직 없으면(커서가 뷰포트 밖 등) 캐릭터 전방 먼 지점으로 폴백
	FVector AimLocation = GetActorLocation() + GetActorForwardVector() * 10000.0f;
	if (AimComponent && AimComponent->HasValidAim())
	{
		AimLocation = AimComponent->GetAimLocation();
	}

	// 몸은 AimComponent가 TurnRate로 천천히 돌린다. 다 돌기 전에는 쏘지 않는다 (탄이 기수 옆·뒤로 나가지 않게).
	const FVector ToAim = AimLocation - GetActorLocation();
	const float YawError = FMath::Abs(FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, ToAim.Rotation().Yaw));
	if (YawError > FireConeHalfAngle)
	{
		return;
	}

	TryFire(AimLocation, AttackType);
}

void AUSPlayerCharacter::RapidAttack()
{
	TryFireAtAim(USGameplayTags::Character_Attack_Rapid);
}

void AUSPlayerCharacter::Input_Fire()
{
	// 자동 사격 중에는 Tick이 이미 쏜다. 여기서도 쏘면 같은 프레임에 두 번 시도할 뿐이라 막는다
	if (!bAutoFire)
	{
		RapidAttack();
	}
}

void AUSPlayerCharacter::Input_SelectTarget()
{
	if (IsDeath() || AimComponent == nullptr)
	{
		return;
	}

	AimComponent->SelectTargetUnderCursor();
}

void AUSPlayerCharacter::Input_Interact()
{
	if (IsDeath())
	{
		return;
	}

	// 키를 눌렀을 때만 도는 검색이고 보급선은 한 대라 비용이 작다. 상호작용 거리는 기체 크기를 아는 보급선이 정한다
	for (TActorIterator<AUSSupplyShip> It(GetWorld()); It; ++It)
	{
		if (It->IsInInteractRange(GetActorLocation()))
		{
			It->Interact(this);
			return;
		}
	}
}

void AUSPlayerCharacter::HandleDeath()
{
	Super::HandleDeath();

	// 죽은 뒤에도 커서를 따라 몸이 돌면 래그돌이 흔들리므로 조준 갱신을 멈춘다
	if (AimComponent)
	{
		AimComponent->SetComponentTickEnabled(false);
	}
}

void AUSPlayerCharacter::RefreshMoveSpeed()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (MovementComponent == nullptr)
	{
		return;
	}

	const float BaseSpeed = bIsRunning ? RunSpeed : NormalSpeed;
	const float Multiplier = AttributeComponent ? AttributeComponent->GetMoveSpeedMultiplier() : 1.0f;

	MovementComponent->MaxWalkSpeed = BaseSpeed * Multiplier;

	// 이속이 오른 만큼 추진 불꽃도 키운다 (불꽃은 ReferenceSpeed에서 최대라 속도만으로는 더 안 커진다).
	// 이속 배율 그대로(+5% → ×1.05)는 눈에 안 띄어서 늘어난 몫을 ThrusterSizePerMoveSpeed배 한다
	if (Thruster)
	{
		// 느려지는 경우(배율 < 1)까지 3배 하면 불꽃이 0으로 사라지므로 늘어난 몫만 본다
		Thruster->SetSizeMultiplier(1.0f + FMath::Max(0.0f, Multiplier - 1.0f) * ThrusterSizePerMoveSpeed);
	}
}

void AUSPlayerCharacter::HandleStatsChanged()
{
	Super::HandleStatsChanged();

	// 이속 배율 변경을 즉시 반영 (공격력/공속 배율은 발사 시 조회되므로 별도 갱신 불필요)
	RefreshMoveSpeed();
}

void AUSPlayerCharacter::HandleLevelUp()
{
	if (LevelUpSound)
	{
		UGameplayStatics::PlaySound2D(this, LevelUpSound);
	}
}
