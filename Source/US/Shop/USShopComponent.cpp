#include "Shop/USShopComponent.h"
#include "US.h"
#include "Agents/USPlayerCharacter.h"
#include "Components/USAttributeComponent.h"
#include "Components/USCurrencyComponent.h"
#include "Components/USSquadComponent.h"
#include "Datas/USAllyTypeData.h"
#include "Datas/USShopUpgradeData.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UUSShopComponent::UUSShopComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 카드는 이름(무엇을) · 효과(얼마나) · 설명 세 줄이다. 이름과 효과를 한 줄에 붙이면 좁은 카드에서 "+15 / 초"처럼 끊긴다
	RepairTitleFormat = NSLOCTEXT("USShopComponent", "RepairTitleFormat", "아군 수리");
	ReviveTitleFormat = NSLOCTEXT("USShopComponent", "ReviveTitleFormat", "{0}번 {1}");
	RecruitTitleFormat = NSLOCTEXT("USShopComponent", "RecruitTitleFormat", "{0}");
	RepairEffectFormat = NSLOCTEXT("USShopComponent", "RepairEffectFormat", "체력 {0}%");
	ReviveEffect = NSLOCTEXT("USShopComponent", "ReviveEffect", "부활");
	RecruitEffect = NSLOCTEXT("USShopComponent", "RecruitEffect", "합류");
	RepairDetail = NSLOCTEXT("USShopComponent", "RepairDetail", "다친 내 전투기·아군 회복");
	ReviveDetail = NSLOCTEXT("USShopComponent", "ReviveDetail", "격추된 전투기");
	RecruitDetail = NSLOCTEXT("USShopComponent", "RecruitDetail", "새 전투기");
	UpgradeDetailFormat = NSLOCTEXT("USShopComponent", "UpgradeDetailFormat", "구매 {0}회");
}

void UUSShopComponent::RefreshOffers(const UUSSquadComponent* Squad, bool bCanExtendRunTime)
{
	Offers.Reset();

	if (Squad == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: no squad; shop has nothing to sell"), *GetNameSafe(GetOwner()));
		return;
	}

	const TArray<FUSSquadMember>& Members = Squad->GetMembers();
	// 수리 1줄 + 격추된 슬롯마다 1줄 + 반복 강화 + 합류 종류마다 1줄이 최대치다
	Offers.Reserve(1 + Members.Num() + Upgrades.Num() + Squad->GetHireableTypes().Num());

	// 다친 부하가 없으면 수리는 효과가 없으니 목록에 넣지 않는다
	if (Squad->HasDamagedMember())
	{
		FUSShopOffer Repair;
		Repair.Kind = EUSShopOfferKind::Repair;
		Repair.Cost = GetRepairCost();
		const FText RepairPercent = FText::AsNumber(FMath::RoundToInt(RepairFraction * 100.0f));
		Repair.Title = FText::Format(RepairTitleFormat, RepairPercent);
		Repair.Effect = FText::Format(RepairEffectFormat, RepairPercent);
		Repair.Detail = RepairDetail;
		Repair.Icon = RepairIcon;
		Offers.Add(Repair);
	}

	const int32 ReviveCost = Squad->GetReviveCost();
	for (int32 Index = 0; Index < Members.Num(); ++Index)
	{
		const FUSSquadMember& Member = Members[Index];
		if (!Member.bDown || Member.Type == nullptr)
		{
			continue;
		}

		FUSShopOffer Revive;
		Revive.Kind = EUSShopOfferKind::Revive;
		Revive.SlotIndex = Index;
		Revive.Cost = ReviveCost;
		Revive.Title = FText::Format(ReviveTitleFormat, FText::AsNumber(Index + 1), Member.Type->GetDisplayName());
		Revive.Effect = ReviveEffect;
		Revive.Detail = ReviveDetail;
		Revive.Icon = Member.Type->GetIcon();
		Offers.Add(Revive);
	}

	// 반복 강화는 분대가 가득 차도 항상 판다. 구매 횟수 배열은 에디터에서 목록 길이가 바뀌어도 맞도록 여기서 맞춘다
	if (Upgrades.IsEmpty())
	{
		UE_LOG(LogUS, Warning, TEXT("%s: shop has no repeatable upgrades assigned"), *GetNameSafe(GetOwner()));
	}
	UpgradeCounts.SetNumZeroed(Upgrades.Num());

	for (int32 Index = 0; Index < Upgrades.Num(); ++Index)
	{
		const UUSShopUpgradeData* Upgrade = Upgrades[Index];
		if (Upgrade == nullptr)
		{
			UE_LOG(LogUS, Warning, TEXT("%s: shop upgrade slot %d is empty"), *GetNameSafe(GetOwner()), Index);
			continue;
		}

		// 시간을 더 사도 쓸 데가 없을 때는 줄을 빼서, 골드만 나가는 상품을 보이지 않는다
		if (Upgrade->GetTarget() == EUSShopUpgradeTarget::RunTime && !bCanExtendRunTime)
		{
			continue;
		}

		FUSShopOffer Offer;
		Offer.Kind = EUSShopOfferKind::Upgrade;
		Offer.UpgradeIndex = Index;
		Offer.Cost = GetUpgradeCost(Index);
		Offer.Title = FText::Format(Upgrade->GetTitleFormat(), FText::AsNumber(UpgradeCounts[Index]));
		Offer.Effect = Upgrade->GetEffectText();
		Offer.Detail = FText::Format(UpgradeDetailFormat, FText::AsNumber(UpgradeCounts[Index]));
		Offer.Icon = Upgrade->GetIcon();
		Offers.Add(Offer);
	}

	// 자리가 없으면 합류 줄은 빼서, 사 봤자 골드만 나가는 상품을 보이지 않는다
	if (bOfferRecruits && Squad->CanAddAlly())
	{
		const int32 RecruitCost = GetRecruitCost();
		for (const TObjectPtr<UUSAllyTypeData>& AllyType : Squad->GetHireableTypes())
		{
			if (AllyType == nullptr)
			{
				continue;
			}

			FUSShopOffer Offer;
			Offer.Kind = EUSShopOfferKind::Recruit;
			Offer.AllyType = AllyType;
			Offer.Cost = RecruitCost;
			Offer.Title = FText::Format(RecruitTitleFormat, AllyType->GetDisplayName());
			Offer.Effect = RecruitEffect;
			Offer.Detail = RecruitDetail;
			Offer.Icon = AllyType->GetIcon();
			Offers.Add(Offer);
		}
	}
}

bool UUSShopComponent::TryPurchase(int32 Index, UUSSquadComponent* Squad, UUSCurrencyComponent* Currency)
{
	if (!Offers.IsValidIndex(Index) || Squad == nullptr || Currency == nullptr)
	{
		return false;
	}

	const FUSShopOffer& Offer = Offers[Index];

	switch (Offer.Kind)
	{
	case EUSShopOfferKind::Repair:
		// 골드부터 내면 수리할 부하가 없을 때 골드만 사라지므로, 다친 부하가 있는지 먼저 본다
		if (!Squad->HasDamagedMember() || !Currency->TrySpend(Offer.Cost))
		{
			return false;
		}

		Squad->RepairAll(RepairFraction);
		++RepairCount;

		if (RepairSound)
		{
			UGameplayStatics::PlaySound2D(this, RepairSound, 1.0f, 1.0f, 0.0f, nullptr, nullptr, true);
		}
		return true;

	case EUSShopOfferKind::Revive:
		// 가격 계산·골드 차감·재스폰은 분대가 한 곳에서 처리한다 (표시 가격과 같은 공식)
		return Squad->TryRevive(Offer.SlotIndex);

	case EUSShopOfferKind::Upgrade:
		return TryPurchaseUpgrade(Offer, Squad, Currency);

	case EUSShopOfferKind::Recruit:
		return TryPurchaseRecruit(Offer, Squad, Currency);

	default:
		return false;
	}
}

int32 UUSShopComponent::GetRepairCost() const
{
	return FMath::RoundToInt(RepairBaseCost * FMath::Pow(RepairCostGrowth, static_cast<float>(RepairCount)));
}

int32 UUSShopComponent::GetUpgradeCost(int32 Index) const
{
	const UUSShopUpgradeData* Upgrade = Upgrades.IsValidIndex(Index) ? Upgrades[Index].Get() : nullptr;
	if (Upgrade == nullptr)
	{
		return 0;
	}

	const int32 Count = UpgradeCounts.IsValidIndex(Index) ? UpgradeCounts[Index] : 0;
	return FMath::RoundToInt(Upgrade->GetBaseCost() * FMath::Pow(Upgrade->GetCostGrowth(), static_cast<float>(Count)));
}

int32 UUSShopComponent::GetRecruitCost() const
{
	return FMath::RoundToInt(RecruitBaseCost * FMath::Pow(RecruitCostGrowth, static_cast<float>(RecruitCount)));
}

bool UUSShopComponent::TryPurchaseUpgrade(const FUSShopOffer& Offer, UUSSquadComponent* Squad, UUSCurrencyComponent* Currency)
{
	const UUSShopUpgradeData* Upgrade = Upgrades.IsValidIndex(Offer.UpgradeIndex) ? Upgrades[Offer.UpgradeIndex].Get() : nullptr;
	if (Upgrade == nullptr || !UpgradeCounts.IsValidIndex(Offer.UpgradeIndex))
	{
		return false;
	}

	// 골드부터 내면 대장을 못 찾을 때 골드만 사라지므로, 대장 강화는 대상을 먼저 찾는다 (구매 때 한 번이라 매번 찾아도 된다)
	AUSPlayerCharacter* Leader = nullptr;
	if (Upgrade->GetTarget() == EUSShopUpgradeTarget::MagnetRadius)
	{
		Leader = Cast<AUSPlayerCharacter>(Squad->GetOwner());
		if (Leader == nullptr)
		{
			UE_LOG(LogUS, Warning, TEXT("%s: leader not found; %s not applied"), *GetNameSafe(GetOwner()), *GetNameSafe(Upgrade));
			return false;
		}
	}

	UUSAttributeComponent* LeaderAttribute = nullptr;
	if (Upgrade->GetTarget() == EUSShopUpgradeTarget::Leader)
	{
		LeaderAttribute = Squad->GetOwner() ? Squad->GetOwner()->FindComponentByClass<UUSAttributeComponent>() : nullptr;
		if (LeaderAttribute == nullptr)
		{
			UE_LOG(LogUS, Warning, TEXT("%s: leader attribute not found; %s not applied"), *GetNameSafe(GetOwner()), *GetNameSafe(Upgrade));
			return false;
		}
	}

	if (!Currency->TrySpend(Offer.Cost))
	{
		return false;
	}

	switch (Upgrade->GetTarget())
	{
	case EUSShopUpgradeTarget::Squad:
		Squad->ApplySquadUpgrade(Upgrade->GetEffect(), Upgrade->GetValue());
		break;

	case EUSShopUpgradeTarget::GoldGain:
		Currency->ScaleGoldGain(1.0f + Upgrade->GetValue());
		break;

	case EUSShopUpgradeTarget::Leader:
		LeaderAttribute->ApplyEffect(Upgrade->GetEffect(), Upgrade->GetValue());
		break;

	case EUSShopUpgradeTarget::RunTime:
		OnRunTimeExtended.Broadcast(Upgrade->GetValue());
		break;

	case EUSShopUpgradeTarget::MagnetRadius:
		Leader->ScaleMagnetRadius(1.0f + Upgrade->GetValue());
		break;
	}

	++UpgradeCounts[Offer.UpgradeIndex];

	if (UpgradeSound)
	{
		UGameplayStatics::PlaySound2D(this, UpgradeSound, 1.0f, 1.0f, 0.0f, nullptr, nullptr, true);
	}
	return true;
}

bool UUSShopComponent::TryPurchaseRecruit(const FUSShopOffer& Offer, UUSSquadComponent* Squad, UUSCurrencyComponent* Currency)
{
	// 골드부터 내면 자리가 없을 때 골드만 사라지므로, 자리가 있는지 먼저 본다
	if (Offer.AllyType == nullptr || !Squad->CanAddAlly() || !Currency->TrySpend(Offer.Cost))
	{
		return false;
	}

	// 합류음·분대 갱신 알림은 분대가 낸다
	if (!Squad->TryAddFreeAlly(Offer.AllyType))
	{
		// 자리를 확인했는데도 실패했다면 스폰 실패(DroneClass 누락 등)다. 분대가 원인을 로그로 남기므로 여기서는 환불만 한다
		UE_LOG(LogUS, Warning, TEXT("%s: recruit %s failed; refunding %d gold"), *GetNameSafe(GetOwner()), *GetNameSafe(Offer.AllyType), Offer.Cost);
		Currency->AddGold(Offer.Cost);
		return false;
	}

	++RecruitCount;
	return true;
}
