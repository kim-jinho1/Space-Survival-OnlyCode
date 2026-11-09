#include "UI/USHealthBar.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UUSHealthBar::SetRatio(const float Value) const
{
	if (HealthBar)
	{
		HealthBar->SetPercent(Value);
	}
}

void UUSHealthBar::SetText(const float Current, const float MaxValue) const
{
	if (HealthText == nullptr)
	{
		return;
	}

	// 소수점 없이 정수로 표시한다.
	FNumberFormattingOptions Options;
	Options.SetMaximumFractionalDigits(0);

	HealthText->SetText(FText::Format(
		HealthTextFormat,
		FText::AsNumber(Current, &Options),
		FText::AsNumber(MaxValue, &Options)));
}
