#include "Datas/USMetaUpgradeData.h"
#include "Datas/USAllyTypeData.h"

int32 UUSMetaUpgradeData::GetCostAtLevel(int32 CurrentLevel) const
{
	return FMath::RoundToInt(BaseCost * FMath::Pow(CostGrowth, static_cast<float>(FMath::Max(0, CurrentLevel))));
}

FText UUSMetaUpgradeData::GetEffectText(int32 Level) const
{
	const float Steps = static_cast<float>(FMath::Max(0, Level));
	FNumberFormattingOptions Format;
	Format.MaximumFractionalDigits = 0;

	switch (Effect)
	{
	// 곱해서 쌓이는 칸: 단계마다 ×(1 + Value) → 합계 증가율 (UUSMetaProgressionSubsystem::GetMultiplier와 같은 식)
	case EUSMetaEffect::LeaderDamage:
	case EUSMetaEffect::LeaderHealth:
	case EUSMetaEffect::SquadDamage:
	case EUSMetaEffect::SquadHealth:
	case EUSMetaEffect::GoldGain:
	case EUSMetaEffect::ExpGain:
	case EUSMetaEffect::ScrapGain:
		return FText::Format(NSLOCTEXT("USMetaUpgradeData", "PercentEffect", "+{0}%"),
			FText::AsNumber((FMath::Pow(1.0f + ValuePerLevel, Steps) - 1.0f) * 100.0f, &Format));

	// 더해서 쌓이는 칸
	case EUSMetaEffect::RunDuration:
		return FText::Format(NSLOCTEXT("USMetaUpgradeData", "SecondsEffect", "+{0}초"), FText::AsNumber(ValuePerLevel * Steps, &Format));

	case EUSMetaEffect::LeaderMoveSpeed:
		return FText::Format(NSLOCTEXT("USMetaUpgradeData", "ChanceEffect", "+{0}%"), FText::AsNumber(ValuePerLevel * Steps * 100.0f, &Format));

	case EUSMetaEffect::CardReroll:
		return FText::Format(NSLOCTEXT("USMetaUpgradeData", "CountEffect", "+{0}회"), FText::AsNumber(ValuePerLevel * Steps, &Format));

	default:
		return FText::GetEmpty();
	}
}
