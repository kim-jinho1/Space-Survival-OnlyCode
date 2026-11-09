#include "UI/USGameOverWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

UUSGameOverWidget::UUSGameOverWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DestroyedTitle = NSLOCTEXT("USGameOverWidget", "DestroyedTitle", "격추됨");
	ReturnedTitle = NSLOCTEXT("USGameOverWidget", "ReturnedTitle", "귀환");
	ClearedDestroyedTitle = NSLOCTEXT("USGameOverWidget", "ClearedDestroyedTitle", "클리어 후 격추");
	StageClearTitleFormat = NSLOCTEXT("USGameOverWidget", "StageClearTitleFormat", "스테이지 {0} 클리어");
	AbandonedTitle = NSLOCTEXT("USGameOverWidget", "AbandonedTitle", "출격 포기");
	TotalLabel = NSLOCTEXT("USGameOverWidget", "TotalLabel", "합계");
	CareerFormat = NSLOCTEXT("USGameOverWidget", "CareerFormat", "누적 출격 {0}회 · 클리어 {1}회");
}

void UUSGameOverWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// BindWidget이라 WBP에 없으면 컴파일 단계에서 막히지만, 다시 열릴 때 중복 바인딩되지 않게 AddUnique
	if (HangarButton)
	{
		HangarButton->OnClicked.AddUniqueDynamic(this, &UUSGameOverWidget::HandleHangarClicked);
	}
	if (TitleButton)
	{
		TitleButton->OnClicked.AddUniqueDynamic(this, &UUSGameOverWidget::HandleTitleClicked);
	}
	if (RetryButton)
	{
		RetryButton->OnClicked.AddUniqueDynamic(this, &UUSGameOverWidget::HandleRetryClicked);
	}
}

void UUSGameOverWidget::SetupResult(const FUSRunResult& Result, const FUSRunRecords& Records)
{
	if (HeaderText)
	{
		HeaderText->SetText(GetHeaderTitle(Result));

		// 귀환·클리어는 성공이다. 격추와 같은 빨강이면 실패처럼 읽힌다
		const bool bSuccess = Result.EndReason == EUSRunEndReason::Returned || Result.bCleared;
		HeaderText->SetColorAndOpacity(bSuccess ? SuccessTitleColor : FailTitleColor);
	}

	if (RunSummary)
	{
		RunSummary->SetupSummary(Result, Records, TotalLabel, true);
	}

	// 기록이 없으면(서브시스템 없음) 누적 줄을 숨긴다. "누적 0회"는 틀린 정보다
	if (CareerText)
	{
		CareerText->SetText(FText::Format(CareerFormat, FText::AsNumber(Records.RunCount), FText::AsNumber(Records.ClearCount)));
		CareerText->SetVisibility(Records.bValid ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UUSGameOverWidget::SetStageClearTitle(int32 StageNumber)
{
	if (HeaderText)
	{
		HeaderText->SetText(FText::Format(StageClearTitleFormat, FText::AsNumber(StageNumber)));
		HeaderText->SetColorAndOpacity(SuccessTitleColor);
	}
}

const FText& UUSGameOverWidget::GetHeaderTitle(const FUSRunResult& Result) const
{
	switch (Result.EndReason)
	{
	case EUSRunEndReason::Returned:
		return ReturnedTitle;
	case EUSRunEndReason::Abandoned:
		return AbandonedTitle;
	default:
		return Result.bCleared ? ClearedDestroyedTitle : DestroyedTitle;
	}
}

void UUSGameOverWidget::HandleHangarClicked()
{
	OnHangarRequested.ExecuteIfBound();
}

void UUSGameOverWidget::HandleTitleClicked()
{
	OnTitleRequested.ExecuteIfBound();
}

void UUSGameOverWidget::HandleRetryClicked()
{
	OnRetryRequested.ExecuteIfBound();
}
