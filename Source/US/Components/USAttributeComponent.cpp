#include "Components/USAttributeComponent.h"
#include "Progression/USProgressionTypes.h"

UUSAttributeComponent::UUSAttributeComponent()
{
	// 체력 처리는 이벤트 기반이므로 매 프레임 Tick은 필요하지 않다.
	PrimaryComponentTick.bCanEverTick = false;
}

void UUSAttributeComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;

	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UUSAttributeComponent::HandleTakeAnyDamage);
	}

	// 플레이어 로그인·빙의는 월드 BeginPlay보다 먼저 일어나므로 HUD 같은 구독자가
	// 이 시점 전에 이미 붙어 있을 수 있다. 초기값을 알려야 그들이 0이 아닌 값을 본다.
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UUSAttributeComponent::ApplyEffect(const EUSEffectType Type, float Value)
{
	switch (Type)
	{
	case EUSEffectType::HealHealth:
		Heal(Value);
		break;

	case EUSEffectType::MoveSpeed:
		AddMoveSpeedMultiplier(Value);
		break;

	// 공격력·공격 속도·최대 체력은 곱셈으로 쌓는다(Value 0.15 → ×1.15). 더하기면 강화할수록 한 장의 체감이 줄어든다.
	// 이동 속도는 지수로 커지면 조작이 망가지므로 더하기로 둔다.
	case EUSEffectType::AttackDamage:
		ScaleAttackDamageMultiplier(1.0f + Value);
		break;

	case EUSEffectType::AttackSpeed:
		ScaleAttackSpeedMultiplier(1.0f + Value);
		break;

	case EUSEffectType::MaxHealth:
		ScaleMaxHealth(1.0f + Value);
		break;

	default:
		break;
	}
}

bool UUSAttributeComponent::IsAlive() const
{
	return !bIsDead && CurrentHealth > 0.0f;
}

void UUSAttributeComponent::Heal(float Amount)
{
	if (!IsAlive() || Amount <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UUSAttributeComponent::ScaleMaxHealth(float Factor)
{
	if (Factor <= 0.0f || FMath::IsNearlyEqual(Factor, 1.0f))
	{
		return;
	}

	const float PreviousMaxHealth = MaxHealth;
	MaxHealth = FMath::Max(1.0f, MaxHealth * Factor);

	// 최대 체력 증가분만큼 현재 체력도 회복(감소 시에는 상한으로 클램프)
	CurrentHealth = FMath::Clamp(CurrentHealth + FMath::Max(0.0f, MaxHealth - PreviousMaxHealth), 0.0f, MaxHealth);

	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
	OnStatsChanged.Broadcast();
}

void UUSAttributeComponent::SetMaxHealth(float NewMaxHealth)
{
	MaxHealth = FMath::Max(1.0f, NewMaxHealth);
	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UUSAttributeComponent::AddMoveSpeedMultiplier(float Amount)
{
	MoveSpeedMultiplier = FMath::Max(0.0f, MoveSpeedMultiplier + Amount);
	OnStatsChanged.Broadcast();
}

void UUSAttributeComponent::ScaleAttackDamageMultiplier(float Factor)
{
	AttackDamageMultiplier = FMath::Max(0.0f, AttackDamageMultiplier * Factor);
	OnStatsChanged.Broadcast();
}

void UUSAttributeComponent::InitAttackDamageMultiplier(float StartMultiplier)
{
	AttackDamageMultiplier = FMath::Max(0.0f, StartMultiplier);
	OnStatsChanged.Broadcast();
}

void UUSAttributeComponent::ScaleAttackSpeedMultiplier(float Factor)
{
	AttackSpeedMultiplier = FMath::Max(0.0f, AttackSpeedMultiplier * Factor);
	OnStatsChanged.Broadcast();
}

void UUSAttributeComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (!IsAlive() || Damage <= 0.0f)
	{
		return;
	}

	const float PreviousHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);

	// 표시·연출용으로는 요청 데미지가 아니라 실제로 깎인 양을 알린다 (막타의 초과분 제외)
	OnDamaged.Broadcast(PreviousHealth - CurrentHealth, InstigatedBy);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		bIsDead = true;
		OnDeath.Broadcast();
	}
}
