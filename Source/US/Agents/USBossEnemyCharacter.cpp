#include "Agents/USBossEnemyCharacter.h"
#include "US.h"
#include "Core/USGameUserSettings.h"
#include "Camera/CameraShakeBase.h"
#include "GameFramework/PlayerController.h"
#include "Components/USAttributeComponent.h"
#include "Components/USDeathExplosionComponent.h"
#include "Components/USRingPulsePoolComponent.h"
#include "Datas/USBossData.h"
#include "Equipments/USEnemyProjectile.h"
#include "Spawning/USEnemySpawner.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/WidgetComponent.h"
#include "Datas/USEnemyTypeData.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AUSBossEnemyCharacter::AUSBossEnemyCharacter()
{
	TelegraphPool = CreateDefaultSubobject<UUSRingPulsePoolComponent>(TEXT("TelegraphPool"));

	// 보급선 이름표와 같은 설정: 화면 공간이라 카메라 각도와 상관없이 똑바로, 원하는 크기 그대로
	MarkerWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("MarkerWidget"));
	MarkerWidget->SetupAttachment(GetRootComponent());
	MarkerWidget->SetWidgetSpace(EWidgetSpace::Screen);
	MarkerWidget->SetDrawAtDesiredSize(true);
	MarkerWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AUSBossEnemyCharacter::RefreshMarker(float CurrentHealth, float MaxHealth) const
{
	UUserWidget* Marker = MarkerWidget ? MarkerWidget->GetUserWidgetObject() : nullptr;
	if (UProgressBar* Bar = Marker ? Cast<UProgressBar>(Marker->GetWidgetFromName(TEXT("HealthBar"))) : nullptr)
	{
		Bar->SetPercent(MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f);
	}
}

void AUSBossEnemyCharacter::SetSpawner(AUSEnemySpawner* InSpawner)
{
	Spawner = InSpawner;
}

float AUSBossEnemyCharacter::GetCurrentHealth() const
{
	return AttributeComponent ? AttributeComponent->GetCurrentHealth() : 0.0f;
}

float AUSBossEnemyCharacter::GetMaxHealth() const
{
	return AttributeComponent ? AttributeComponent->GetMaxHealth() : 0.0f;
}

void AUSBossEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AttributeComponent)
	{
		AttributeComponent->OnHealthChanged.AddUObject(this, &AUSBossEnemyCharacter::HandleHealthChanged);
	}

	if (MarkerWidget)
	{
		MarkerWidget->SetRelativeLocation(FVector(0.0f, 0.0f, MarkerHeight));

		// 이름은 종류 데이터에서 (체력바·클리어 창과 같은 이름)
		const UUSEnemyTypeData* Type = GetEnemyType();
		UUserWidget* Marker = MarkerWidget->GetUserWidgetObject();
		if (UTextBlock* Name = Marker ? Cast<UTextBlock>(Marker->GetWidgetFromName(TEXT("NameText"))) : nullptr)
		{
			if (Type)
			{
				Name->SetText(Type->GetDisplayName());
			}
		}
		else
		{
			UE_LOG(LogUS, Warning, TEXT("[%s] MarkerWidget has no NameText (set Widget Class to WBP_USBossMarker)."), *GetName());
		}
		RefreshMarker(GetCurrentHealth(), GetMaxHealth());
	}

	if (BossData == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] BossData is not set. The boss only chases and rams."), *GetName());
		return;
	}

	// 화면 밖에서 밀고 들어오므로 대장에게 다가오는 순간을 0.2초마다 본다 (매 프레임 거리 계산 대신)
	GetWorldTimerManager().SetTimer(ArrivalTimerHandle, this, &AUSBossEnemyCharacter::CheckArrival, 0.2f, true);

	// 등장 경고 띠가 지나간 뒤 첫 패턴. 월드 타이머라 일시정지·카드 중엔 같이 멈춘다
	GetWorldTimerManager().SetTimer(PatternTimerHandle, this, &AUSBossEnemyCharacter::RunNextPattern,
		FMath::Max(BossData->FirstPatternDelay, KINDA_SMALL_NUMBER), false);
}

void AUSBossEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(PatternTimerHandle);
	GetWorldTimerManager().ClearTimer(BarrageTimerHandle);
	GetWorldTimerManager().ClearTimer(DeathExplosionTimerHandle);
	GetWorldTimerManager().ClearTimer(ArrivalTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AUSBossEnemyCharacter::RunNextPattern()
{
	if (!IsAlive() || BossData == nullptr)
	{
		return;
	}

	++PatternCount;
	const bool bMinionTurn = BossData->MinionType && PatternCount % FMath::Max(BossData->MinionEveryNPatterns, 1) == 0;
	if (bMinionTurn)
	{
		StartMinions();
	}
	else
	{
		StartBarrage();
	}

	// 다음 패턴은 이번 패턴(경고 포함)이 끝난 뒤부터 센다 — 탄막 경고와 다음 패턴이 겹치지 않게
	const float Interval = BossData->PatternInterval * (bPhaseTwo ? BossData->PhaseTwoIntervalMultiplier : 1.0f);
	GetWorldTimerManager().SetTimer(PatternTimerHandle, this, &AUSBossEnemyCharacter::RunNextPattern,
		FMath::Max(Interval + BossData->BarrageTelegraphTime, 0.1f), false);
}

void AUSBossEnemyCharacter::StartBarrage()
{
	if (BossData->ProjectileClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] %s has no ProjectileClass. Barrage skipped."), *GetName(), *GetNameSafe(BossData));
		return;
	}

	// 경고 고리는 바닥(캡슐 아래)에 깐다. 보스가 움직여도 고리는 제자리라 "여기서 퍼진다"가 읽힌다
	if (TelegraphPool && TelegraphPool->HasMaterial())
	{
		const float HalfHeight = GetDefaultHalfHeight();
		TelegraphPool->PlayPulse(GetActorLocation() - FVector(0.0f, 0.0f, HalfHeight - 2.0f), BossData->BarrageTelegraphRadius);
	}

	BarrageBaseAngle = FMath::FRandRange(0.0f, 2.0f * PI);
	RingsRemaining = bPhaseTwo ? FMath::Max(BossData->PhaseTwoRings, 1) : 1;
	GetWorldTimerManager().SetTimer(BarrageTimerHandle, FTimerDelegate::CreateUObject(this, &AUSBossEnemyCharacter::FireRing, 0),
		FMath::Max(BossData->BarrageTelegraphTime, KINDA_SMALL_NUMBER), false);
}

void AUSBossEnemyCharacter::FireRing(int32 RingIndex)
{
	UWorld* World = GetWorld();
	if (World == nullptr || !IsAlive() || BossData == nullptr || BossData->ProjectileClass == nullptr)
	{
		return;
	}

	const int32 Count = FMath::Max(BossData->BulletsPerRing, 1);
	const float Step = 2.0f * PI / Count;
	const float Offset = BarrageBaseAngle + (RingIndex % 2 == 1 ? Step * 0.5f : 0.0f);
	const FVector Origin = GetActorLocation();

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Angle = Offset + Step * Index;
		const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
		if (AUSEnemyProjectile* Projectile = World->SpawnActor<AUSEnemyProjectile>(BossData->ProjectileClass, Origin, Direction.Rotation(), Params))
		{
			Projectile->Launch(Direction, BossData->BulletSpeed, BossData->BulletDamage, BossData->BulletLifeSeconds);
		}
	}

	if (BossData->BarrageSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BossData->BarrageSound, Origin);
	}
	PlayBossShake(BossData->BarrageShakeScale);

	--RingsRemaining;
	if (RingsRemaining > 0)
	{
		GetWorldTimerManager().SetTimer(BarrageTimerHandle, FTimerDelegate::CreateUObject(this, &AUSBossEnemyCharacter::FireRing, RingIndex + 1),
			FMath::Max(BossData->RingDelay, KINDA_SMALL_NUMBER), false);
	}
}

void AUSBossEnemyCharacter::StartMinions()
{
	AUSEnemySpawner* SpawnerPtr = Spawner.Get();
	if (SpawnerPtr == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] No spawner. Minions skipped."), *GetName());
		return;
	}

	SpawnerPtr->SpawnMinions(BossData->MinionType, GetActorLocation(), BossData->MinionCount, BossData->MinionSpawnRadius,
		BossData->MinionHealthMultiplier);
}

void AUSBossEnemyCharacter::HandleHealthChanged(float CurrentHealth, float MaxHealth)
{
	OnBossHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	RefreshMarker(CurrentHealth, MaxHealth);

	if (!bPhaseTwo && BossData && MaxHealth > 0.0f && CurrentHealth > 0.0f && CurrentHealth / MaxHealth <= BossData->PhaseTwoHealthRatio)
	{
		bPhaseTwo = true;
		UE_LOG(LogUS, Log, TEXT("[%s] Phase two at %.0f / %.0f HP"), *GetName(), CurrentHealth, MaxHealth);
	}
}

void AUSBossEnemyCharacter::CheckArrival()
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!IsAlive() || BossData == nullptr || Player == nullptr)
	{
		return;
	}

	if (FVector::Dist2D(GetActorLocation(), Player->GetActorLocation()) > BossData->ArrivalDistance)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ArrivalTimerHandle);
	PlayBossShake(BossData->ArrivalShakeScale);
	if (BossData->ArrivalSound)
	{
		UGameplayStatics::PlaySound2D(this, BossData->ArrivalSound);
	}
	UE_LOG(LogUS, Log, TEXT("[%s] Arrived near the leader"), *GetName());
}

void AUSBossEnemyCharacter::PlayBossShake(float Scale) const
{
	if (BossData == nullptr || BossData->ShakeClass == nullptr || Scale <= 0.0f)
	{
		return;
	}

	const UUSGameUserSettings* Settings = UUSGameUserSettings::Get();
	if (Settings && !Settings->IsScreenShakeEnabled())
	{
		return;
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->ClientStartCameraShake(BossData->ShakeClass, Scale);
	}
}

void AUSBossEnemyCharacter::HandleDeath()
{
	GetWorldTimerManager().ClearTimer(PatternTimerHandle);
	GetWorldTimerManager().ClearTimer(BarrageTimerHandle);
	GetWorldTimerManager().ClearTimer(ArrivalTimerHandle);

	Super::HandleDeath();

	if (MarkerWidget)
	{
		MarkerWidget->SetVisibility(false);
	}

	// 몸 크기만큼 여기저기 이어 터져야 "큰 배가 부서졌다"가 보인다. 기본 폭발 한 번은 Super가 이미 냈다
	DeathExplosionsRemaining = BossData ? BossData->DeathExplosionCount : 0;
	if (DeathExplosionsRemaining > 0)
	{
		GetWorldTimerManager().SetTimer(DeathExplosionTimerHandle, this, &AUSBossEnemyCharacter::PlayDeathExplosionStep,
			BossData->DeathExplosionInterval, true);
	}
	else
	{
		PlayFinalDeathSound();
	}
}

void AUSBossEnemyCharacter::PlayFinalDeathSound() const
{
	if (BossData && BossData->FinalDeathSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BossData->FinalDeathSound, GetFeedbackLocation());
	}
}

void AUSBossEnemyCharacter::PlayDeathExplosionStep()
{
	if (DeathExplosionsRemaining <= 0 || DeathExplosion == nullptr || BossData == nullptr)
	{
		GetWorldTimerManager().ClearTimer(DeathExplosionTimerHandle);
		return;
	}

	--DeathExplosionsRemaining;
	const FVector2D Offset = FMath::RandPointInCircle(BossData->DeathExplosionSpread);
	DeathExplosion->PlayExplosion(GetFeedbackLocation() + FVector(Offset.X, Offset.Y, 0.0f));

	// 마지막 연쇄와 함께 큰 한 방으로 마무리
	if (DeathExplosionsRemaining == 0)
	{
		PlayFinalDeathSound();
	}
}
