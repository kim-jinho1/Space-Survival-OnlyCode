#include "UI/USHangarOptionWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UUSHangarOptionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (OptionButton)
	{
		OptionButton->OnClicked.AddUniqueDynamic(this, &UUSHangarOptionWidget::HandleButtonClicked);
	}
}

void UUSHangarOptionWidget::SetupOption(int32 InIndex, const FText& Label, bool bEnabled, bool bHighlighted)
{
	SetupOption(InIndex, Label, FText::GetEmpty(), nullptr, bEnabled, bHighlighted);
}

void UUSHangarOptionWidget::SetupOption(int32 InIndex, const FText& Label, const FText& SubLabel, UTexture2D* Icon,
	bool bEnabled, bool bHighlighted)
{
	OptionIndex = InIndex;

	if (OptionIcon)
	{
		// 같은 줄 위젯이 아이콘 있는 기종과 없는 "+" 칸을 오가므로 매번 보이기/숨기기를 다시 정한다
		if (Icon)
		{
			OptionIcon->SetBrushFromTexture(Icon);
			OptionIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			OptionIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (SubText)
	{
		SubText->SetText(SubLabel);
		SubText->SetVisibility(SubLabel.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	// 아이콘 칸(편성 칸)은 그림이 있으면 이름을 비워서 넘긴다. 빈 글자는 숨겨 그림을 가리지 않게 한다
	if (OptionText)
	{
		OptionText->SetText(Label);
		OptionText->SetVisibility(Label.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	// 잠긴 선택지도 버튼은 켜 둔다: 누르면 만든 쪽이 "해금 필요" 같은 이유를 알려 줄 수 있게 (꺼 두면 아무 반응이 없다)
	if (OptionButton)
	{
		OptionButton->SetBackgroundColor(!bEnabled ? DisabledColor : (bHighlighted ? HighlightColor : NormalColor));
	}
}

void UUSHangarOptionWidget::SetCornerText(const FText& Text)
{
	if (CornerText)
	{
		CornerText->SetText(Text);
		CornerText->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void UUSHangarOptionWidget::SetButtonColor(const FLinearColor& Color)
{
	if (OptionButton)
	{
		OptionButton->SetBackgroundColor(Color);
	}
}

void UUSHangarOptionWidget::SetKeyFocused(bool bFocused)
{
	// 버튼 배경색(상태 색)이 테두리까지 물들이므로 강조는 따로 겹친 위젯으로 한다
	if (KeyFocusFrame)
	{
		KeyFocusFrame->SetVisibility(bFocused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UUSHangarOptionWidget::HandleButtonClicked()
{
	OnOptionClicked.ExecuteIfBound(OptionIndex);
}
