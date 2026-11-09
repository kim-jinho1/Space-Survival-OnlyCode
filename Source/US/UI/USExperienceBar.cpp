#include "UI/USExperienceBar.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UUSExperienceBar::SetRatio(const float Value) const
{
	if (ExperienceBar)
	{
		ExperienceBar->SetPercent(Value);
	}
}

void UUSExperienceBar::SetExperience(const float CurrentExperience, const float TargetExperience) const
{
	if (ExperienceText == nullptr)
	{
		return;
	}

	// 소수점 없이 정수로 표시한다.
	FNumberFormattingOptions Options;
	Options.SetMaximumFractionalDigits(0);

	ExperienceText->SetText(FText::Format(
		ExperienceTextFormat,
		FText::AsNumber(CurrentExperience, &Options),
		FText::AsNumber(TargetExperience, &Options)));
}

void UUSExperienceBar::SetLevel(const int Level) const
{
	if (LevelText)
	{
		LevelText->SetText(FText::Format(LevelTextFormat, Level));
	}
}
