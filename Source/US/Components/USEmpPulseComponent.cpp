#include "Components/USEmpPulseComponent.h"
#include "US.h"
#include "Agents/USEnemyCharacter.h"
#include "Components/USAttributeComponent.h"
#include "Core/USTeamSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	/** 엔진 기본 Plane 메시의 한 변 길이 (uu) */
	constexpr float PlaneMeshSize = 100.0f;
}

UUSEmpPulseComponent::UUSEmpPulseComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 표시 전용: 충돌·그림자·내비 영향 없음
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCanEverAffectNavigation(false);
	CastShadow = false;
	SetGenerateOverlapEvents(false);

	// 기체가 돌아도 고리는 바닥에 평평하게 둔다
	SetUsingAbsoluteRotation(true);
}

void UUSEmpPulseComponent::BeginPlay()
{
	Super::BeginPlay();

	ApplyRadiusScale();

	if (GetMaterial(0) == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: EMP pulse has no ring material; slow works but nothing is drawn"), *GetNameSafe(GetOwner()));
	}

	OwnerAttribute = GetOwner() ? GetOwner()->FindComponentByClass<UUSAttributeComponent>() : nullptr;
	if (OwnerAttribute)
	{
		OwnerAttribute->OnDeath.AddUObject(this, &UUSEmpPulseComponent::HandleOwnerDeath);
	}

	ScheduleNextPulse();
}

void UUSEmpPulseComponent::ApplyRadiusScale()
{
	// 판 지름 = 감속 지름이 되게 키운다. 부모(캡슐) 스케일은 1이라 상대 스케일로 충분하다.
	const float Scale = PulseRadius * 2.0f / PlaneMeshSize;
	SetRelativeScale3D(FVector(Scale, Scale, 1.0f));
}

void UUSEmpPulseComponent::SetPulseDamage(float Damage)
{
	PulseDamage = FMath::Max(0.0f, Damage);
}

void UUSEmpPulseComponent::SetPulseArea(float Radius, float Duration)
{
	PulseRadius = FMath::Max(10.0f, Radius);
	SlowDuration = FMath::Max(0.0f, Duration);
	ApplyRadiusScale();
}

void UUSEmpPulseComponent::SetSlowDurationBonus(float Bonus)
{
	SlowDurationBonus = FMath::Max(0.0f, Bonus);
}

void UUSEmpPulseComponent::ScheduleNextPulse()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// 반복 타이머는 간격이 고정이라, 한 번짜리 타이머를 펄스마다 다시 걸어 강화된 배율을 다음 간격부터 반영한다
	const float Multiplier = OwnerAttribute ? OwnerAttribute->GetAttackSpeedMultiplier() : 1.0f;
	const float Interval = Multiplier > KINDA_SMALL_NUMBER ? PulseInterval / Multiplier : PulseInterval;
	World->GetTimerManager().SetTimer(PulseTimerHandle, this, &UUSEmpPulseComponent::Pulse, Interval, false);
}

void UUSEmpPulseComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PulseTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UUSEmpPulseComponent::HandleOwnerDeath()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PulseTimerHandle);
	}

	SetVisibility(false);
}

void UUSEmpPulseComponent::Pulse()
{
	// 먼저 다음 펄스를 걸어 둔다 (아래에서 일찍 끝나도 주기가 끊기지 않게)
	ScheduleNextPulse();

	const UWorld* World = GetWorld();
	const UUSTeamSubsystem* Team = World ? World->GetSubsystem<UUSTeamSubsystem>() : nullptr;
	if (Team == nullptr)
	{
		return;
	}

	const FVector Center = GetComponentLocation();
	const float RadiusSq = FMath::Square(PulseRadius);

	const float Damage = PulseDamage * (OwnerAttribute ? OwnerAttribute->GetAttackDamageMultiplier() : 1.0f);
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	AController* Instigator = OwnerPawn ? OwnerPawn->GetController() : nullptr;

	// 피해로 적이 죽으면 등록부 배열이 바뀌므로 복사본을 돈다 (충각과 같은 이유)
	const TArray<TObjectPtr<AActor>> Enemies = Team->GetEnemies();
	for (AActor* Actor : Enemies)
	{
		AUSEnemyCharacter* Enemy = Cast<AUSEnemyCharacter>(Actor);
		if (Enemy == nullptr || !Enemy->IsAlive())
		{
			continue;
		}

		// 바닥 높이 차이는 무시하고 평면 거리로 판정한다
		if (FVector::DistSquared2D(Center, Enemy->GetActorLocation()) <= RadiusSq)
		{
			Enemy->ApplySlow(SlowMultiplier, SlowDuration + SlowDurationBonus);
			if (Damage > 0.0f)
			{
				UGameplayStatics::ApplyDamage(Enemy, Damage, Instigator, GetOwner(), UDamageType::StaticClass());
			}
		}
	}

	// 머티리얼 Time 노드와 같은 기준인 월드 시간을 적으면 머티리얼이 고리를 퍼뜨린다
	SetCustomPrimitiveDataFloat(PulseTimeDataIndex, World->GetTimeSeconds());
}
