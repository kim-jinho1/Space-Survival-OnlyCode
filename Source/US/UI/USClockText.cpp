#include "UI/USClockText.h"

namespace
{
	constexpr int32 ClockSecondsPerMinute = 60;
}

void USClockText::Split(float Seconds, FText& OutMinutes, FText& OutSeconds)
{
	const int32 TotalSeconds = FMath::Max(0, FMath::FloorToInt(Seconds));

	FNumberFormattingOptions TwoDigits;
	TwoDigits.MinimumIntegralDigits = 2;
	TwoDigits.UseGrouping = false;

	OutMinutes = FText::AsNumber(TotalSeconds / ClockSecondsPerMinute);
	OutSeconds = FText::AsNumber(TotalSeconds % ClockSecondsPerMinute, &TwoDigits);
}
