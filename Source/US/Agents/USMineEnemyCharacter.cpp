#include "Agents/USMineEnemyCharacter.h"
#include "Core/USTeamSubsystem.h"
#include "Core/USChainExplosionSubsystem.h"
#include "Components/USAttributeComponent.h"
#include "Components/USRingPulsePoolComponent.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

AUSMineEnemyCharacter::AUSMineEnemyCharacter()
{
	// 기본 1초 간격이면 닿고 나서 최대 1초 뒤에 터진다. 기뢰는 닿자마자 터져야 해서 자주 확인한다
	ContactDamageInterval = 0.2f;

	BlastRing = CreateDefaultSubobject<UUSRingPulsePoolComponent>(TEXT("BlastRing"));
	BlastRing->SetupAttachment(GetRootComponent());
	BlastRing->SetPoolSize(1);
}

void AUSMineEnemyCharacter::ApplyContactDamage()
{
	if (!IsAlive())
	{
		return;
	}

	if (IsTargetInContactRange(GetTargetActor()))
	{
		Detonate();
	}
}

void AUSMineEnemyCharacter::Detonate()
{
	bDetonated = true;

	if (UUSTeamSubsystem* Team = GetWorld() ? GetWorld()->GetSubsystem<UUSTeamSubsystem>() : nullptr)
	{
		// 피해로 아군이 죽으면 등록부에서 빠지며 배열이 바뀌므로 복사본을 돈다 (아군은 최대 9대)
		TArray<AActor*, TInlineAllocator<16>> Allies;
		for (AActor* Ally : Team->GetAllies())
		{
			Allies.Add(Ally);
		}

		const FVector Origin = GetActorLocation();

		for (AActor* Ally : Allies)
		{
			if (IsValid(Ally) && FVector::Dist(Origin, Ally->GetActorLocation()) <= ExplosionRadius)
			{
				UGameplayStatics::ApplyDamage(Ally, ExplosionDamage, GetController(), this, UDamageType::StaticClass());
			}
		}
	}

	// 체력을 깎아 죽이면 데미지 숫자·피격 발광이 뜬다. 사망 처리를 직접 불러 폭발 연출만 남긴다
	HandleDeath();
}

void AUSMineEnemyCharacter::RequestChainExplosion()
{
	UUSChainExplosionSubsystem* Chain = GetWorld() ? GetWorld()->GetSubsystem<UUSChainExplosionSubsystem>() : nullptr;
	if (Chain == nullptr)
	{
		return;
	}

	// 지연 없이 바로 터진다. 일반 연쇄 폭발처럼 이 폭발에 죽은 적도 다시 확률을 굴린다
	const float MaxHealth = AttributeComponent ? AttributeComponent->GetMaxHealth() : 0.0f;
	Chain->QueueExplosion(GetFeedbackLocation(), MaxHealth * BlastDamageRatio, BlastRadius, BlastFXScaleMultiplier, 0.0f);

	// 피해 범위를 눈으로 보이게 한다. 판이 기뢰에 붙어 있지만 PlayPulse가 월드 위치로 옮겨 그리므로 숨겨진 기체와 상관없다
	if (BlastRing)
	{
		BlastRing->PlayPulse(GetFeedbackLocation(), Chain->ScaleRadius(BlastRadius));
	}
}

bool AUSMineEnemyCharacter::ShouldCountAsKill() const
{
	return !bDetonated;
}
