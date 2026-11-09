#include "Core/USChainExplosionSubsystem.h"
#include "Agents/USEnemyCharacter.h"
#include "Core/USTeamSubsystem.h"
#include "Core/USFXBudgetSubsystem.h"
#include "US.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace
{
	/** 대기열 확인 간격. 지연 0.1초를 눈에 띄게 어기지 않을 만큼 촘촘하게 */
	constexpr float ProcessInterval = 1.0f / 30.0f;
}

void UUSChainExplosionSubsystem::Configure(const FUSChainExplosionSettings& InSettings)
{
	Settings = InSettings;
	bWarnedMissingFX = false;
}

void UUSChainExplosionSubsystem::TryChainExplosion(const FVector& Location, float SourceMaxHealth)
{
	if (Settings.Chance <= 0.0f || FMath::FRand() >= Settings.Chance)
	{
		return;
	}

	QueueExplosion(Location, SourceMaxHealth * Settings.DamageRatio, Settings.Radius, 1.0f, Settings.Delay);
}

void UUSChainExplosionSubsystem::QueueExplosion(const FVector& Location, float Damage, float Radius, float FXScaleMultiplier, float Delay)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	FPendingExplosion& Explosion = Pending.AddDefaulted_GetRef();
	Explosion.Location = Location;
	Explosion.Damage = Damage;
	Explosion.Radius = ScaleRadius(Radius);
	Explosion.FXScaleMultiplier = FXScaleMultiplier;
	Explosion.ExplodeTime = World->GetTimeSeconds() + Delay;

	// 터질 것이 있을 때만 타이머를 돌린다 (평소엔 비용 0)
	if (!World->GetTimerManager().IsTimerActive(ProcessTimerHandle))
	{
		World->GetTimerManager().SetTimer(ProcessTimerHandle, this, &UUSChainExplosionSubsystem::ProcessQueue, ProcessInterval, true);
	}
}

void UUSChainExplosionSubsystem::ProcessQueue()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// 터뜨리는 중에 죽은 적이 새 폭발을 예약해 Pending이 바뀌므로, 이번에 터질 것을 먼저 꺼내 둔다
	const double Now = World->GetTimeSeconds();
	TArray<FPendingExplosion, TInlineAllocator<16>> Due;
	for (int32 Index = 0; Index < Pending.Num() && Due.Num() < Settings.MaxExplosionsPerStep;)
	{
		if (Pending[Index].ExplodeTime <= Now)
		{
			Due.Add(Pending[Index]);
			Pending.RemoveAt(Index);
		}
		else
		{
			++Index;
		}
	}

	for (const FPendingExplosion& Explosion : Due)
	{
		Explode(Explosion);
	}

	if (Pending.IsEmpty())
	{
		World->GetTimerManager().ClearTimer(ProcessTimerHandle);
	}
}

void UUSChainExplosionSubsystem::Explode(const FPendingExplosion& Explosion)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	if (Settings.Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(World, Settings.Sound, Explosion.Location);
	}

	if (Settings.FX)
	{
		// 연달아 터지므로 동시 개수 상한을 거친다 (풀링·LOD 고정도 서브시스템이 한다). 상한이면 가벼운 판, 피해는 그대로
		if (UUSFXBudgetSubsystem* Budget = World->GetSubsystem<UUSFXBudgetSubsystem>())
		{
			Budget->SpawnBudgetedEmitter(Settings.FX, Explosion.Location, FVector(Settings.FXScale * Explosion.FXScaleMultiplier));
		}
	}
	else if (!bWarnedMissingFX)
	{
		bWarnedMissingFX = true;
		UE_LOG(LogUS, Warning, TEXT("ChainExplosion: FX is not set on the player controller; explosions deal damage without visuals"));
	}

	UUSTeamSubsystem* Team = World->GetSubsystem<UUSTeamSubsystem>();
	if (Team == nullptr || Explosion.Damage <= 0.0f)
	{
		return;
	}

	// 피해로 적이 죽으면 등록부 배열이 바뀌므로 복사본을 돈다 (EMP 펄스와 같은 이유)
	const TArray<TObjectPtr<AActor>> Enemies = Team->GetEnemies();
	const float RadiusSq = FMath::Square(Explosion.Radius);
	for (AActor* Actor : Enemies)
	{
		AUSEnemyCharacter* Enemy = Cast<AUSEnemyCharacter>(Actor);
		if (Enemy == nullptr || !Enemy->IsAlive())
		{
			continue;
		}

		// 바닥 높이 차이는 무시하고 평면 거리로 판정한다
		if (FVector::DistSquared2D(Explosion.Location, Enemy->GetActorLocation()) <= RadiusSq)
		{
			// 가해자 컨트롤러가 없으므로 마킹 보너스(부하 피해 전용)는 붙지 않는다
			UGameplayStatics::ApplyDamage(Enemy, Explosion.Damage, nullptr, nullptr, UDamageType::StaticClass());
		}
	}
}
