#include "Components/USTargetMarkComponent.h"
#include "US.h"
#include "Agents/USEnemyCharacter.h"
#include "Components/USCombatComponent.h"
#include "Equipments/USWeapon.h"
#include "GameFramework/Actor.h"

UUSTargetMarkComponent::UUSTargetMarkComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUSTargetMarkComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindWeapon();

	Super::EndPlay(EndPlayReason);
}

void UUSTargetMarkComponent::SetMark(float InDamageBonus, float InDuration)
{
	DamageBonus = FMath::Max(0.0f, InDamageBonus);
	Duration = FMath::Max(0.0f, InDuration);

	// 카드는 BeginPlay 한참 뒤에 고르므로 이때는 주무기가 이미 있다. 무기가 바뀌었으면(지금은 없는 일) 새 무기로 옮겨 붙는다
	const UUSCombatComponent* Combat = GetOwner() ? GetOwner()->FindComponentByClass<UUSCombatComponent>() : nullptr;
	AUSWeapon* Weapon = Combat ? Combat->GetMainWeapon() : nullptr;
	if (Weapon == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: TargetMark found no main weapon; marks will not be applied"), *GetNameSafe(GetOwner()));
		return;
	}

	if (BoundWeapon.Get() != Weapon)
	{
		UnbindWeapon();
		TargetHitHandle = Weapon->OnTargetHit.AddUObject(this, &UUSTargetMarkComponent::HandleTargetHit);
		BoundWeapon = Weapon;
	}
}

void UUSTargetMarkComponent::HandleTargetHit(AActor* HitActor)
{
	// 적이 아닌 것(벽 등)은 무시. 이 발에 죽은 적은 ApplyMark가 거른다
	if (AUSEnemyCharacter* Enemy = Cast<AUSEnemyCharacter>(HitActor))
	{
		Enemy->ApplyMark(DamageBonus, Duration);
	}
}

void UUSTargetMarkComponent::UnbindWeapon()
{
	if (AUSWeapon* Weapon = BoundWeapon.Get())
	{
		Weapon->OnTargetHit.Remove(TargetHitHandle);
	}
	BoundWeapon = nullptr;
	TargetHitHandle.Reset();
}
