#include "UI/USQuickInfoUI.h"
#include "Components/TextBlock.h"

void UUSQuickInfoUI::SetGoldAmount(const int32 Value) const
{
	if (GoldAmount)
	{
		GoldAmount->SetText(FText::Format(GoldAmountTextFormat, Value));
	}
}

void UUSQuickInfoUI::SetKillCount(int32 Value) const
{
	if (KillCountText)
	{
		// AsNumber는 천 단위 쉼표를 넣는다 (1,240)
		KillCountText->SetText(FText::Format(KillCountTextFormat, FText::AsNumber(Value)));
	}
}

void UUSQuickInfoUI::SetSquadDps(float Dps, float AllyShare) const
{
	if (SquadDpsText)
	{
		SquadDpsText->SetText(FText::Format(SquadDpsTextFormat,
			FText::AsNumber(FMath::RoundToInt(Dps)),
			FText::AsNumber(FMath::RoundToInt(AllyShare * 100.0f))));
	}
}
