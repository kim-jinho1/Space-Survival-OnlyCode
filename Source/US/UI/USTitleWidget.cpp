#include "UI/USTitleWidget.h"
#include "UI/USCreditsWidget.h"
#include "UI/USSettingsWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GeneralProjectSettings.h"

void UUSTitleWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// BindWidget이라 WBP에 없으면 컴파일 단계에서 막히지만, 다시 열릴 때 중복 바인딩되지 않게 AddUnique
	if (StartButton)
	{
		StartButton->OnClicked.AddUniqueDynamic(this, &UUSTitleWidget::HandleStartClicked);
	}
	if (SettingsButton)
	{
		SettingsButton->OnClicked.AddUniqueDynamic(this, &UUSTitleWidget::HandleSettingsClicked);
	}
	if (CreditsButton)
	{
		CreditsButton->OnClicked.AddUniqueDynamic(this, &UUSTitleWidget::HandleCreditsClicked);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &UUSTitleWidget::HandleQuitClicked);
	}

	// 마우스를 올린 항목 = 고른 항목. 마우스와 키보드가 같은 강조 하나를 쓴다
	if (StartButton)
	{
		StartButton->OnHovered.AddUniqueDynamic(this, &UUSTitleWidget::HandleStartHovered);
	}
	if (SettingsButton)
	{
		SettingsButton->OnHovered.AddUniqueDynamic(this, &UUSTitleWidget::HandleSettingsHovered);
	}
	if (CreditsButton)
	{
		CreditsButton->OnHovered.AddUniqueDynamic(this, &UUSTitleWidget::HandleCreditsHovered);
	}
	if (QuitButton)
	{
		QuitButton->OnHovered.AddUniqueDynamic(this, &UUSTitleWidget::HandleQuitHovered);
	}

	if (SettingsPanel)
	{
		SettingsPanel->OnClosed.BindUObject(this, &UUSTitleWidget::HandlePanelClosed);
	}
	if (CreditsPanel)
	{
		CreditsPanel->OnClosed.BindUObject(this, &UUSTitleWidget::HandlePanelClosed);
	}

	if (VersionText)
	{
		const FString& Version = GetDefault<UGeneralProjectSettings>()->ProjectVersion;
		VersionText->SetText(FText::Format(VersionFormat, FText::FromString(Version)));
	}

	// 스타일을 C++ 기본값으로 따로 두지 않고 WBP에서 칠해 둔 시작(고른 모양)·설정(나머지 모양) 버튼을 그대로 쓴다
	Menu.Initialize({ StartButton.Get(), SettingsButton.Get(), CreditsButton.Get(), QuitButton.Get() }, SelectedLabelColor, IdleLabelColor);
}

FReply UUSTitleWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// 설정·크레딧 창이 떠 있으면 그 창이 키를 쓴다
	if (IsPanelOpen())
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}

	switch (Menu.HandleKey(InKeyEvent.GetKey()))
	{
	case FUSKeyMenu::EResult::Moved:
		return FReply::Handled();
	case FUSKeyMenu::EResult::Activate:
		ActivateSelected();
		return FReply::Handled();
	default:
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
}

void UUSTitleWidget::ActivateSelected()
{
	switch (Menu.GetSelectedIndex())
	{
	case 0:
		HandleStartClicked();
		break;
	case 1:
		HandleSettingsClicked();
		break;
	case 2:
		HandleCreditsClicked();
		break;
	case 3:
		HandleQuitClicked();
		break;
	default:
		break;
	}
}

bool UUSTitleWidget::IsPanelOpen() const
{
	return (SettingsPanel && SettingsPanel->IsOpen()) || (CreditsPanel && CreditsPanel->IsOpen());
}

void UUSTitleWidget::HandlePanelClosed()
{
	// 창의 닫기 버튼을 누르면 포커스가 그 버튼(이제 숨겨짐)에 남는다. 타이틀로 돌려줘야 화살표가 다시 먹는다
	SetKeyboardFocus();
}

void UUSTitleWidget::HandleStartHovered()
{
	Menu.Select(0);
}

void UUSTitleWidget::HandleSettingsHovered()
{
	Menu.Select(1);
}

void UUSTitleWidget::HandleCreditsHovered()
{
	Menu.Select(2);
}

void UUSTitleWidget::HandleQuitHovered()
{
	Menu.Select(3);
}

void UUSTitleWidget::HandleStartClicked()
{
	OnStartRequested.ExecuteIfBound();
}

void UUSTitleWidget::HandleSettingsClicked()
{
	if (SettingsPanel)
	{
		SettingsPanel->Open();
	}
}

void UUSTitleWidget::HandleCreditsClicked()
{
	if (CreditsPanel)
	{
		CreditsPanel->Open();
	}
}

void UUSTitleWidget::HandleQuitClicked()
{
	OnQuitRequested.ExecuteIfBound();
}
