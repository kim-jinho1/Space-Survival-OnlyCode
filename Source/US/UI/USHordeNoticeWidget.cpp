#include "UI/USHordeNoticeWidget.h"
#include "UI/USEdgeArrowUtils.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

UUSHordeNoticeWidget::UUSHordeNoticeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 기본은 문구 없음 — 화살표·바닥 X·소리만으로 알린다. 필요하면 WBP에서 넣는다
	RushText = FText::GetEmpty();
	RingText = FText::GetEmpty();
}

void UUSHordeNoticeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 예고가 오기 전에는 숨긴다
	HandleHoldFinished();
}

void UUSHordeNoticeWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HoldTimerHandle);
		World->GetTimerManager().ClearTimer(BlinkTimerHandle);
	}

	Super::NativeDestruct();
}

void UUSHordeNoticeWidget::HandleHordeWarning(EUSHordeShape Shape, const FVector& HordeCenter, float SecondsUntilSpawn)
{
	// 문구는 비워 둘 수 있다 (사용자 2026-10-15: 화살표·X·소리로 충분, 명령조 문구는 거슬림) → 비었으면 글자를 띄우지 않는다
	const FText& Message = Shape == EUSHordeShape::Ring ? RingText : RushText;
	if (NoticeText)
	{
		NoticeText->SetText(Message);
		NoticeText->SetVisibility(Message.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	UWorld* World = GetWorld();
	if (WarningArrow)
	{
		// 예고 순간 화면 안이어도 추적은 한다 — 예고 중에 대장이 멀어지면 화면 밖으로 나가 화살표가 필요해진다
		ArrowTargetLocation = HordeCenter;
		bTrackingArrow = Shape == EUSHordeShape::Rush;
		UpdateWarningArrow();

		bBlinkBright = true;
		WarningArrow->SetRenderOpacity(1.0f);
		if (World)
		{
			if (bTrackingArrow)
			{
				World->GetTimerManager().SetTimer(BlinkTimerHandle, this, &UUSHordeNoticeWidget::HandleBlink, BlinkInterval, true);
			}
			else
			{
				World->GetTimerManager().ClearTimer(BlinkTimerHandle);
			}
		}
	}

	if (WarningSound)
	{
		PlaySound(WarningSound);
	}

	if (World)
	{
		World->GetTimerManager().SetTimer(HoldTimerHandle, this, &UUSHordeNoticeWidget::HandleHoldFinished,
			FMath::Max(MinHoldTime, SecondsUntilSpawn), false);
	}
}

void UUSHordeNoticeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bTrackingArrow || WarningArrow == nullptr)
	{
		return;
	}

	// 카메라가 움직여도 고정된 무리 자리를 계속 가리키고, 그 자리가 화면에 들어오면 숨긴다
	UpdateWarningArrow();
}

void UUSHordeNoticeWidget::HandleHoldFinished()
{
	bTrackingArrow = false;

	if (NoticeText)
	{
		NoticeText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (WarningArrow)
	{
		WarningArrow->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BlinkTimerHandle);
	}
}

void UUSHordeNoticeWidget::HandleBlink()
{
	if (WarningArrow)
	{
		bBlinkBright = !bBlinkBright;
		WarningArrow->SetRenderOpacity(bBlinkBright ? 1.0f : BlinkDimOpacity);
	}
}

void UUSHordeNoticeWidget::UpdateWarningArrow()
{
	if (WarningArrow == nullptr)
	{
		return;
	}

	FUSEdgeArrowPlacement Placement;
	const bool bShowArrow = bTrackingArrow && USEdgeArrowUtils::TryPlaceAtScreenEdge(*this, ArrowTargetLocation, EdgeMargin, EdgeMargin, Placement);
	if (bShowArrow)
	{
		WarningArrow->SetRenderTranslation(Placement.EdgeOffset);
		WarningArrow->SetRenderTransformAngle(Placement.AngleDegrees + ArrowAngleOffset);
	}

	// 바뀔 때만 설정한다 (매 프레임 같은 값을 넣으면 Slate가 다시 배치한다)
	const ESlateVisibility NewVisibility = bShowArrow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	if (WarningArrow->GetVisibility() != NewVisibility)
	{
		WarningArrow->SetVisibility(NewVisibility);
	}
}
