#include "Components/USDamageReflectComponent.h"
#include "Agents/USEnemyCharacter.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UUSDamageReflectComponent::UUSDamageReflectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUSDamageReflectComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddUniqueDynamic(this, &UUSDamageReflectComponent::HandleTakeAnyDamage);
	}
}

void UUSDamageReflectComponent::SetReflectFraction(float Fraction)
{
	ReflectFraction = FMath::Max(0.0f, Fraction);
}

void UUSDamageReflectComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	if (ReflectFraction <= 0.0f || Damage <= 0.0f)
	{
		return;
	}

	// 적의 접촉·자폭 피해만 되돌린다. 가해자가 적이 아니면(다른 반사·환경 피해) 무시해 반사끼리 주고받는 고리를 막는다
	AUSEnemyCharacter* Enemy = Cast<AUSEnemyCharacter>(DamageCauser);
	if (Enemy == nullptr || !Enemy->IsAlive())
	{
		return;
	}

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	UGameplayStatics::ApplyDamage(Enemy, Damage * ReflectFraction, OwnerPawn ? OwnerPawn->GetController() : nullptr,
		GetOwner(), UDamageType::StaticClass());
}
