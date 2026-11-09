#include "UI/USSupplyNoticeWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

UUSSupplyNoticeWidget::UUSSupplyNoticeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ArrivingFormat = NSLOCTEXT("USSupplyNoticeWidget", "ArrivingFormat", "보급선이 {0}초 후 도착합니다");
	ArrivedText = NSLOCTEXT("USSupplyNoticeWidget", "ArrivedText", "보급선 도착! 가까이 가서 Q를 누르세요");
	DepartingFormat = NSLOCTEXT("USSupplyNoticeWidget", "DepartingFormat", "보급선이 {0}초 후 떠납니다");
	DepartedText = NSLOCTEXT("USSupplyNoticeWidget", "DepartedText", "보급선이 떠났습니다");
}

void UUSSupplyNoticeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 알릴 것이 생기기 전에는 숨긴다
	if (NoticeText)
	{
		NoticeText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UUSSupplyNoticeWidget::NativeDestruct()
{
	ClearTimers();

	Super::NativeDestruct();
}

void UUSSupplyNoticeWidget::HandleSupplyStateChanged(EUSSupplyShipState NewState, float SecondsRemaining)
{
	ClearTimers();

	const bool bWasDeparting = bDepartureAnnounced;
	bDepartureAnnounced = NewState == EUSSupplyShipState::Departing;

	switch (NewState)
	{
	case EUSSupplyShipState::Arriving:
	case EUSSupplyShipState::Departing:
	{
		CountdownFormat = NewState == EUSSupplyShipState::Arriving ? ArrivingFormat : DepartingFormat;
		RemainingSeconds = FMath::CeilToInt(SecondsRemaining);
		ShowCountdownText();

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(CountdownTimerHandle, this, &UUSSupplyNoticeWidget::HandleCountdownTick, 1.0f, true);
		}

		if (NoticeSound)
		{
			PlaySound(NoticeSound);
		}
		break;
	}

	case EUSSupplyShipState::Present:
		ShowMessage(ArrivedText, MessageHoldTime);
		if (NoticeSound)
		{
			PlaySound(NoticeSound);
		}
		break;

	case EUSSupplyShipState::Absent:
		// 실제로 떠났을 때만 알린다. 판 시작 직후의 '없음'은 떠난 게 아니다 (플레이테스트 2026-10-20)
		if (bWasDeparting)
		{
			ShowMessage(DepartedText, MessageHoldTime);
		}
		break;

	default:
		break;
	}
}

void UUSSupplyNoticeWidget::HandleCountdownTick()
{
	RemainingSeconds = FMath::Max(0, RemainingSeconds - 1);
	ShowCountdownText();

	// 0초가 되면 곧 보급선이 상태를 바꿔 다음 문구를 보낸다
	if (RemainingSeconds <= 0)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(CountdownTimerHandle);
		}
	}
}

void UUSSupplyNoticeWidget::HandleHoldFinished()
{
	if (NoticeText)
	{
		NoticeText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UUSSupplyNoticeWidget::ShowCountdownText()
{
	ShowMessage(FText::Format(CountdownFormat, FText::AsNumber(RemainingSeconds)), 0.0f);
}

void UUSSupplyNoticeWidget::ShowMessage(const FText& Message, float HoldTime)
{
	if (NoticeText == nullptr)
	{
		return;
	}

	NoticeText->SetText(Message);
	NoticeText->SetVisibility(ESlateVisibility::HitTestInvisible);

	UWorld* World = GetWorld();
	if (HoldTime > 0.0f && World)
	{
		World->GetTimerManager().SetTimer(HoldTimerHandle, this, &UUSSupplyNoticeWidget::HandleHoldFinished, HoldTime, false);
	}
}

void UUSSupplyNoticeWidget::ClearTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CountdownTimerHandle);
		World->GetTimerManager().ClearTimer(HoldTimerHandle);
	}
}
