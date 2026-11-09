#include "Progression/USUpgradeComponent.h"
#include "US.h"
#include "Components/USAttributeComponent.h"
#include "Components/USCurrencyComponent.h"
#include "Components/USMissileBarrageComponent.h"
#include "Components/USSquadComponent.h"
#include "Datas/USAbilityData.h"
#include "Datas/USAllyTypeData.h"
#include "Datas/USBuildData.h"
#include "GameFramework/Pawn.h"

UUSUpgradeComponent::UUSUpgradeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	AbilityTitleFormat = NSLOCTEXT("USUpgradeComponent", "AbilityTitleFormat", "{0} · {1} {2}단계");
	LeaderDisplayName = NSLOCTEXT("USUpgradeComponent", "LeaderDisplayName", "내 전투기");
	SupplyTag = NSLOCTEXT("USUpgradeComponent", "SupplyTag", "보급");
	SupplyGoldName = NSLOCTEXT("USUpgradeComponent", "SupplyGoldName", "골드");
	SupplyRepairName = NSLOCTEXT("USUpgradeComponent", "SupplyRepairName", "수리");
	SupplyGoldTitle = NSLOCTEXT("USUpgradeComponent", "SupplyGoldTitle", "보급 · 골드");
	SupplyGoldDescriptionFormat = NSLOCTEXT("USUpgradeComponent", "SupplyGoldDescriptionFormat", "골드 {0}을 받습니다.");
	SupplyRepairTitle = NSLOCTEXT("USUpgradeComponent", "SupplyRepairTitle", "보급 · 수리");
	SupplyRepairDescriptionFormat = NSLOCTEXT("USUpgradeComponent", "SupplyRepairDescriptionFormat", "내 전투기와 아군 전원의 체력을 {0}% 회복합니다.");
}

int32 UUSUpgradeComponent::GetAbilityLevel(const UUSAbilityData* Ability) const
{
	const int32* Level = AbilityLevels.Find(Ability);
	return Level ? *Level : 0;
}

TArray<FUSLevelUpCard> UUSUpgradeComponent::GenerateChoices(int32 Count, const UUSSquadComponent* Squad) const
{
	if (Abilities.Num() == 0)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: Abilities is empty; only supply cards can appear"), *GetName());
	}

	TArray<const UUSAllyTypeData*> ActiveTypes;
	if (Squad)
	{
		Squad->GetActiveTypes(ActiveTypes);
	}

	// 후보 = 대장 능력 + 분대에 살아 있는 기종의 능력, 그중 최대 단계가 아닌 것
	TArray<FUSLevelUpCard> Pool;
	for (const UUSAbilityData* Ability : Abilities)
	{
		if (Ability == nullptr)
		{
			continue;
		}

		if (!Ability->IsLeaderAbility() && !ActiveTypes.Contains(Ability->GetOwnerType()))
		{
			continue;
		}

		const int32 NextLevel = GetAbilityLevel(Ability) + 1;
		const FUSAbilityLevel* LevelData = Ability->GetLevelData(NextLevel);
		if (LevelData == nullptr)
		{
			continue;
		}

		const FText OwnerName = Ability->IsLeaderAbility() ? LeaderDisplayName : Ability->GetOwnerType()->GetDisplayName();

		FUSLevelUpCard& Card = Pool.AddDefaulted_GetRef();
		Card.Kind = EUSLevelUpCardKind::Ability;
		Card.Ability = Ability;
		Card.NextLevel = NextLevel;
		Card.Title = FText::Format(AbilityTitleFormat, OwnerName, Ability->GetDisplayName(), FText::AsNumber(NextLevel));
		Card.Tag = OwnerName;
		Card.Name = Ability->GetDisplayName();
		Card.MaxLevel = Ability->GetMaxLevel();
		Card.Description = LevelData->Description;
		Card.Detail = LevelData->Detail;
		Card.Icon = Ability->GetIcon();

		if (const UUSBuildData* Build = Ability->GetBuild())
		{
			Card.BandText = Build->GetBandText();
			Card.BandColor = Build->GetBandColor();
			Card.AccentColor = Build->GetColor();
			Card.BandIcon = Build->GetIcon();
		}
		else
		{
			Card.BandText = OwnerName;
			Card.BandColor = NeutralBandColor;
			Card.Tag = FText::GetEmpty();
		}
	}

	// 가중치 = 1 + 그 빌드에서 배운 능력 수 × BuildWeightPerLearned. 빌드가 없는 능력은 1
	TArray<float> Weights;
	Weights.Reserve(Pool.Num());
	for (const FUSLevelUpCard& Card : Pool)
	{
		const UUSBuildData* Build = Card.Ability ? Card.Ability->GetBuild() : nullptr;
		Weights.Add(1.0f + (Build ? CountLearned(Build) * BuildWeightPerLearned : 0.0f));
	}

	TArray<FUSLevelUpCard> Choices;
	Choices.Reserve(Count);

	while (Choices.Num() < Count && Pool.Num() > 0)
	{
		float TotalWeight = 0.0f;
		for (const float Weight : Weights)
		{
			TotalWeight += Weight;
		}

		float Roll = FMath::FRand() * TotalWeight;
		int32 Index = Pool.Num() - 1;
		for (int32 i = 0; i < Pool.Num(); ++i)
		{
			Roll -= Weights[i];
			if (Roll < 0.0f)
			{
				Index = i;
				break;
			}
		}

		Choices.Add(Pool[Index]);
		Pool.RemoveAtSwap(Index);
		Weights.RemoveAtSwap(Index);
	}

	// 무거운 빌드가 먼저 뽑혀 왼쪽 칸에 몰리지 않게 능력 카드 순서를 섞는다 (보급 카드는 아래에서 뒤에 붙는다)
	for (int32 i = Choices.Num() - 1; i > 0; --i)
	{
		Choices.Swap(i, FMath::RandRange(0, i));
	}

	// 능력이 모자라면 보급 카드로 채운다 (골드 → 수리 순, 각 한 장)
	if (Choices.Num() < Count)
	{
		Choices.Add(MakeSupplyCard(EUSLevelUpCardKind::SupplyGold));
	}
	if (Choices.Num() < Count)
	{
		Choices.Add(MakeSupplyCard(EUSLevelUpCardKind::SupplyRepair));
	}

	return Choices;
}

FUSLevelUpCard UUSUpgradeComponent::MakeSupplyCard(EUSLevelUpCardKind Kind) const
{
	FUSLevelUpCard Card;
	Card.Kind = Kind;
	Card.BandText = SupplyTag;
	Card.BandColor = NeutralBandColor;

	if (Kind == EUSLevelUpCardKind::SupplyGold)
	{
		Card.Title = SupplyGoldTitle;
		Card.Name = SupplyGoldName;
		Card.Description = FText::Format(SupplyGoldDescriptionFormat, FText::AsNumber(SupplyGoldAmount));
		Card.Icon = SupplyGoldIcon;
	}
	else
	{
		Card.Title = SupplyRepairTitle;
		Card.Name = SupplyRepairName;
		Card.Description = FText::Format(SupplyRepairDescriptionFormat, FText::AsNumber(FMath::RoundToInt(SupplyRepairFraction * 100.0f)));
		Card.Icon = SupplyRepairIcon;
	}

	return Card;
}

void UUSUpgradeComponent::ApplyCard(const FUSLevelUpCard& Card, APawn* Leader, UUSSquadComponent* Squad)
{
	switch (Card.Kind)
	{
	case EUSLevelUpCardKind::Ability:
	{
		const UUSAbilityData* Ability = Card.Ability;
		if (Ability == nullptr || Card.NextLevel != GetAbilityLevel(Ability) + 1)
		{
			// 후보를 만든 뒤 같은 능력이 먼저 적용된 경우 (카드가 쌓여 연속으로 뜰 때). 단계를 건너뛰지 않게 막는다
			UE_LOG(LogUS, Warning, TEXT("%s: stale ability card %s (level %d, now %d); skipped"),
				*GetName(), *GetNameSafe(Ability), Card.NextLevel, GetAbilityLevel(Ability));
			return;
		}

		AbilityLevels.Add(Ability, Card.NextLevel);

		if (Ability->IsLeaderAbility())
		{
			Ability->ApplyTo(Leader, Card.NextLevel);
		}
		else if (Squad)
		{
			Squad->SetAbilityLevel(Ability, Card.NextLevel);
		}
		else
		{
			UE_LOG(LogUS, Warning, TEXT("%s: no squad for ally ability %s"), *GetName(), *GetNameSafe(Ability));
		}

		// 새로 배운 능력이면 빌드가 다 찼는지 본다 (능력 단계 적용 뒤에 보너스를 얹는다)
		if (Card.NextLevel == 1)
		{
			TryCompleteBuild(Ability->GetBuild(), Leader, Squad);
		}
		break;
	}

	case EUSLevelUpCardKind::SupplyGold:
		if (UUSCurrencyComponent* Currency = Leader ? Leader->FindComponentByClass<UUSCurrencyComponent>() : nullptr)
		{
			Currency->AddGold(SupplyGoldAmount);
		}
		else
		{
			UE_LOG(LogUS, Warning, TEXT("%s: leader has no currency component; supply gold skipped"), *GetName());
		}
		break;

	case EUSLevelUpCardKind::SupplyRepair:
		if (UUSAttributeComponent* Attribute = Leader ? Leader->FindComponentByClass<UUSAttributeComponent>() : nullptr)
		{
			Attribute->Heal(Attribute->GetMaxHealth() * SupplyRepairFraction);
		}
		if (Squad)
		{
			Squad->RepairAll(SupplyRepairFraction);
		}
		break;

	default:
		break;
	}
}

TArray<FUSBuildProgress> UUSUpgradeComponent::GetBuildProgress() const
{
	TArray<FUSBuildProgress> Result;
	for (const UUSBuildData* Build : Builds)
	{
		if (Build == nullptr)
		{
			continue;
		}

		FUSBuildProgress& Progress = Result.AddDefaulted_GetRef();
		Progress.Build = Build;
		Progress.Learned = CountLearned(Build);
		Progress.Total = CountTotal(Build);
		Progress.bComplete = CompletedBuilds.Contains(Build);
	}
	return Result;
}

int32 UUSUpgradeComponent::CountLearned(const UUSBuildData* Build) const
{
	int32 Learned = 0;
	for (const UUSAbilityData* Ability : Abilities)
	{
		if (Ability && Ability->GetBuild() == Build && GetAbilityLevel(Ability) > 0)
		{
			++Learned;
		}
	}
	return Learned;
}

int32 UUSUpgradeComponent::CountTotal(const UUSBuildData* Build) const
{
	int32 Total = 0;
	for (const UUSAbilityData* Ability : Abilities)
	{
		if (Ability && Ability->GetBuild() == Build)
		{
			++Total;
		}
	}
	return Total;
}

void UUSUpgradeComponent::TryCompleteBuild(const UUSBuildData* Build, APawn* Leader, UUSSquadComponent* Squad)
{
	if (Build == nullptr || CompletedBuilds.Contains(Build))
	{
		return;
	}

	const int32 Total = CountTotal(Build);
	if (Total == 0 || CountLearned(Build) < Total)
	{
		return;
	}

	CompletedBuilds.Add(Build);
	const float Value = Build->GetBonusValue();
	UE_LOG(LogUS, Log, TEXT("%s: build %s complete, bonus %d (%.2f)"), *GetName(), *GetNameSafe(Build), (int32)Build->GetBonusType(), Value);

	switch (Build->GetBonusType())
	{
	case EUSBuildBonusType::AttackDamage:
	case EUSBuildBonusType::MaxHealth:
	{
		// 보급선 반복 강화와 같은 길: 대장은 속성에 바로, 부하는 분대가 기록해 이후 스폰·부활에도 다시 적용한다
		const EUSEffectType Effect = Build->GetBonusType() == EUSBuildBonusType::AttackDamage ? EUSEffectType::AttackDamage : EUSEffectType::MaxHealth;
		if (UUSAttributeComponent* Attribute = Leader ? Leader->FindComponentByClass<UUSAttributeComponent>() : nullptr)
		{
			Attribute->ApplyEffect(Effect, Value);
		}
		if (Squad)
		{
			Squad->ApplySquadUpgrade(Effect, Value);
		}
		break;
	}

	case EUSBuildBonusType::SlowDuration:
		if (UUSMissileBarrageComponent* Barrage = Leader ? Leader->FindComponentByClass<UUSMissileBarrageComponent>() : nullptr)
		{
			Barrage->SetSlowDurationBonus(Value);
		}
		if (Squad)
		{
			Squad->AddSlowDurationBonus(Value);
		}
		break;

	default:
		break;
	}
}
