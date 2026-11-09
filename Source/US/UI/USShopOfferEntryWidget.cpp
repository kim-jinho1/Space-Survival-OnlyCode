#include "UI/USShopOfferEntryWidget.h"
#include "Shop/USShopComponent.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Components/TextBlock.h"

UUSShopOfferEntryWidget::UUSShopOfferEntryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CostFormat = NSLOCTEXT("USShopOfferEntryWidget", "CostFormat", "{0} 골드");
}

void UUSShopOfferEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (OfferButton)
	{
		OfferButton->OnClicked.AddUniqueDynamic(this, &UUSShopOfferEntryWidget::HandleButtonClicked);
	}
}

void UUSShopOfferEntryWidget::SetupOffer(int32 InIndex, const FUSShopOffer& Offer, bool bAffordable)
{
	OfferIndex = InIndex;

	if (OfferTitle)
	{
		OfferTitle->SetText(Offer.Title);
	}

	if (OfferEffect)
	{
		OfferEffect->SetText(Offer.Effect);
		OfferEffect->SetVisibility(Offer.Effect.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (OfferDetail)
	{
		OfferDetail->SetText(Offer.Detail);
		OfferDetail->SetVisibility(Offer.Detail.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (OfferIcon)
	{
		if (Offer.Icon)
		{
			OfferIcon->SetBrushFromTexture(Offer.Icon);
			OfferIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			OfferIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (OfferCost)
	{
		OfferCost->SetText(FText::Format(CostFormat, FText::AsNumber(Offer.Cost)));
		OfferCost->SetColorAndOpacity(bAffordable ? AffordableCostColor : UnaffordableCostColor);
	}

	// 버튼을 꺼도 칩 바탕색은 그대로라, 살 수 있는지는 칩 색으로 직접 보여 준다
	if (CostChip)
	{
		CostChip->SetBrushColor(bAffordable ? AffordableChipColor : UnaffordableChipColor);
	}

	if (OfferButton)
	{
		OfferButton->SetIsEnabled(bAffordable);
	}
}

void UUSShopOfferEntryWidget::HandleButtonClicked()
{
	OnOfferClicked.ExecuteIfBound(OfferIndex);
}
