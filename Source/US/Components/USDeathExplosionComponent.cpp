#include "Components/USDeathExplosionComponent.h"
#include "US.h"
#include "Core/USFXBudgetSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"

UUSDeathExplosionComponent::UUSDeathExplosionComponent()
{
	// 사망 순간 한 번 부르는 게 전부라 Tick이 필요 없다
	PrimaryComponentTick.bCanEverTick = false;
}

void UUSDeathExplosionComponent::PlayExplosion(const FVector& Location) const
{
	if (DeathSound)
	{
		// 무리가 한꺼번에 터지거나 보스가 연쇄로 터져도 같은 소리 반복으로 들리지 않게 음 높이를 흔든다
		const float Pitch = FMath::FRandRange(1.0f - DeathSoundPitchVariation, 1.0f + DeathSoundPitchVariation);
		UGameplayStatics::PlaySoundAtLocation(this, DeathSound, Location, 1.0f, Pitch);
	}

	if (DeathFX == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: DeathFX is not set; no death explosion"), *GetNameSafe(GetOwner()));
		return;
	}

	// 여러 기체가 연달아 터질 수 있으므로 동시 개수 상한을 거친다 (풀링·LOD 고정도 서브시스템이 한다).
	// 상한이면 가벼운 판으로 대신 터지고, 소유자의 섬광·고리(DeathBurstPool)는 그대로 나온다
	if (UWorld* World = GetWorld())
	{
		if (UUSFXBudgetSubsystem* Budget = World->GetSubsystem<UUSFXBudgetSubsystem>())
		{
			Budget->SpawnBudgetedEmitter(DeathFX, Location, FVector(DeathFXScale));
		}
	}
}
