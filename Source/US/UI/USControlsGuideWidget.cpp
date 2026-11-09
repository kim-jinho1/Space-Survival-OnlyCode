#include "UI/USControlsGuideWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
	FUSKeyGuideEntry MakeEntry(std::initializer_list<FText> Keys, const FText& Action)
	{
		FUSKeyGuideEntry Entry;
		Entry.Keys = Keys;
		Entry.Action = Action;
		return Entry;
	}
}

UUSControlsGuideWidget::UUSControlsGuideWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 실제 키는 IMC_Default(이동·사격·가속·상호작용·줌)와 AUSPlayerController(Esc/P), 레벨업 카드(1/2/3)
	Entries.Add(MakeEntry({ INVTEXT("WASD"), NSLOCTEXT("USControlsGuideWidget", "ArrowKeys", "방향키") },
		NSLOCTEXT("USControlsGuideWidget", "Move", "이동")));
	// 조준은 가장 가까운 적을 자동으로 노린다. 마우스는 클릭 표적 지정에만 쓰인다
	Entries.Add(MakeEntry({ NSLOCTEXT("USControlsGuideWidget", "LeftClickKey", "좌클릭") },
		NSLOCTEXT("USControlsGuideWidget", "Target", "표적 지정 (사격은 자동)")));
	Entries.Add(MakeEntry({ INVTEXT("Space"), INVTEXT("Shift") },
		NSLOCTEXT("USControlsGuideWidget", "Boost", "가속")));
	Entries.Add(MakeEntry({ INVTEXT("Q") },
		NSLOCTEXT("USControlsGuideWidget", "Supply", "보급선 상점 (가까이에서)")));
	Entries.Add(MakeEntry({ INVTEXT("1"), INVTEXT("2"), INVTEXT("3") },
		NSLOCTEXT("USControlsGuideWidget", "Cards", "레벨업 카드 고르기")));
	Entries.Add(MakeEntry({ NSLOCTEXT("USControlsGuideWidget", "WheelKey", "휠") },
		NSLOCTEXT("USControlsGuideWidget", "Zoom", "확대·축소")));
	Entries.Add(MakeEntry({ INVTEXT("Esc") },
		NSLOCTEXT("USControlsGuideWidget", "Pause", "일시정지 · 설정")));

	HintLabel = NSLOCTEXT("USControlsGuideWidget", "HintLabel", "아무 키나 눌러 시작하세요");
}

void UUSControlsGuideWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 아무 키를 받으려면 포커스를 받을 수 있어야 한다 (포커스는 컨트롤러가 띄운 뒤 준다)
	SetIsFocusable(bDismissible);

	if (HintText)
	{
		HintText->SetText(HintLabel);
		HintText->SetVisibility(bDismissible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	BuildKeyList();
}

void UUSControlsGuideWidget::BuildKeyList()
{
	if (KeyList == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	KeyList->ClearChildren();

	// 글꼴을 따로 안 정했으면 WBP에서 Pretendard로 칠해 둔 안내 글 글꼴을 빌린다 (엔진 기본 글꼴엔 한글이 없다)
	FSlateFontInfo ResolvedKeyFont = KeyFont;
	FSlateFontInfo ResolvedActionFont = ActionFont;
	if (HintText)
	{
		if (!ResolvedKeyFont.HasValidFont())
		{
			ResolvedKeyFont = HintText->GetFont();
		}
		if (!ResolvedActionFont.HasValidFont())
		{
			ResolvedActionFont = HintText->GetFont();
		}
	}

	FSlateBrush KeyBrush;
	KeyBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	KeyBrush.TintColor = KeyFillColor;
	KeyBrush.OutlineSettings = FSlateBrushOutlineSettings(FVector4(4.0f, 4.0f, 4.0f, 4.0f), FSlateColor(KeyOutlineColor), 1.0f);
	KeyBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;

	// 키 칸 너비는 다 만든 뒤 가장 긴 키 줄에 맞춘다 (글꼴 크기가 다른 카드·일시정지 목록 둘 다 숫자 없이 정렬)
	TArray<TPair<USizeBox*, UHorizontalBox*>> KeyColumns;

	for (const FUSKeyGuideEntry& Entry : Entries)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

		USizeBox* KeyColumn = WidgetTree->ConstructWidget<USizeBox>();
		UHorizontalBox* KeyBox = WidgetTree->ConstructWidget<UHorizontalBox>();
		KeyColumns.Emplace(KeyColumn, KeyBox);
		KeyColumn->AddChild(KeyBox);

		for (const FText& Key : Entry.Keys)
		{
			UBorder* KeyCap = WidgetTree->ConstructWidget<UBorder>();
			KeyCap->SetBrush(KeyBrush);
			KeyCap->SetPadding(FMargin(8.0f, 2.0f));
			KeyCap->SetHorizontalAlignment(HAlign_Center);

			UTextBlock* KeyText = WidgetTree->ConstructWidget<UTextBlock>();
			KeyText->SetText(Key);
			KeyText->SetFont(ResolvedKeyFont);
			KeyText->SetColorAndOpacity(KeyTextColor);
			KeyCap->SetContent(KeyText);

			if (UHorizontalBoxSlot* CapSlot = KeyBox->AddChildToHorizontalBox(KeyCap))
			{
				CapSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
				CapSlot->SetVerticalAlignment(VAlign_Center);
			}
		}

		if (UHorizontalBoxSlot* KeySlot = Row->AddChildToHorizontalBox(KeyColumn))
		{
			KeySlot->SetPadding(FMargin(0.0f, 0.0f, KeyActionGap, 0.0f));
			KeySlot->SetVerticalAlignment(VAlign_Center);
		}

		UTextBlock* ActionText = WidgetTree->ConstructWidget<UTextBlock>();
		ActionText->SetText(Entry.Action);
		ActionText->SetFont(ResolvedActionFont);
		ActionText->SetColorAndOpacity(ActionTextColor);
		if (UHorizontalBoxSlot* ActionSlot = Row->AddChildToHorizontalBox(ActionText))
		{
			ActionSlot->SetVerticalAlignment(VAlign_Center);
		}

		if (UVerticalBoxSlot* RowSlot = KeyList->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.0f, 4.0f));
		}
	}

	// 줄마다 키 그림 너비를 재서 가장 넓은 값으로 모든 키 칸을 맞춘다 (표의 "열 너비 자동 맞춤")
	float MaxKeyWidth = 0.0f;
	for (const TPair<USizeBox*, UHorizontalBox*>& Column : KeyColumns)
	{
		Column.Value->TakeWidget();
		Column.Value->ForceLayoutPrepass();
		MaxKeyWidth = FMath::Max(MaxKeyWidth, Column.Value->GetDesiredSize().X);
	}
	for (const TPair<USizeBox*, UHorizontalBox*>& Column : KeyColumns)
	{
		Column.Key->SetMinDesiredWidth(MaxKeyWidth);
	}
}

FReply UUSControlsGuideWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!bDismissible)
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}

	Dismiss();
	return FReply::Handled();
}

FReply UUSControlsGuideWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bDismissible)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	Dismiss();
	return FReply::Handled();
}

void UUSControlsGuideWidget::Dismiss()
{
	if (bDismissed)
	{
		return;
	}

	bDismissed = true;
	OnDismissed.ExecuteIfBound();
}
