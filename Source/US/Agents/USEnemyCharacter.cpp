#include "Agents/USEnemyCharacter.h"
#include "AI/USEnemyAIController.h"
#include "Core/USTeamSubsystem.h"
#include "Core/USCombatFeedbackSubsystem.h"
#include "Core/USChainExplosionSubsystem.h"
#include "Datas/USEnemyTypeData.h"
#include "Pickups/USPickup.h"
#include "US.h"
#include "Components/USAttributeComponent.h"
#include "Components/USHoverVisualComponent.h"
#include "Components/USHitFlashComponent.h"
#include "Components/USWorldHealthBarComponent.h"
#include "Components/USDeathExplosionComponent.h"
#include "Components/USRingPulsePoolComponent.h"
#include "Components/USThrusterFXComponent.h"
#include "Components/CapsuleComponent.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AUSEnemyCharacter::AUSEnemyCharacter()
{
	// AI가 빙의해 스폰되도록 설정 (배치/스폰 모두)
	AIControllerClass = AUSEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
		MovementComponent->MaxWalkSpeed = MoveSpeed;

		// 모두 같은 한 점(대장)을 쫓아서 한 덩어리로 뭉치던 것을 RVO 회피로 서로 비켜 가게 한다 (부하와 같은 CharacterMovement 내장 기능)
		MovementComponent->bUseRVOAvoidance = true;
		MovementComponent->AvoidanceConsiderationRadius = 150.0f;
	}

	// 무기 히트스캔(ECC_Visibility 라인트레이스)에 맞도록 캡슐이 Visibility를 Block 하게 한다.
	// 캡슐 크기는 BP에서 드론 크기에 맞춰 조정한다.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->InitCapsuleSize(30.0f, 40.0f);
		Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	// 적 외형은 스태틱 메시 드론이다. 수십 마리가 동시에 나오므로 스켈레탈 메시(뼈 계산·스키닝)는 쓰지 않고 숨긴다.
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetVisibility(false);
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 대장·부하 드론과 같은 외형 구성: VisualRoot(회전 중심) 아래 메시, HoverVisual이 VisualRoot를 둥실·기울인다.
	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(GetRootComponent());

	// 외형 전용: 피격 판정은 캡슐이 맡으므로 충돌·내비 영향 없음
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(VisualRoot);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetCanEverAffectNavigation(false);

	HoverVisual = CreateDefaultSubobject<UUSHoverVisualComponent>(TEXT("HoverVisual"));
	HitFlash = CreateDefaultSubobject<UUSHitFlashComponent>(TEXT("HitFlash"));

	// 캡슐(루트)에 붙여 둥실 연출에 흔들리지 않게 한다. 위치·크기·기울기는 BP에서 잡는다.
	HealthBar = CreateDefaultSubobject<UUSWorldHealthBarComponent>(TEXT("HealthBar"));
	HealthBar->SetupAttachment(GetRootComponent());

	DeathExplosion = CreateDefaultSubobject<UUSDeathExplosionComponent>(TEXT("DeathExplosion"));

	// 적마다 붙지만 판은 처음 죽을 때 1장만 만든다 (액터는 DeathLifeSpan 동안 남아 0.45초 연출이 끊기지 않음)
	DeathBurstPool = CreateDefaultSubobject<UUSRingPulsePoolComponent>(TEXT("DeathBurstPool"));
	DeathBurstPool->SetupAttachment(GetRootComponent());
	DeathBurstPool->SetPoolSize(1);
}

void AUSEnemyCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 컴포넌트 BeginPlay(액터 BeginPlay의 Super 안에서 먼저 호출됨)보다 앞서 대상을 넘겨야 한다.
	if (HoverVisual)
	{
		HoverVisual->SetVisualTarget(VisualRoot);
	}

	if (HitFlash)
	{
		HitFlash->SetFlashTarget(VisualMesh);
	}
}

void AUSEnemyCharacter::SetEnemyType(const UUSEnemyTypeData* InEnemyType, float InHealthMultiplier)
{
	EnemyType = InEnemyType;
	HealthMultiplier = FMath::Max(InHealthMultiplier, 0.01f);
}

void AUSEnemyCharacter::ApplyEnemyType()
{
	if (EnemyType == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: no enemy type; using blueprint default stats and mesh"), *GetName());
		return;
	}

	MoveSpeed = EnemyType->GetMoveSpeed();
	ContactDamage = EnemyType->GetContactDamage();
	ExperienceReward = EnemyType->GetExperienceReward();
	GoldReward = EnemyType->GetGoldReward();

	if (AttributeComponent)
	{
		AttributeComponent->SetMaxHealth(EnemyType->GetMaxHealth() * HealthMultiplier);
	}

	if (VisualMesh == nullptr)
	{
		return;
	}

	if (UStaticMesh* TypeMesh = EnemyType->GetMesh())
	{
		VisualMesh->SetStaticMesh(TypeMesh);
	}

	// BP에서 드론 크기에 맞춰 잡은 스케일 위에 종류 배율을 곱한다 (돌격 드론은 작게, 기뢰는 크게)
	VisualMesh->SetRelativeScale3D(VisualMesh->GetRelativeScale3D() * EnemyType->GetVisualScale());

	const TArray<TObjectPtr<UMaterialInterface>>& Materials = EnemyType->GetOverrideMaterials();
	for (int32 Index = 0; Index < Materials.Num(); ++Index)
	{
		if (Materials[Index])
		{
			VisualMesh->SetMaterial(Index, Materials[Index]);
		}
	}
}

void AUSEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	CombatFeedback = GetWorld() ? GetWorld()->GetSubsystem<UUSCombatFeedbackSubsystem>() : nullptr;

	// 컴포넌트 BeginPlay(체력 초기화)가 Super 안에서 먼저 끝나므로 그 뒤에 종류 스탯으로 덮어쓴다
	ApplyEnemyType();
	SpawnTypeTrails();
	RefreshMoveSpeed();

	// 적은 이미 쫓기 때문에 Tick이 돈다 → 타이머를 따로 두지 않고 Tick에서 키운다
	if (SpawnScaleInTime > 0.0f && VisualRoot)
	{
		bScalingIn = true;
		VisualRoot->SetRelativeScale3D(FVector(KINDA_SMALL_NUMBER));
	}

	if (AttributeComponent)
	{
		AttributeComponent->OnDeath.AddUObject(this, &AUSEnemyCharacter::HandleDeath);
		AttributeComponent->OnDamaged.AddUObject(this, &AUSEnemyCharacter::HandleDamaged);
	}

	// 접촉 데미지를 일정 주기로 판정한다.
	if (ContactDamageInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			ContactDamageTimerHandle,
			this,
			&AUSEnemyCharacter::ApplyContactDamage,
			ContactDamageInterval,
			true);
	}

	// 아군(대장·부하) 중 가장 가까운 쪽을 주기적으로 다시 고른다. 부하가 생기면 부하도 공격 대상이 된다.
	if (UUSTeamSubsystem* Team = GetWorld() ? GetWorld()->GetSubsystem<UUSTeamSubsystem>() : nullptr)
	{
		Team->RegisterEnemy(this);
	}

	RefreshTarget();
	if (TargetRefreshInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			TargetRefreshTimerHandle,
			this,
			&AUSEnemyCharacter::RefreshTarget,
			TargetRefreshInterval,
			true);
	}
}

void AUSEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UUSTeamSubsystem* Team = GetWorld() ? GetWorld()->GetSubsystem<UUSTeamSubsystem>() : nullptr)
	{
		Team->UnregisterEnemy(this);
	}

	Super::EndPlay(EndPlayReason);
}

void AUSEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ChaseTarget(DeltaTime);
	UpdateSpawnScale(DeltaTime);
}

void AUSEnemyCharacter::UpdateSpawnScale(float DeltaTime)
{
	if (!bScalingIn || VisualRoot == nullptr)
	{
		return;
	}

	SpawnScaleElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(SpawnScaleElapsed / SpawnScaleInTime, 0.0f, 1.0f);

	// ease-out-back: 살짝 넘쳤다가(약 1.1배) 제 크기로 — "튀어나온다"보다 "생겨난다"로 보이게
	constexpr float Overshoot = 1.70158f;
	const float T = Alpha - 1.0f;
	const float Scale = 1.0f + (Overshoot + 1.0f) * T * T * T + Overshoot * T * T;
	VisualRoot->SetRelativeScale3D(FVector(FMath::Max(KINDA_SMALL_NUMBER, Scale)));

	if (Alpha >= 1.0f)
	{
		bScalingIn = false;
	}
}

bool AUSEnemyCharacter::IsAlive() const
{
	return !bIsDead && AttributeComponent && AttributeComponent->IsAlive();
}

void AUSEnemyCharacter::AddPendingDamage(float Amount)
{
	PendingDamage = FMath::Max(0.0f, PendingDamage + Amount);
}

bool AUSEnemyCharacter::IsDoomed() const
{
	return PendingDamage > 0.0f && AttributeComponent && PendingDamage >= AttributeComponent->GetCurrentHealth();
}

FBox AUSEnemyCharacter::GetPickBounds() const
{
	// Bounds는 엔진이 이동할 때마다 갱신해 두므로 따로 계산하지 않는다
	if (VisualMesh && VisualMesh->GetStaticMesh())
	{
		return VisualMesh->Bounds.GetBox();
	}
	return GetCapsuleComponent()->Bounds.GetBox();
}

FVector AUSEnemyCharacter::GetClosestPickPoint(const FVector& From) const
{
	// 탑뷰라 높이는 무시한다 (FindEnemyAtPoint와 같은 규칙)
	const FBox Bounds = GetPickBounds();
	return Bounds.GetClosestPointTo(FVector(From.X, From.Y, Bounds.GetCenter().Z));
}

void AUSEnemyCharacter::RefreshTarget()
{
	UUSTeamSubsystem* Team = GetWorld() ? GetWorld()->GetSubsystem<UUSTeamSubsystem>() : nullptr;
	if (Team == nullptr)
	{
		CachedTarget = nullptr;
		return;
	}

	// 방패기 도발 반경 안이면 방패기를, 아니면 가장 가까운 아군을 노린다
	AActor* Taunter = Team->FindTaunterFor(GetActorLocation());
	CachedTarget = Taunter ? Taunter : Team->FindNearestAlly(GetActorLocation());
}

void AUSEnemyCharacter::ApplySlow(float Multiplier, float Duration)
{
	if (!IsAlive() || Duration <= 0.0f)
	{
		return;
	}

	SlowMultiplier = FMath::Min(SlowMultiplier, FMath::Clamp(Multiplier, 0.0f, 1.0f));
	RefreshMoveSpeed();
	RefreshOverlay();

	GetWorldTimerManager().SetTimer(SlowTimerHandle, this, &AUSEnemyCharacter::ClearSlow, Duration, false);
}

void AUSEnemyCharacter::ClearSlow()
{
	SlowMultiplier = 1.0f;
	RefreshMoveSpeed();
	RefreshOverlay();
}

void AUSEnemyCharacter::ApplyMark(float DamageBonus, float Duration)
{
	if (!IsAlive() || DamageBonus <= 0.0f || Duration <= 0.0f)
	{
		return;
	}

	// 이미 표시된 적은 시간만 다시 건다 (매 명중마다 머티리얼을 다시 넣지 않는다)
	const bool bWasMarked = IsMarked();
	MarkDamageBonus = FMath::Max(MarkDamageBonus, DamageBonus);
	if (!bWasMarked)
	{
		RefreshOverlay();
	}

	GetWorldTimerManager().SetTimer(MarkTimerHandle, this, &AUSEnemyCharacter::ClearMark, Duration, false);
}

void AUSEnemyCharacter::ClearMark()
{
	MarkDamageBonus = 0.0f;
	RefreshOverlay();
}

void AUSEnemyCharacter::RefreshOverlay()
{
	if (VisualMesh == nullptr)
	{
		return;
	}

	const bool bSlowed = IsAlive() && SlowMultiplier < 1.0f;
	UMaterialInterface* Overlay = IsMarked() ? MarkOverlayMaterial.Get() : (bSlowed ? SlowOverlayMaterial.Get() : nullptr);

	if (Overlay == nullptr && (IsMarked() || bSlowed))
	{
		// 적마다 찍으면 로그가 넘치므로 한 번만 알린다
		static bool bWarnedMissingOverlay = false;
		if (!bWarnedMissingOverlay)
		{
			UE_LOG(LogUS, Warning, TEXT("%s: Mark/SlowOverlayMaterial is empty; target marks or slows are invisible"), *GetClass()->GetName());
			bWarnedMissingOverlay = true;
		}
	}

	// 덧씌우기는 메시를 한 번 더 그리는 방식이라 적 종류별 원래 머티리얼(SetMaterial)과 피격 발광(CPD)을 건드리지 않는다
	VisualMesh->SetOverlayMaterial(Overlay);
}

float AUSEnemyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 부하 = 플레이어가 아닌 아군 컨트롤러(AI). 대장의 총알·미사일은 키우지 않는다 — 대장은 "표시하는 쪽", 부하가 "때리는 쪽"이다.
	// 연쇄 폭발은 가해자 컨트롤러가 없어(nullptr) 여기에 걸리지 않으므로 컨트롤러 종류만으로 구분된다
	const bool bSquadDamage = EventInstigator != nullptr && !EventInstigator->IsPlayerController();
	if (bSquadDamage && MarkDamageBonus > 0.0f)
	{
		DamageAmount *= 1.0f + MarkDamageBonus;
	}

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AUSEnemyCharacter::RefreshMoveSpeed()
{
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = MoveSpeed * SlowMultiplier;
	}
}

void AUSEnemyCharacter::ChaseTarget(float DeltaTime)
{
	if (!IsAlive())
		return;

	AActor* Target = GetTargetActor();
	if (Target == nullptr)
		return;

	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	const FVector Direction = ToTarget.GetSafeNormal2D();

	if (!Direction.IsNearlyZero())
	{
		AddMovementInput(Direction, 1.0f);
	}
}

void AUSEnemyCharacter::ApplyContactDamage()
{
	if (!IsAlive())
		return;

	AActor* Target = GetTargetActor();
	if (IsTargetInContactRange(Target))
	{
		UGameplayStatics::ApplyDamage(
			Target,
			ContactDamage,
			GetController(),
			this,
			UDamageType::StaticClass());
	}
}

bool AUSEnemyCharacter::IsTargetInContactRange(const AActor* Target) const
{
	if (Target == nullptr)
	{
		return false;
	}

	// 3D 거리로 판정 (바닥 아래/위로 떨어진 적이 수평상 가깝다고 때리는 것 방지)
	return FVector::Dist(GetActorLocation(), Target->GetActorLocation()) <= ContactRange;
}

void AUSEnemyCharacter::SpawnTypeTrails()
{
	UParticleSystem* TrailFX = EnemyType ? EnemyType->GetTrailFX() : nullptr;
	if (TrailFX == nullptr || VisualRoot == nullptr)
	{
		return;
	}

	// 기수가 +X라 꼬리 쪽(뒤)을 보게 돌린다 (UUSThrusterFXComponent::ThrusterRotation과 같다)
	const FRotator TrailRotation(0.0f, 180.0f, 0.0f);
	const TArray<FVector>& Offsets = EnemyType->GetTrailOffsets();
	TrailComponents.Reserve(Offsets.Num());
	for (const FVector& Offset : Offsets)
	{
		UParticleSystemComponent* Trail = UGameplayStatics::SpawnEmitterAttached(TrailFX, VisualRoot, NAME_None,
			Offset, TrailRotation, FVector(EnemyType->GetTrailScale()), EAttachLocation::KeepRelativeOffset, false);
		if (Trail)
		{
			UUSThrusterFXComponent::ForceTopLOD(Trail);
			TrailComponents.Add(Trail);
		}
	}
}

void AUSEnemyCharacter::HandleDeath()
{
	if (bIsDead)
		return;

	bIsDead = true;

	// 꼬리는 새로 그리지 않게만 끈다. 이미 나온 궤적은 자연스럽게 사라진다
	for (UParticleSystemComponent* Trail : TrailComponents)
	{
		if (Trail)
		{
			Trail->DeactivateSystem();
		}
	}

	// 사망 시 둥실·기울기 연출 정지
	if (HoverVisual)
	{
		HoverVisual->SetComponentTickEnabled(false);
	}

	// 폭발이 기체를 대신하므로 메시는 바로 숨긴다. 액터는 DeathLifeSpan 뒤에 제거된다.
	if (VisualMesh)
	{
		VisualMesh->SetVisibility(false);
	}

	if (DeathExplosion)
	{
		DeathExplosion->PlayExplosion(GetFeedbackLocation());
	}

	if (DeathBurstPool && DeathBurstPool->HasMaterial())
	{
		const float TypeScale = EnemyType ? EnemyType->GetVisualScale() : 1.0f;
		DeathBurstPool->PlayPulse(GetFeedbackLocation(), DeathBurstRadius * TypeScale);
	}

	RequestChainExplosion();

	if (HealthBar)
	{
		HealthBar->HideBar();
	}

	// 대장이 막타를 쳤을 때만 처치 연출(화면 흔들림)을 알린다. 부하가 잡을 때마다 흔들리면 화면이 계속 떨린다.
	if (bLastHitByPlayer && CombatFeedback)
	{
		CombatFeedback->ReportPlayerKilledEnemy(GetFeedbackLocation());
	}

	// 처치일 때만 보상 젬을 드롭하고(주운 플레이어가 획득 → 킬러 추적 불필요) 처치 수에 넣는다
	if (ShouldCountAsKill())
	{
		SpawnRewards();

		if (CombatFeedback)
		{
			CombatFeedback->ReportEnemyKilled();
		}
	}

	// 스포너 등 외부에 사망을 알린다.
	OnEnemyDeath.Broadcast(this);

	// 더 이상 추적/접촉 데미지를 발생시키지 않고, 아군의 타겟 후보에서도 빠진다.
	GetWorldTimerManager().ClearTimer(ContactDamageTimerHandle);
	GetWorldTimerManager().ClearTimer(TargetRefreshTimerHandle);
	GetWorldTimerManager().ClearTimer(SlowTimerHandle);
	GetWorldTimerManager().ClearTimer(MarkTimerHandle);
	SlowMultiplier = 1.0f;
	ClearMark();
	CachedTarget = nullptr;

	if (UUSTeamSubsystem* Team = GetWorld() ? GetWorld()->GetSubsystem<UUSTeamSubsystem>() : nullptr)
	{
		Team->UnregisterEnemy(this);
	}

	Super::HandleDeath();

	// AI 정지
	if (AController* AIController = GetController())
	{
		AIController->UnPossess();
	}

	// 일정 시간 뒤 자동 제거
	SetLifeSpan(DeathLifeSpan);
}

void AUSEnemyCharacter::HandleDamaged(float Damage, AController* InstigatedBy)
{
	// 사망 타격도 번쩍여야 해서 생존 여부와 무관하게 발광한다
	if (HitFlash)
	{
		HitFlash->PlayFlash();
	}

	const float CurrentHealth = AttributeComponent ? AttributeComponent->GetCurrentHealth() : 0.0f;
	const float MaxHealth = AttributeComponent ? AttributeComponent->GetMaxHealth() : 0.0f;

	// 살아있는 상태에서 데미지를 받았을 때만 피격음 (사망 타격은 DeathExplosion의 사망음이 담당)
	if (HitSound && CurrentHealth > 0.0f)
	{
		const float Pitch = FMath::FRandRange(1.0f - HitSoundPitchVariation, 1.0f + HitSoundPitchVariation);
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation(), 1.0f, Pitch);
	}

	if (HealthBar)
	{
		// 최대 체력 0은 비정상이라 바를 숨기는 쪽(비율 0)으로 둔다
		HealthBar->ShowHealthRatio(MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f);
	}

	// 막타를 누가 쳤는지는 사망 처리에서 쓴다. OnDamaged가 OnDeath보다 먼저 온다.
	bLastHitByPlayer = InstigatedBy != nullptr && InstigatedBy->IsPlayerController();

	// 데미지 숫자는 대장·부하 구분 없이 띄운다
	if (CombatFeedback)
	{
		CombatFeedback->ReportEnemyDamaged(GetFeedbackLocation(), Damage, bLastHitByPlayer);
	}
}

FVector AUSEnemyCharacter::GetFeedbackLocation() const
{
	// 둥실 연출로 캡슐 중심과 기체 위치가 달라서 보이는 기체 기준으로 잡는다
	return VisualRoot ? VisualRoot->GetComponentLocation() : GetActorLocation();
}

bool AUSEnemyCharacter::ShouldCountAsKill() const
{
	return true;
}

void AUSEnemyCharacter::RequestChainExplosion()
{
	UUSChainExplosionSubsystem* Chain = GetWorld() ? GetWorld()->GetSubsystem<UUSChainExplosionSubsystem>() : nullptr;
	if (Chain == nullptr)
	{
		return;
	}

	const float MaxHealth = AttributeComponent ? AttributeComponent->GetMaxHealth() : 0.0f;
	Chain->TryChainExplosion(GetFeedbackLocation(), MaxHealth);
}

void AUSEnemyCharacter::SpawnRewards() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
		return;

	const FVector BaseLocation = GetActorLocation();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 경험치 젬
	if (ExperienceReward > 0.0f)
	{
		UClass* SpawnClass = ExperiencePickupClass ? ExperiencePickupClass.Get() : AUSPickup::StaticClass();
		if (AUSPickup* Gem = World->SpawnActor<AUSPickup>(SpawnClass, BaseLocation, FRotator::ZeroRotator, SpawnParams))
		{
			Gem->InitPickup(EUSPickupType::Experience, ExperienceReward);
		}
	}

	// 골드 젬 (같은 자리에 스폰해도 AUSPickup이 무작위 방향으로 흩뿌린다)
	if (GoldReward > 0)
	{
		UClass* SpawnClass = GoldPickupClass ? GoldPickupClass.Get() : AUSPickup::StaticClass();
		if (AUSPickup* Gem = World->SpawnActor<AUSPickup>(SpawnClass, BaseLocation, FRotator::ZeroRotator, SpawnParams))
		{
			Gem->InitPickup(EUSPickupType::Gold, static_cast<float>(GoldReward));
		}
	}
}
