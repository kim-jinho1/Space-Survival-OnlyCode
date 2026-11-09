#include "UI/USPauseWidget.h"
#include "UI/USClockText.h"
#include "UI/USSettingsWidget.h"
#include "Datas/USEnemyTypeData.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

UUSPauseWidget::UUSPauseWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RemainingLabel = NSLOCTEXT("USPauseWidget", "RemainingLabel", "남은 시간");
	ContinueLabel = NSLOCTEXT("USPauseWidget", "ContinueLabel", "계속 가기");
	ClockFormat = NSLOCTEXT("USPauseWidget", "ClockFormat", "{0}:{1}");
	ContinueClockFormat = NSLOCTEXT("USPauseWidget", "ContinueClockFormat", "+{0}:{1}");
	AllyFormat = NSLOCTEXT("USPauseWidget", "AllyFormat", "{0} / {1}대");
	QuitLabelText = NSLOCTEXT("USPauseWidget", "QuitLabel", "게임 종료");
	QuitConfirmLabelText = NSLOCTEXT("USPauseWidget", "QuitConfirmLabel", "한 번 더 누르면 종료됩니다");
}

void UUSPauseWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// ↑/↓·Enter를 받으려면 포커스를 받을 수 있어야 한다 (포커스는 컨트롤러가 창을 띄운 뒤 준다)
	SetIsFocusable(true);

	// BindWidget이라 WBP에 없으면 컴파일 단계에서 막히지만, 다시 열릴 때 중복 바인딩되지 않게 AddUnique
	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddUniqueDynamic(this, &UUSPauseWidget::HandleResumeClicked);
		ResumeButton->OnHovered.AddUniqueDynamic(this, &UUSPauseWidget::HandleResumeHovered);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked.AddUniqueDynamic(this, &UUSPauseWidget::HandleSettingsClicked);
		SettingsButton->OnHovered.AddUniqueDynamic(this, &UUSPauseWidget::HandleSettingsHovered);
	}
	if (AbandonButton)
	{
		AbandonButton->OnClicked.AddUniqueDynamic(this, &UUSPauseWidget::HandleAbandonClicked);
		AbandonButton->OnHovered.AddUniqueDynamic(this, &UUSPauseWidget::HandleAbandonHovered);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &UUSPauseWidget::HandleQuitClicked);
		QuitButton->OnHovered.AddUniqueDynamic(this, &UUSPauseWidget::HandleQuitHovered);
	}
	if (QuitLabel)
	{
		QuitLabel->SetText(QuitLabelText);
	}

	if (SettingsPanel)
	{
		SettingsPanel->OnClosed.BindUObject(this, &UUSPauseWidget::HandleSettingsClosed);
	}

	// 스타일은 WBP에서 칠해 둔 계속(고른 모양)·설정(나머지 모양) 버튼을 그대로 쓴다
	Menu.Initialize({ ResumeButton.Get(), SettingsButton.Get(), AbandonButton.Get(), QuitButton.Get() }, SelectedLabelColor, IdleLabelColor,
		{ QuitButton.Get() });
}

void UUSPauseWidget::NativeDestruct()
{
	FTSTicker::GetCoreTicker().RemoveTicker(QuitConfirmTickerHandle);
	QuitConfirmTickerHandle.Reset();
	bQuitArmed = false;

	Super::NativeDestruct();
}

FReply UUSPauseWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// 설정 창이 떠 있으면 그 창이 키를 쓴다 (Esc/P는 컨트롤러가 받아 HandleBackRequested로 온다)
	if (SettingsPanel && SettingsPanel->IsOpen())
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}

	switch (Menu.HandleKey(InKeyEvent.GetKey()))
	{
	case FUSKeyMenu::EResult::Moved:
		return FReply::Handled();
	case FUSKeyMenu::EResult::Activate:
		switch (Menu.GetSelectedIndex())
		{
		case 0:
			HandleResumeClicked();
			break;
		case 1:
			HandleSettingsClicked();
			break;
		case 2:
			HandleAbandonClicked();
			break;
		case 3:
			HandleQuitClicked();
			break;
		default:
			break;
		}
		return FReply::Handled();
	default:
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
}

void UUSPauseWidget::ShowSnapshot(const FUSPauseSnapshot& Snapshot)
{
	if (ClockLabelText)
	{
		ClockLabelText->SetText(Snapshot.bContinuing ? ContinueLabel : RemainingLabel);
	}
	if (ClockText)
	{
		FText Minutes;
		FText Seconds;
		USClockText::Split(Snapshot.ClockSeconds, Minutes, Seconds);
		ClockText->SetText(FText::Format(Snapshot.bContinuing ? ContinueClockFormat : ClockFormat, Minutes, Seconds));
	}
	if (LevelText)
	{
		LevelText->SetText(FText::AsNumber(Snapshot.Level));
	}
	if (KillText)
	{
		KillText->SetText(FText::AsNumber(Snapshot.KillCount));
	}
	if (GoldText)
	{
		GoldText->SetText(FText::AsNumber(Snapshot.Gold));
	}
	if (AllyText)
	{
		AllyText->SetText(FText::Format(AllyFormat, FText::AsNumber(Snapshot.AllyCount), FText::AsNumber(Snapshot.MaxAllies)));
	}
	if (MetEnemiesText)
	{
		TArray<FText> Lines;
		for (const UUSEnemyTypeData* Enemy : Snapshot.MetEnemies)
		{
			if (Enemy)
			{
				Lines.Add(Enemy->GetSummary().IsEmpty() ? Enemy->GetDisplayName() : FText::Format(MetEnemyLineFormat, Enemy->GetDisplayName(), Enemy->GetSummary()));
			}
		}
		MetEnemiesText->SetText(Lines.IsEmpty() ? NoMetEnemiesText : FText::Join(FText::FromString(TEXT("\n")), Lines));
	}
}

void UUSPauseWidget::HandleBackRequested()
{
	if (SettingsPanel && SettingsPanel->IsOpen())
	{
		SettingsPanel->Close();
		return;
	}

	OnResumeRequested.ExecuteIfBound();
}

void UUSPauseWidget::HandleSettingsClosed()
{
	if (StatsPanel)
	{
		StatsPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	// 설정 창의 닫기 버튼을 누르면 포커스가 그 버튼(이제 숨겨짐)에 남는다. 메뉴로 돌려줘야 화살표가 다시 먹는다
	SetKeyboardFocus();
}

void UUSPauseWidget::HandleResumeClicked()
{
	OnResumeRequested.ExecuteIfBound();
}

void UUSPauseWidget::HandleSettingsClicked()
{
	if (SettingsPanel)
	{
		if (StatsPanel)
		{
			StatsPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
		SettingsPanel->Open();
	}
}

void UUSPauseWidget::HandleAbandonClicked()
{
	OnAbandonRequested.ExecuteIfBound();
}

void UUSPauseWidget::HandleQuitClicked()
{
	if (!bQuitArmed)
	{
		bQuitArmed = true;
		if (QuitLabel)
		{
			QuitLabel->SetText(QuitConfirmLabelText);
		}
		FTSTicker::GetCoreTicker().RemoveTicker(QuitConfirmTickerHandle);
		QuitConfirmTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &UUSPauseWidget::HandleQuitConfirmExpired), QuitConfirmSeconds);
		return;
	}

	FTSTicker::GetCoreTicker().RemoveTicker(QuitConfirmTickerHandle);
	QuitConfirmTickerHandle.Reset();
	bQuitArmed = false;
	OnQuitRequested.ExecuteIfBound();
}

bool UUSPauseWidget::HandleQuitConfirmExpired(float DeltaTime)
{
	bQuitArmed = false;
	QuitConfirmTickerHandle.Reset();
	if (QuitLabel)
	{
		QuitLabel->SetText(QuitLabelText);
	}
	return false; // 한 번만 부른다
}

void UUSPauseWidget::HandleQuitHovered()
{
	Menu.Select(3);
}

void UUSPauseWidget::HandleResumeHovered()
{
	Menu.Select(0);
}

void UUSPauseWidget::HandleSettingsHovered()
{
	Menu.Select(1);
}

void UUSPauseWidget::HandleAbandonHovered()
{
	Menu.Select(2);
}
