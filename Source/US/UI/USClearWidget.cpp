#include "UI/USClearWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

UUSClearWidget::UUSClearWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Subtitle = NSLOCTEXT("USClearWidget", "Subtitle", "1구역 클리어!");
	TotalLabel = NSLOCTEXT("USClearWidget", "TotalLabel", "지금까지");
	ContinueHintFormat = NSLOCTEXT("USClearWidget", "ContinueHintFormat", "계속하면 {0}초마다 적이 강해집니다 (+{1})");
	DemoNotice = NSLOCTEXT("USClearWidget", "DemoNotice", "데모는 여기까지입니다. 플레이해 주셔서 감사합니다!\n정식판이 궁금하다면 스토브에서 찜해 주세요.");
	DefeatedBossFormat = NSLOCTEXT("USClearWidget", "DefeatedBossFormat", "{0} 격파!");
}

void UUSClearWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LeaveButton)
	{
		LeaveButton->OnClicked.AddUniqueDynamic(this, &UUSClearWidget::HandleLeaveClicked);
	}
	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddUniqueDynamic(this, &UUSClearWidget::HandleContinueClicked);
	}

	// 보스를 잡고 클리어했을 때만 ShowDefeatedBoss가 연다
	if (BossPanel)
	{
		BossPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UUSClearWidget::ShowDefeatedBoss(UTexture2D* Image, const FText& BossName)
{
	if (BossImage)
	{
		if (Image)
		{
			BossImage->SetBrushFromTexture(Image);
		}
		BossImage->SetVisibility(Image ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (BossNameText)
	{
		BossNameText->SetText(FText::Format(DefeatedBossFormat, BossName));
	}
	if (BossPanel)
	{
		BossPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UUSClearWidget::SetupClear(const FUSRunResult& Result, const FUSRunRecords& Records, int32 ContinueBonusPerStep, float StepSeconds)
{
	if (SubtitleText)
	{
		SubtitleText->SetText(Subtitle);
	}

	// 클리어 창은 판 중간에 떠서 곧 사라질 수 있으므로 합계 올리기 연출 없이 바로 보여 준다
	if (RunSummary)
	{
		RunSummary->SetupSummary(Result, Records, TotalLabel, false);
	}

	if (DemoNoticeText)
	{
		DemoNoticeText->SetText(DemoNotice);
	}
	if (DemoNoticePanel)
	{
		DemoNoticePanel->SetVisibility(DemoNotice.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	// 데모는 "여기까지"라고 말한 뒤 계속 가기를 보이면 어색하다 → 데모 문구가 있으면 격납고로만 남긴다
	const bool bDemoEnd = !DemoNotice.IsEmpty();
	if (ContinueButton)
	{
		ContinueButton->SetVisibility(bDemoEnd ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
	if (ContinueHintText)
	{
		ContinueHintText->SetVisibility(bDemoEnd ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (ContinueHintText)
	{
		ContinueHintText->SetText(FText::Format(ContinueHintFormat,
			FText::AsNumber(FMath::RoundToInt(StepSeconds)), FText::AsNumber(ContinueBonusPerStep)));
	}
}

void UUSClearWidget::HandleLeaveClicked()
{
	OnLeaveRequested.ExecuteIfBound();
}

void UUSClearWidget::HandleContinueClicked()
{
	OnContinueRequested.ExecuteIfBound();
}
