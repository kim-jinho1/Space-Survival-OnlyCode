#include "UI/USKeyMenu.h"
#include "US.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"

void FUSKeyMenu::Initialize(const TArray<UButton*>& InButtons, const FSlateColor& InSelectedLabelColor, const FSlateColor& InIdleLabelColor,
	const TArray<UButton*>& OwnIdleStyleButtons)
{
	Buttons.Reset();
	OwnIdleStyles.Reset();
	OwnIdleLabelColors.Reset();
	for (UButton* Button : InButtons)
	{
		if (Button == nullptr)
		{
			continue;
		}

		// Select(0)이 덮어쓰기 전에 WBP 모양을 읽어 둔다
		if (OwnIdleStyleButtons.Contains(Button))
		{
			OwnIdleStyles.Add(Buttons.Num(), Button->GetStyle());
			if (const UTextBlock* Label = Cast<UTextBlock>(Button->GetContent()))
			{
				OwnIdleLabelColors.Add(Buttons.Num(), Label->GetColorAndOpacity());
			}
		}
		Buttons.Add(Button);
	}

	SelectedLabelColor = InSelectedLabelColor;
	IdleLabelColor = InIdleLabelColor;

	if (Buttons.Num() >= 2)
	{
		SelectedStyle = Buttons[0]->GetStyle();
		IdleStyle = Buttons[1]->GetStyle();
	}
	else
	{
		UE_LOG(LogUS, Warning, TEXT("FUSKeyMenu: fewer than 2 buttons. Menu highlight uses default styles."));
	}

	Select(0);
}

FUSKeyMenu::EResult FUSKeyMenu::HandleKey(const FKey& Key)
{
	const int32 Count = Buttons.Num();
	if (Count == 0)
	{
		return EResult::Unhandled;
	}

	if (Key == EKeys::Up || Key == EKeys::W || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
	{
		Select((SelectedIndex - 1 + Count) % Count);
		return EResult::Moved;
	}
	if (Key == EKeys::Down || Key == EKeys::S || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
	{
		Select((SelectedIndex + 1) % Count);
		return EResult::Moved;
	}
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		return EResult::Activate;
	}

	return EResult::Unhandled;
}

void FUSKeyMenu::Select(int32 Index)
{
	if (!Buttons.IsValidIndex(Index))
	{
		return;
	}

	SelectedIndex = Index;
	Refresh();
}

void FUSKeyMenu::Refresh() const
{
	for (int32 Index = 0; Index < Buttons.Num(); ++Index)
	{
		UButton* Button = Buttons[Index].Get();
		if (Button == nullptr)
		{
			continue;
		}

		const bool bSelected = Index == SelectedIndex;
		const FButtonStyle* OwnStyle = OwnIdleStyles.Find(Index);
		Button->SetStyle(bSelected ? SelectedStyle : (OwnStyle ? *OwnStyle : IdleStyle));

		// 글자는 버튼의 자식 TextBlock이다
		if (UTextBlock* Label = Cast<UTextBlock>(Button->GetContent()))
		{
			const FSlateColor* OwnLabelColor = OwnIdleLabelColors.Find(Index);
			Label->SetColorAndOpacity(bSelected ? SelectedLabelColor : (OwnLabelColor ? *OwnLabelColor : IdleLabelColor));
		}
	}
}
