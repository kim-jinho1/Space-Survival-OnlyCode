#include "UI/USRunSummaryWidget.h"
#include "UI/USClockText.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"

UUSRunSummaryWidget::UUSRunSummaryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TimeFormat = NSLOCTEXT("USRunSummaryWidget", "TimeFormat", "{0}:{1}");
	ScrapFormat = NSLOCTEXT("USRunSummaryWidget", "ScrapFormat", "+{0}");
	ClearedText = NSLOCTEXT("USRunSummaryWidget", "ClearedText", "달성");
	NotClearedText = NSLOCTEXT("USRunSummaryWidget", "NotClearedText", "-");
	StagesClearedFormat = NSLOCTEXT("USRunSummaryWidget", "StagesClearedFormat", "{0}곳");
	NoRecordText = NSLOCTEXT("USRunSummaryWidget", "NoRecordText", "-");
}

void UUSRunSummaryWidget::SetupSummary(const FUSRunResult& Result, const FUSRunRecords& Records, const FText& TotalLabel, bool bCountUp)
{
	SetupRecordColumn(Result, Records);

	if (SurvivalTimeText)
	{
		SurvivalTimeText->SetText(FormatTime(Result.SurvivedSeconds));
	}
	if (TimeScrapText)
	{
		TimeScrapText->SetText(FormatScrap(Result.TimeScrap));
	}

	if (KillText)
	{
		KillText->SetText(FText::AsNumber(Result.KillCount));
	}
	if (KillScrapText)
	{
		KillScrapText->SetText(FormatScrap(Result.KillScrap));
	}

	// 클리어 줄은 못 했어도 남긴다: "+0"이 보여야 다음 판 목표(클리어 보너스)가 눈에 들어온다
	if (ClearText)
	{
		const FText Cleared = Result.StagesCleared > 0 ? FText::Format(StagesClearedFormat, FText::AsNumber(Result.StagesCleared)) : ClearedText;
		ClearText->SetText(Result.bCleared ? Cleared : NotClearedText);
		ClearText->SetColorAndOpacity(Result.bCleared ? ClearedColor : NotClearedColor);
	}
	if (ClearScrapText)
	{
		ClearScrapText->SetText(FormatScrap(Result.ClearScrap));
	}

	// 계속 가기 줄은 계속 가기를 고른 판에만 의미가 있다
	if (ContinueRow)
	{
		ContinueRow->SetVisibility(Result.ContinueSeconds > 0.0f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (ContinueTimeText)
	{
		ContinueTimeText->SetText(FormatTime(Result.ContinueSeconds));
	}
	if (ContinueScrapText)
	{
		ContinueScrapText->SetText(FormatScrap(Result.ContinueScrap));
	}

	if (LevelText)
	{
		LevelText->SetText(FText::AsNumber(Result.Level));
	}

	if (TotalLabelText)
	{
		TotalLabelText->SetText(TotalLabel);
	}

	TargetScrap = Result.TotalScrap;
	if (bCountUp && CountUpDuration > 0.0f && TargetScrap > 0)
	{
		CountUpElapsed = 0.0f;
		SetTotalScrapText(0);
	}
	else
	{
		CountUpElapsed = -1.0f;
		SetTotalScrapText(TargetScrap);
	}
}

void UUSRunSummaryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 연출이 끝났으면 바로 빠진다 (창이 떠 있는 동안 계속 틱이 돌므로)
	if (CountUpElapsed < 0.0f)
	{
		return;
	}

	// InDeltaTime은 Slate 기준 실제 시간이라 일시정지·slomo와 상관없이 흐른다
	CountUpElapsed += InDeltaTime;
	const float Alpha = FMath::Clamp(CountUpElapsed / CountUpDuration, 0.0f, 1.0f);

	// 끝에서 천천히 멈추게 (ease-out) 해 마지막 숫자가 눈에 들어오게 한다
	const float Eased = 1.0f - FMath::Square(1.0f - Alpha);
	SetTotalScrapText(FMath::RoundToInt(TargetScrap * Eased));

	if (Alpha >= 1.0f)
	{
		CountUpElapsed = -1.0f;
	}
}

void UUSRunSummaryWidget::SetupRecordColumn(const FUSRunResult& Result, const FUSRunRecords& Records)
{
	// 기록이 없으면(서브시스템 없음) 최고 열을 통째로 숨긴다. 0으로 보이면 "최고 0:00"처럼 틀린 정보가 된다
	const ESlateVisibility RecordVisibility = Records.bValid ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;

	// 첫 출격은 이전 기록이 0이라 무엇을 해도 "갱신"이 되므로 이전 기록이 있을 때만 켠다
	const bool bNewBestKill = Records.bValid && Records.PreviousBestKills > 0 && Result.KillCount > Records.PreviousBestKills;

	if (BestHeader)
	{
		BestHeader->SetVisibility(RecordVisibility);
	}

	// 스테이지는 1:30 고정이라 오래 버틴 시간은 실력 기록이 아니다 → 시간 최고 칸·갱신 표시는 쓰지 않는다.
	// 칸은 표 열 정렬을 지키려고 Collapsed 대신 Hidden (생존 시간 줄은 시간 스크랩 근거라 남긴다)
	if (BestTimeText)
	{
		BestTimeText->SetVisibility(ESlateVisibility::Hidden);
	}

	// 최고 칸은 이번 판 "전" 기록이다: 이번 판과 나란히 놓아 얼마나 넘었는지(또는 모자랐는지) 보이게 한다
	if (BestKillText)
	{
		BestKillText->SetText(Records.PreviousBestKills > 0 ? FText::AsNumber(Records.PreviousBestKills) : NoRecordText);
		BestKillText->SetVisibility(RecordVisibility);
	}
	if (NewBestTimeBadge)
	{
		NewBestTimeBadge->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (NewBestKillBadge)
	{
		NewBestKillBadge->SetVisibility(bNewBestKill ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UUSRunSummaryWidget::SetTotalScrapText(int32 Value) const
{
	if (TotalScrapText)
	{
		TotalScrapText->SetText(FormatScrap(Value));
	}
}

FText UUSRunSummaryWidget::FormatTime(float Seconds) const
{
	FText Minutes;
	FText SecondsText;
	USClockText::Split(Seconds, Minutes, SecondsText);
	return FText::Format(TimeFormat, Minutes, SecondsText);
}

FText UUSRunSummaryWidget::FormatScrap(int32 Scrap) const
{
	return FText::Format(ScrapFormat, FText::AsNumber(Scrap));
}
