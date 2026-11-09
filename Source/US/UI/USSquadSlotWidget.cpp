#include "UI/USSquadSlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UUSSquadSlotWidget::SetupSlot(int32 KeyNumber, UTexture2D* Icon)
{
	if (KeyText)
	{
		KeyText->SetText(FText::AsNumber(KeyNumber));
	}

	if (SlotIcon && Icon)
	{
		SlotIcon->SetBrushFromTexture(Icon);
	}
}

void UUSSquadSlotWidget::SetIconColor(const FLinearColor& Color)
{
	if (SlotIcon)
	{
		SlotIcon->SetColorAndOpacity(Color);
	}
}

void UUSSquadSlotWidget::SetCost(const FText& Cost, const FLinearColor& Color)
{
	if (CostText == nullptr)
	{
		return;
	}

	CostText->SetText(Cost);
	CostText->SetColorAndOpacity(Color);
	CostText->SetVisibility(Cost.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
}
