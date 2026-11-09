#include "UI/USSupplyMarkerWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "Shop/USSupplyShip.h"

UUSSupplyMarkerWidget::UUSSupplyMarkerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TimeFormat = NSLOCTEXT("USSupplyMarkerWidget", "TimeFormat", "{0}:{1}");
	InteractPrompt = NSLOCTEXT("USSupplyMarkerWidget", "InteractPrompt", "[Q] 상점");
}

void UUSSupplyMarkerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (InteractText)
	{
		InteractText->SetText(InteractPrompt);
		InteractText->SetVisibility(ESlateVisibility::Collapsed);
	}
	bPromptShown = false;
}

void UUSSupplyMarkerWidget::SetSupplyShip(AUSSupplyShip* InSupplyShip)
{
	SupplyShip = InSupplyShip;
	LastShownSeconds = INDEX_NONE;
}

void UUSSupplyMarkerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 남은 시간은 보급선 상태 타이머에서 읽는 값이라 매 프레임 읽어도 싸다 (막대가 부드럽게 줄도록 매 프레임)
	const AUSSupplyShip* Ship = SupplyShip.Get();
	if (Ship == nullptr)
	{
		return;
	}

	const float Remaining = Ship->GetStayTimeRemaining();
	const float Duration = Ship->GetStayDuration();
	if (TimeBar && Duration > 0.0f)
	{
		TimeBar->SetPercent(FMath::Clamp(Remaining / Duration, 0.0f, 1.0f));
	}

	if (InteractText)
	{
		const APawn* Leader = GetOwningPlayerPawn();
		const bool bInRange = Leader && Ship->IsInInteractRange(Leader->GetActorLocation());
		if (bInRange != bPromptShown)
		{
			bPromptShown = bInRange;
			InteractText->SetVisibility(bInRange ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}

	const int32 Seconds = FMath::CeilToInt(Remaining);
	if (TimeText && Seconds != LastShownSeconds)
	{
		LastShownSeconds = Seconds;
		FNumberFormattingOptions TwoDigits;
		TwoDigits.MinimumIntegralDigits = 2;
		TimeText->SetText(FText::Format(TimeFormat, FText::AsNumber(Seconds / 60), FText::AsNumber(Seconds % 60, &TwoDigits)));
	}
}
