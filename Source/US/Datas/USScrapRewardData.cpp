#include "Datas/USScrapRewardData.h"

namespace
{
	constexpr float ScrapSecondsPerMinute = 60.0f;
}

void UUSScrapRewardData::CalculateScrap(FUSRunResult& InOutResult, float Multiplier) const
{
	const float Scale = FMath::Max(0.0f, Multiplier);
	InOutResult.KillScrap = FMath::FloorToInt(InOutResult.KillCount * ScrapPerKill * Scale);
	InOutResult.TimeScrap = FMath::FloorToInt(InOutResult.SurvivedSeconds / ScrapSecondsPerMinute * ScrapPerMinute * Scale);
	// 스테이지 판은 스테이지마다 조금 + 행성 클리어에 큰 보너스, 스테이지 없는 판은 예전처럼 클리어 한 번
	const bool bStageRun = InOutResult.StagesCleared > 0;
	const bool bBigClear = bStageRun ? InOutResult.bPlanetCleared : InOutResult.bCleared;
	const float StageScale = 1.0f + StageClearBonusGrowth * static_cast<float>(FMath::Max(0, InOutResult.StageNumber - 1));
	const float StageTotal = bStageRun ? InOutResult.StagesCleared * StageClearBonus * StageScale : 0.0f;
	const float ClearTotal = StageTotal + (bBigClear ? ClearBonus : 0);
	InOutResult.ClearScrap = FMath::FloorToInt(ClearTotal * Scale);

	// 구간을 다 채운 만큼만 준다 (29초 버티고 격추되면 0)
	const int32 ContinueSteps = FMath::FloorToInt(FMath::Max(0.0f, InOutResult.ContinueSeconds) / ContinueStepSeconds);
	InOutResult.ContinueScrap = FMath::FloorToInt(ContinueSteps * ContinueBonusPerStep * Scale);

	InOutResult.TotalScrap = InOutResult.KillScrap + InOutResult.TimeScrap + InOutResult.ClearScrap + InOutResult.ContinueScrap;
}
