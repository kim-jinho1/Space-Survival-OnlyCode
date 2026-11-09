#include "UI/USShopWidget.h"
#include "US.h"
#include "Shop/USShopComponent.h"
#include "UI/USShopOfferEntryWidget.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Sound/SoundBase.h"

UUSShopWidget::UUSShopWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GoldFormat = NSLOCTEXT("USShopWidget", "GoldFormat", "보유 {0} 골드");
}

void UUSShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UUSShopWidget::HandleCloseClicked);
	}

	// 게임이 멈춘 상태에서 뜨므로 UI 사운드(일시정지 무시)로 재생한다
	if (OpenSound)
	{
		PlaySound(OpenSound);
	}
}

void UUSShopWidget::SetupOffers(const TArray<FUSShopOffer>& Offers, int32 CurrentGold)
{
	if (GoldText)
	{
		GoldText->SetText(FText::Format(GoldFormat, FText::AsNumber(CurrentGold)));
	}

	if (EmptyText)
	{
		EmptyText->SetVisibility(Offers.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (OfferList == nullptr)
	{
		return;
	}

	// 구매할 때마다 목록(부활 줄 수·가격·살 수 있는지)이 바뀐다. 카드가 많아야 9장이라 매번 새로 만든다
	OfferList->ClearChildren();
	if (SquadOfferList)
	{
		SquadOfferList->ClearChildren();
	}

	if (OfferEntryClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: OfferEntryClass is not set; shop offers are not shown"), *GetName());
		return;
	}

	// 카드 순번은 줄과 상관없이 Offers 안의 순번 그대로 넘긴다: 구매 처리(UUSShopComponent::TryPurchase)가 이 순번을 쓴다
	bool bHasSquadOffer = false;
	for (int32 Index = 0; Index < Offers.Num(); ++Index)
	{
		UUSShopOfferEntryWidget* Entry = CreateWidget<UUSShopOfferEntryWidget>(this, OfferEntryClass);
		if (Entry == nullptr)
		{
			continue;
		}

		Entry->SetupOffer(Index, Offers[Index], Offers[Index].Cost <= CurrentGold);
		Entry->OnOfferClicked.BindUObject(this, &UUSShopWidget::HandleOfferClicked);

		UPanelWidget* List = GetListForOffer(Offers[Index]);
		List->AddChild(Entry);
		bHasSquadOffer |= List == SquadOfferList;
	}

	// 다친 아군·빈 자리가 없으면 아군 줄은 비어 있다. 빈 머리글만 남지 않게 통째로 숨긴다
	if (SquadGroup)
	{
		SquadGroup->SetVisibility(bHasSquadOffer ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

UPanelWidget* UUSShopWidget::GetListForOffer(const FUSShopOffer& Offer) const
{
	if (SquadOfferList && Offer.Kind != EUSShopOfferKind::Upgrade)
	{
		return SquadOfferList;
	}
	return OfferList;
}

void UUSShopWidget::HandleOfferClicked(int32 Index)
{
	OnOfferSelected.ExecuteIfBound(Index);
}

void UUSShopWidget::HandleCloseClicked()
{
	OnShopClosed.ExecuteIfBound();
}
