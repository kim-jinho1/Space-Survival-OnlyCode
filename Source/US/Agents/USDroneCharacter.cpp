#include "Agents/USDroneCharacter.h"
#include "US.h"
#include "Core/USTeamSubsystem.h"
#include "USGameplayTags.h"
#include "Components/USAttributeComponent.h"
#include "Components/USCombatComponent.h"
#include "Components/USStateComponent.h"
#include "Components/USHoverVisualComponent.h"
#include "Components/USThrusterFXComponent.h"
#include "Components/USDeathExplosionComponent.h"
#include "Components/USRingPulsePoolComponent.h"
#include "Components/USHitFlashComponent.h"
#include "Equipments/USWeapon.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

AUSDroneCharacter::AUSDroneCharacter()
{
	StateComponent = CreateDefaultSubobject<UUSStateComponent>(TEXT("State"));
	CombatComponent = CreateDefaultSubobject<UUSCombatComponent>(TEXT("Combat"));

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(GetRootComponent());

	// 외형 전용: 피격 판정은 캡슐이 맡으므로 충돌·내비 영향 없음
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(VisualRoot);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetCanEverAffectNavigation(false);

	HoverVisual = CreateDefaultSubobject<UUSHoverVisualComponent>(TEXT("HoverVisual"));
	Thruster = CreateDefaultSubobject<UUSThrusterFXComponent>(TEXT("Thruster"));
	DeathExplosion = CreateDefaultSubobject<UUSDeathExplosionComponent>(TEXT("DeathExplosion"));

	// 격추된 기체는 부활·교체 전까지 액터가 남으므로 0.45초 연출이 끊기지 않는다. 판은 처음 죽을 때 1장만 만든다
	DeathBurstPool = CreateDefaultSubobject<UUSRingPulsePoolComponent>(TEXT("DeathBurstPool"));
	DeathBurstPool->SetupAttachment(GetRootComponent());
	DeathBurstPool->SetPoolSize(1);

	HitFlash = CreateDefaultSubobject<UUSHitFlashComponent>(TEXT("HitFlash"));

	WeaponMount = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponMount"));
	WeaponMount->SetupAttachment(VisualRoot);

	// 기본 무기 클래스 (BP 미지정 시에도 라이플이 스폰되도록)
	DefaultWeaponClass = AUSWeapon::StaticClass();
}

void AUSDroneCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 컴포넌트 BeginPlay(액터 BeginPlay의 Super 안에서 먼저 호출됨)보다 앞서 대상을 넘겨야 한다.
	// 이 시점의 VisualRoot 상대 트랜스폼은 BP에서 잡은 값이다.
	if (HoverVisual)
	{
		HoverVisual->SetVisualTarget(VisualRoot);
	}

	// 불꽃도 VisualRoot에 붙여야 둥실·기울기를 따라간다. VisualMesh에 붙이면 메시 스케일(4)까지 곱해진다.
	if (Thruster)
	{
		Thruster->SetAttachParent(VisualRoot);
	}

	if (HitFlash)
	{
		HitFlash->SetFlashTarget(VisualMesh);
	}

	ApplySlotMaterials();
}

void AUSDroneCharacter::ApplySlotMaterials()
{
	ApplySlotMaterialMap(SlotMaterials);
}

void AUSDroneCharacter::ApplySlotMaterialMap(const TMap<FName, TObjectPtr<UMaterialInterface>>& Materials)
{
	if (VisualMesh == nullptr)
	{
		return;
	}

	// 에셋 인스턴스를 그대로 꽂는다. 드론마다 동적 머티리얼을 만들지 않아 같은 편끼리 렌더 상태를 공유한다.
	for (const TPair<FName, TObjectPtr<UMaterialInterface>>& Pair : Materials)
	{
		if (Pair.Value == nullptr)
		{
			UE_LOG(LogUS, Warning, TEXT("%s: slot material '%s' is empty"), *GetName(), *Pair.Key.ToString());
			continue;
		}

		const int32 SlotIndex = VisualMesh->GetMaterialIndex(Pair.Key);
		if (SlotIndex == INDEX_NONE)
		{
			UE_LOG(LogUS, Warning, TEXT("%s: VisualMesh has no material slot '%s'; material not applied"),
				*GetName(), *Pair.Key.ToString());
			continue;
		}

		VisualMesh->SetMaterial(SlotIndex, Pair.Value);
	}
}

void AUSDroneCharacter::SetVisualMesh(UStaticMesh* NewMesh, const FTransform& RelativeTransform, const TMap<FName, TObjectPtr<UMaterialInterface>>& ExtraSlotMaterials)
{
	if (VisualMesh == nullptr || NewMesh == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: SetVisualMesh needs a visual mesh component and a mesh"), *GetName());
		return;
	}

	// 덮어쓴 재질은 슬롯 번호로 저장되므로, 그대로 두면 새 모델의 엉뚱한 슬롯에 칠해진다
	VisualMesh->SetStaticMesh(NewMesh);
	VisualMesh->EmptyOverrideMaterials();
	VisualMesh->SetRelativeTransform(RelativeTransform);
	ApplySlotMaterials();
	ApplySlotMaterialMap(ExtraSlotMaterials);
}

void AUSDroneCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AttributeComponent)
	{
		AttributeComponent->OnDeath.AddUObject(this, &AUSDroneCharacter::HandleDeath);
		AttributeComponent->OnStatsChanged.AddUObject(this, &AUSDroneCharacter::HandleStatsChanged);
		AttributeComponent->OnDamaged.AddUObject(this, &AUSDroneCharacter::HandleDamaged);
	}

	// 적이 타겟으로 삼을 수 있도록 아군 등록부에 올린다 (대장·부하 공통)
	if (UUSTeamSubsystem* Team = GetWorld() ? GetWorld()->GetSubsystem<UUSTeamSubsystem>() : nullptr)
	{
		Team->RegisterAlly(this);
	}

	EquipDefaultWeapon();
}

void AUSDroneCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UUSTeamSubsystem* Team = GetWorld() ? GetWorld()->GetSubsystem<UUSTeamSubsystem>() : nullptr)
	{
		Team->UnregisterAlly(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AUSDroneCharacter::EquipDefaultWeapon()
{
	if (DefaultWeaponClass == nullptr || CombatComponent == nullptr)
	{
		return;
	}

	// 이미 무기를 장착한 경우 중복 스폰 방지
	if (CombatComponent->GetMainWeapon() != nullptr)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Owner를 지정해 스폰해야 EquipItem()이 CombatComponent를 찾을 수 있다.
	AUSWeapon* SpawnedWeapon = World->SpawnActor<AUSWeapon>(DefaultWeaponClass, GetActorTransform(), SpawnParams);
	if (SpawnedWeapon)
	{
		SpawnedWeapon->EquipItem();
	}
}

USceneComponent* AUSDroneCharacter::GetEquipmentAttachParent() const
{
	return WeaponMount;
}

bool AUSDroneCharacter::GetMuzzleWorldLocation(int32 ShotIndex, FVector& OutLocation) const
{
	if (MuzzleOffsets.IsEmpty() || WeaponMount == nullptr)
	{
		return false;
	}

	// WeaponMount 기준이라 둥실·기울기를 따라간다
	const int32 Index = FMath::Abs(ShotIndex) % MuzzleOffsets.Num();
	OutLocation = WeaponMount->GetComponentTransform().TransformPosition(MuzzleOffsets[Index]);
	return true;
}

bool AUSDroneCharacter::IsDeath() const
{
	if (StateComponent == nullptr)
	{
		return false;
	}

	FGameplayTagContainer CheckTags;
	CheckTags.AddTag(USGameplayTags::Character_State_Death);

	return StateComponent->IsCurrentStateEqualToAny(CheckTags);
}

bool AUSDroneCharacter::CanAttack() const
{
	if (IsDeath() || StateComponent == nullptr)
	{
		return false;
	}

	// 재장전/구르기/피격/일반행동 중에는 발사 불가
	FGameplayTagContainer BlockingTags;
	BlockingTags.AddTag(USGameplayTags::Character_State_Rolling);
	BlockingTags.AddTag(USGameplayTags::Character_State_GeneralAction);
	BlockingTags.AddTag(USGameplayTags::Character_State_Hit);

	return StateComponent->IsCurrentStateEqualToAny(BlockingTags) == false;
}

bool AUSDroneCharacter::IsAiming() const
{
	return CombatComponent && CombatComponent->GetMainWeapon() != nullptr;
}

float AUSDroneCharacter::GetEffectiveRapidFireInterval() const
{
	const float Multiplier = AttributeComponent ? AttributeComponent->GetAttackSpeedMultiplier() : 1.0f;

	// 공속 배율이 비정상(0 이하)이면 기본 간격으로 폴백
	if (Multiplier <= 0.0f)
	{
		return RapidFireInterval;
	}

	return RapidFireInterval / Multiplier;
}

void AUSDroneCharacter::TryFire(const FVector& AimLocation, const FGameplayTag& AttackType)
{
	if (!CanAttack())
	{
		return;
	}

	AUSWeapon* Weapon = CombatComponent ? CombatComponent->GetMainWeapon() : nullptr;
	if (Weapon == nullptr)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// 연사 간격 제어 (공속 배율 반영)
	const float Now = World->GetTimeSeconds();
	if (Now - LastFireTime < GetEffectiveRapidFireInterval())
	{
		return;
	}

	LastFireTime = Now;

	Weapon->Fire(AimLocation, AttackType);
}

void AUSDroneCharacter::HandleDeath()
{
	// 죽은 드론은 더 이상 적의 타겟이 아니다
	if (UUSTeamSubsystem* Team = GetWorld() ? GetWorld()->GetSubsystem<UUSTeamSubsystem>() : nullptr)
	{
		Team->UnregisterAlly(this);
	}

	if (StateComponent)
	{
		StateComponent->SetCurrentState(USGameplayTags::Character_State_Death);
		StateComponent->ToggleMovementInput(false);
	}

	Super::HandleDeath();

	// 폭발이 기체를 대신한다. 액터는 지우지 않고 남겨 둔다 (부하는 P0-5 부활에서 다시 쓴다).
	if (VisualMesh)
	{
		VisualMesh->SetVisibility(false);
	}

	// 보이지 않는 기체의 둥실·기울기 계산과 추진 불꽃을 멈춘다
	if (HoverVisual)
	{
		HoverVisual->SetComponentTickEnabled(false);
	}

	if (Thruster)
	{
		Thruster->StopThrusters();
	}

	// 둥실 연출이 적용된 실제 기체 위치에서 터뜨린다
	const FVector BurstLocation = VisualRoot ? VisualRoot->GetComponentLocation() : GetActorLocation();
	if (DeathExplosion)
	{
		DeathExplosion->PlayExplosion(BurstLocation);
	}

	if (DeathBurstPool && DeathBurstPool->HasMaterial())
	{
		DeathBurstPool->PlayPulse(BurstLocation, DeathBurstRadius);
	}
}

void AUSDroneCharacter::HandleStatsChanged()
{
	// 기본 구현 없음. 자식이 이속 등 재계산에 사용한다.
}

void AUSDroneCharacter::HandleDamaged(float Damage, AController* InstigatedBy)
{
	if (HitFlash)
	{
		HitFlash->PlayFlash();
	}
}
