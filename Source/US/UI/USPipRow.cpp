#include "UI/USPipRow.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"

void USPipRow::Refresh(UWidgetTree& Tree, UPanelWidget& Row, int32 Level, int32 MaxLevel, const FStyle& Style)
{
	Row.SetVisibility(MaxLevel > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if (Row.GetChildrenCount() != MaxLevel)
	{
		Row.ClearChildren();

		for (int32 Index = 0; Index < MaxLevel; ++Index)
		{
			UImage* Pip = Tree.ConstructWidget<UImage>(UImage::StaticClass());
			if (UHorizontalBoxSlot* PipSlot = Cast<UHorizontalBoxSlot>(Row.AddChild(Pip)))
			{
				PipSlot->SetPadding(FMargin(0.0f, 0.0f, Style.Spacing, 0.0f));
				PipSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}

	// 색을 브러시에 직접 넣는다: 테두리 색은 SetColorAndOpacity(전체 곱하기)로는 채움과 따로 칠할 수 없어서.
	// 브러시는 값 타입이라 SetBrush가 복사해 가므로 지역 변수로 충분하다
	const FVector2f PipSize(Style.Size, Style.Height > 0.0f ? Style.Height : Style.Size);
	const float Radius = Style.CornerRadius >= 0.0f ? Style.CornerRadius : FMath::Min(PipSize.X, PipSize.Y) * 0.5f;
	const FSlateRoundedBoxBrush FilledBrush(Style.FilledColor, Radius, PipSize);
	const FSlateRoundedBoxBrush EmptyBrush(Style.EmptyColor, Radius, PipSize);
	const bool bOutlineNext = Style.NextOutlineColor.A > 0.0f;
	const FSlateRoundedBoxBrush NextBrush(Style.EmptyColor, Radius, Style.NextOutlineColor, Style.OutlineWidth, PipSize);

	for (int32 Index = 0; Index < Row.GetChildrenCount(); ++Index)
	{
		if (UImage* Pip = Cast<UImage>(Row.GetChildAt(Index)))
		{
			const bool bNext = bOutlineNext && Index == Level;
			Pip->SetBrush(Index < Level ? FilledBrush : (bNext ? NextBrush : EmptyBrush));
			Pip->SetColorAndOpacity(FLinearColor::White);
		}
	}
}
