#include "UI/USOffscreenIndicatorWidget.h"
#include "UI/USEdgeArrowUtils.h"
#include "Components/Image.h"

void UUSOffscreenIndicatorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 대상이 정해지기 전에는 보이지 않게 한다
	SetArrowVisible(false);
}

void UUSOffscreenIndicatorWidget::SetTarget(AActor* InTarget)
{
	Target = InTarget;

	if (InTarget == nullptr)
	{
		SetArrowVisible(false);
	}
}

void UUSOffscreenIndicatorWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const AActor* TargetActor = Target.Get();
	if (TargetActor == nullptr || ArrowImage == nullptr)
	{
		return;
	}

	// 대상이 여백만큼 안쪽에 들어오면 화면 안으로 보고 숨긴다
	FUSEdgeArrowPlacement Placement;
	if (!USEdgeArrowUtils::TryPlaceAtScreenEdge(*this, TargetActor->GetActorLocation(), EdgeMargin, EdgeMargin, Placement))
	{
		SetArrowVisible(false);
		return;
	}

	if (BadgeWidget)
	{
		BadgeWidget->SetRenderTranslation(Placement.EdgeOffset);
		ArrowImage->SetRenderTranslation(Placement.EdgeOffset + Placement.Direction * ArrowDistance);
	}
	else
	{
		ArrowImage->SetRenderTranslation(Placement.EdgeOffset);
	}
	ArrowImage->SetRenderTransformAngle(Placement.AngleDegrees + ArrowAngleOffset);
	SetArrowVisible(true);
}

void UUSOffscreenIndicatorWidget::SetArrowVisible(bool bVisible)
{
	if (ArrowImage == nullptr || bArrowVisible == bVisible)
	{
		return;
	}

	bArrowVisible = bVisible;
	const ESlateVisibility NewVisibility = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
	ArrowImage->SetVisibility(NewVisibility);
	if (BadgeWidget)
	{
		BadgeWidget->SetVisibility(NewVisibility);
	}
}
