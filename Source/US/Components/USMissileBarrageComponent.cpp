#include "Components/USMissileBarrageComponent.h"
#include "US.h"
#include "Agents/USEnemyCharacter.h"
#include "Components/USAttributeComponent.h"
#include "Components/USCombatComponent.h"
#include "Components/USRingPulsePoolComponent.h"
#include "Core/USChainExplosionSubsystem.h"
#include "Core/USFXBudgetSubsystem.h"
#include "Core/USLocationUtils.h"
#include "Core/USTeamSubsystem.h"
#include "Engine/World.h"
#include "Equipments/USMissile.h"
#include "Equipments/USWeapon.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "TimerManager.h"

UUSMissileBarrageComponent::UUSMissileBarrageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUSMissileBarrageComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UUSAttributeComponent* Attribute = GetOwner() ? GetOwner()->FindComponentByClass<UUSAttributeComponent>() : nullptr)
	{
		Attribute->OnDeath.AddUObject(this, &UUSMissileBarrageComponent::HandleOwnerDeath);
	}
}

void UUSMissileBarrageComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 주기 타이머와 예약된 폭발(CreateWeakLambda로 이 컴포넌트에 묶임)을 한꺼번에 지운다
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	Super::EndPlay(EndPlayReason);
}

void UUSMissileBarrageComponent::HandleOwnerDeath()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
}

void UUSMissileBarrageComponent::SetSlowDurationBonus(float Bonus)
{
	SlowDurationBonus = FMath::Max(0.0f, Bonus);
}

void UUSMissileBarrageComponent::SetBarrage(int32 InTargetCount, float InSlowDuration)
{
	TargetCount = FMath::Max(0, InTargetCount);
	SlowDuration = FMath::Max(0.0f, InSlowDuration);

	if (ExplosionFX == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: MissileBarrage has no ExplosionFX; damage works but nothing is drawn"), *GetNameSafe(GetOwner()));
	}

	// 처음 배울 때만 타이머를 건다. 단계가 올라도 주기는 그대로이고 값만 바뀐다
	UWorld* World = GetWorld();
	if (World && TargetCount > 0 && !World->GetTimerManager().IsTimerActive(BarrageTimerHandle))
	{
		World->GetTimerManager().SetTimer(BarrageTimerHandle, this, &UUSMissileBarrageComponent::FireBarrage, Interval, true);
	}
}

void UUSMissileBarrageComponent::FireBarrage()
{
	UWorld* World = GetWorld();
	const UUSTeamSubsystem* Team = World ? World->GetSubsystem<UUSTeamSubsystem>() : nullptr;
	const AActor* Owner = GetOwner();
	if (Team == nullptr || Owner == nullptr || TargetCount <= 0)
	{
		return;
	}

	// 피해 기준 = 대장 주무기 한 발 (공격력 배율 포함). 무기가 없으면 쏘지 않는다
	const UUSCombatComponent* Combat = Owner->FindComponentByClass<UUSCombatComponent>();
	const AUSWeapon* Weapon = Combat ? Combat->GetMainWeapon() : nullptr;
	if (Weapon == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: MissileBarrage found no main weapon; barrage skipped"), *GetNameSafe(Owner));
		return;
	}
	const float Damage = Weapon->GetAttackDamage() * DamageMultiplier;

	// 사거리 안의 살아 있는 적을 거리순으로 TargetCount곳
	const FVector OwnerLocation = Owner->GetActorLocation();
	const float MaxRangeSq = FMath::Square(MaxRange);
	TArray<TPair<float, FVector>> Candidates;
	// 화면 밖 적에게는 쏘지 않는다 (포격이 보이게)
	const FUSScreenBounds Screen(GetWorld());
	for (AActor* Actor : Team->GetEnemies())
	{
		const AUSEnemyCharacter* Enemy = Cast<AUSEnemyCharacter>(Actor);
		if (Enemy == nullptr || !Enemy->IsAlive())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared2D(OwnerLocation, Enemy->GetActorLocation());
		if (DistSq <= MaxRangeSq && Screen.Contains(Enemy->GetActorLocation()))
		{
			Candidates.Emplace(DistSq, Enemy->GetActorLocation());
		}
	}

	Candidates.Sort([](const TPair<float, FVector>& A, const TPair<float, FVector>& B) { return A.Key < B.Key; });

	const int32 NumTargets = FMath::Min(TargetCount, Candidates.Num());
	UE_LOG(LogUS, Verbose, TEXT("[MissileBarrage] %s fires %d missiles (damage %.1f, slow x%.2f for %.1fs)"),
		*GetNameSafe(Owner), NumTargets, Damage, SlowMultiplier, SlowDuration);

	for (int32 Index = 0; Index < NumTargets; ++Index)
	{
		// 목표는 지금 위치로 고정한다 (적을 따라가지 않는 포격). 첫 발은 바로, 나머지는 StaggerDelay 간격
		const FVector Target = Candidates[Index].Value;
		const float Delay = StaggerDelay * Index;
		if (Delay <= 0.0f)
		{
			LaunchMissile(Target, Damage);
			continue;
		}

		FTimerHandle Handle;
		World->GetTimerManager().SetTimer(Handle,
			FTimerDelegate::CreateWeakLambda(this, [this, Target, Damage]() { LaunchMissile(Target, Damage); }), Delay, false);
	}
}

void UUSMissileBarrageComponent::LaunchMissile(const FVector& Target, float Damage)
{
	UWorld* World = GetWorld();
	const AActor* Owner = GetOwner();
	if (World == nullptr || Owner == nullptr)
	{
		return;
	}

	if (MissileClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: MissileBarrage has no MissileClass; exploding without a missile"), *GetNameSafe(Owner));
		Explode(Target, Damage);
		return;
	}

	const FVector Start = Owner->GetActorLocation() + FVector(0.0f, 0.0f, LaunchHeight);
	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 6초에 최대 4발이라 풀 없이 스폰·파괴한다 (총알처럼 초당 수십 발이면 풀이 필요하다)
	AUSMissile* Missile = World->SpawnActor<AUSMissile>(MissileClass, Start, FRotator::ZeroRotator, Params);
	if (Missile == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: failed to spawn missile; exploding in place"), *GetNameSafe(Owner));
		Explode(Target, Damage);
		return;
	}

	// 대장이 죽거나 맵이 바뀌어 이 컴포넌트가 사라지면 도착 알림은 무시된다 (WeakLambda)
	Missile->OnArrived.BindWeakLambda(this, [this, Damage](const FVector& Location) { Explode(Location, Damage); });
	Missile->Launch(Start, Target);
}

void UUSMissileBarrageComponent::Explode(const FVector& Location, float Damage)
{
	// 격납고 "폭발 범위" 칸은 연쇄 폭발 서브시스템이 들고 있다 (판정은 감속 때문에 여기서 따로 한다)
	const UUSChainExplosionSubsystem* Chain = GetWorld() ? GetWorld()->GetSubsystem<UUSChainExplosionSubsystem>() : nullptr;
	const float Radius = Chain ? Chain->ScaleRadius(ExplosionRadius) : ExplosionRadius;

	// 격추·연쇄 폭발과 같은 이펙트라 같은 동시 개수 상한을 거친다 (넘치면 가벼운 판)
	if (ExplosionFX)
	{
		if (UUSFXBudgetSubsystem* Budget = GetWorld() ? GetWorld()->GetSubsystem<UUSFXBudgetSubsystem>() : nullptr)
		{
			Budget->SpawnBudgetedEmitter(ExplosionFX, Location, FVector(ExplosionFXScale));
		}
	}
	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, Location);
	}
	// 파티클만으로는 어디까지 느려지는지 안 보인다 — 실제 판정 반경 그대로 고리를 그린다 (기뢰 BlastRing과 같은 방식)
	if (SlowRing)
	{
		SlowRing->PlayPulse(Location, Radius);
	}

	const UWorld* World = GetWorld();
	const UUSTeamSubsystem* Team = World ? World->GetSubsystem<UUSTeamSubsystem>() : nullptr;
	if (Team == nullptr)
	{
		return;
	}

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AController* Instigator = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	const float RadiusSq = FMath::Square(Radius);

	// ApplyDamage가 적을 죽이면 등록부 배열이 바뀌므로 복사본을 돈다
	const TArray<TObjectPtr<AActor>> Enemies = Team->GetEnemies();
	for (AActor* Actor : Enemies)
	{
		AUSEnemyCharacter* Enemy = Cast<AUSEnemyCharacter>(Actor);
		if (Enemy && Enemy->IsAlive() && FVector::DistSquared2D(Location, Enemy->GetActorLocation()) <= RadiusSq)
		{
			// 감속을 먼저 건다: 피해로 죽으면 ApplySlow가 죽은 적은 무시한다
			Enemy->ApplySlow(SlowMultiplier, SlowDuration + SlowDurationBonus);
			if (Damage > 0.0f)
			{
				UGameplayStatics::ApplyDamage(Enemy, Damage, Instigator, GetOwner(), UDamageType::StaticClass());
			}
		}
	}
}
