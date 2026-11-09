#include "Components/USRamComponent.h"
#include "Agents/USEnemyCharacter.h"
#include "Components/USAttributeComponent.h"
#include "Core/USTeamSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

UUSRamComponent::UUSRamComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUSRamComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RamTimerHandle, this, &UUSRamComponent::ApplyRamDamage, RamInterval, true);
	}

	OwnerAttribute = GetOwner() ? GetOwner()->FindComponentByClass<UUSAttributeComponent>() : nullptr;
	if (OwnerAttribute)
	{
		OwnerAttribute->OnDeath.AddUObject(this, &UUSRamComponent::HandleOwnerDeath);
	}
}

void UUSRamComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RamTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UUSRamComponent::HandleOwnerDeath()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RamTimerHandle);
	}
}

void UUSRamComponent::ApplyRamDamage()
{
	AActor* Owner = GetOwner();
	const UWorld* World = GetWorld();
	const UUSTeamSubsystem* Team = World ? World->GetSubsystem<UUSTeamSubsystem>() : nullptr;
	if (Owner == nullptr || Team == nullptr)
	{
		return;
	}

	const APawn* OwnerPawn = Cast<APawn>(Owner);
	AController* Instigator = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	const FVector OwnerLocation = Owner->GetActorLocation();
	const float RadiusSq = FMath::Square(RamRadius);
	// 배율은 강화로 바뀔 수 있으므로 칠 때마다 읽는다 (0.5초 간격이라 비용 없음)
	const float Damage = RamDamage * DamageScale * (OwnerAttribute ? OwnerAttribute->GetAttackDamageMultiplier() : 1.0f);

	// ApplyDamage가 적을 죽이면 등록부 배열이 바뀌므로 복사본을 돈다 (적 수십 마리라 복사 비용은 작다)
	const TArray<TObjectPtr<AActor>> Enemies = Team->GetEnemies();
	for (AActor* Actor : Enemies)
	{
		AUSEnemyCharacter* Enemy = Cast<AUSEnemyCharacter>(Actor);
		if (Enemy == nullptr || !Enemy->IsAlive())
		{
			continue;
		}

		if (FVector::DistSquared(OwnerLocation, Enemy->GetActorLocation()) <= RadiusSq)
		{
			UGameplayStatics::ApplyDamage(Enemy, Damage, Instigator, Owner, UDamageType::StaticClass());

			// 진형 앞에서 적을 떼어 내 대장·부하에게 덜 닿게 한다. 피해로 죽은 적은 밀지 않는다
			UCharacterMovementComponent* Movement = Enemy->IsAlive() ? Enemy->GetCharacterMovement() : nullptr;
			if (Movement && KnockbackSpeed > 0.0f)
			{
				Movement->AddImpulse((Enemy->GetActorLocation() - OwnerLocation).GetSafeNormal2D() * KnockbackSpeed, true);
			}
		}
	}
}

void UUSRamComponent::SetDamageScale(float Scale)
{
	DamageScale = FMath::Max(0.0f, Scale);
}
