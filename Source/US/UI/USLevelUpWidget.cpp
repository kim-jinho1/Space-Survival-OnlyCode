#include "UI/USLevelUpWidget.h"
#include "UI/USPipRow.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Datas/USBuildData.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "Sound/SoundBase.h"

UUSLevelUpWidget::UUSLevelUpWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BuildCompleteFormat = NSLOCTEXT("USLevelUpWidget", "BuildCompleteFormat", "{0} 완성! {1}");
	RerollFormat = NSLOCTEXT("USLevelUpWidget", "RerollFormat", "다시 뽑기  R · 남은 {0}");
}

void UUSLevelUpWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 1·2·3 키를 받으려면 포커스를 받을 수 있어야 한다 (포커스는 컨트롤러가 창을 띄운 뒤 준다)
	SetIsFocusable(true);

	// BindWidget이라 WBP에 없으면 컴파일 단계에서 막히지만, 다시 열릴 때 중복 바인딩되지 않게 AddUnique
	if (Card0Button)
	{
		Card0Button->OnClicked.AddUniqueDynamic(this, &UUSLevelUpWidget::HandleCard0Clicked);
	}
	if (Card1Button)
	{
		Card1Button->OnClicked.AddUniqueDynamic(this, &UUSLevelUpWidget::HandleCard1Clicked);
	}
	if (Card2Button)
	{
		Card2Button->OnClicked.AddUniqueDynamic(this, &UUSLevelUpWidget::HandleCard2Clicked);
	}
	if (RerollButton)
	{
		RerollButton->OnClicked.AddUniqueDynamic(this, &UUSLevelUpWidget::HandleRerollClicked);
	}

	// 게임이 멈춘 상태에서 뜨므로 UI 사운드(일시정지 무시)로 재생한다
	if (OpenSound)
	{
		PlaySound(OpenSound);
	}
}

void UUSLevelUpWidget::SetupCards(const TArray<FUSLevelUpCard>& InOptions)
{
	CardOptions = InOptions;
	bSelected = false;

	FillCard({ Card0Button, Card0Title, Card0Desc, Card0Detail, Card0Icon, Card0Tag, Card0Pips, Card0Band, Card0BandText, Card0BandIcon }, 0);
	FillCard({ Card1Button, Card1Title, Card1Desc, Card1Detail, Card1Icon, Card1Tag, Card1Pips, Card1Band, Card1BandText, Card1BandIcon }, 1);
	FillCard({ Card2Button, Card2Title, Card2Desc, Card2Detail, Card2Icon, Card2Tag, Card2Pips, Card2Band, Card2BandText, Card2BandIcon }, 2);
}

void UUSLevelUpWidget::SetupBuildProgress(const TArray<FUSBuildProgress>& Progress)
{
	FillBuildEntry(Build0Name, Build0Pips, Progress.IsValidIndex(0) ? &Progress[0] : nullptr);
	FillBuildEntry(Build1Name, Build1Pips, Progress.IsValidIndex(1) ? &Progress[1] : nullptr);
	FillBuildEntry(Build2Name, Build2Pips, Progress.IsValidIndex(2) ? &Progress[2] : nullptr);
}

void UUSLevelUpWidget::FillBuildEntry(UTextBlock* Name, UPanelWidget* Pips, const FUSBuildProgress* Progress)
{
	const UUSBuildData* Build = Progress ? Progress->Build.Get() : nullptr;

	if (Name)
	{
		if (Build)
		{
			Name->SetText(Progress->bComplete ? FText::Format(BuildCompleteFormat, Build->GetDisplayName(), Build->GetBonusText()) : Build->GetDisplayName());
			Name->SetColorAndOpacity(FSlateColor(Build->GetColor()));
			Name->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Name->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (Pips && WidgetTree)
	{
		// 진행 줄은 네모 칸, 카드 단계 점은 원 — 같은 함수, 모양만 다르게
		USPipRow::FStyle Style;
		Style.Size = BuildPipSize;
		Style.CornerRadius = 2.0f;
		Style.Spacing = PipSpacing;
		if (Build)
		{
			Style.FilledColor = Build->GetColor();
		}
		USPipRow::Refresh(*WidgetTree, *Pips, Build ? Progress->Learned : 0, Build ? Progress->Total : 0, Style);
	}
}

FReply UUSLevelUpWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	int32 Index = INDEX_NONE;
	if (Key == EKeys::One || Key == EKeys::NumPadOne)
	{
		Index = 0;
	}
	else if (Key == EKeys::Two || Key == EKeys::NumPadTwo)
	{
		Index = 1;
	}
	else if (Key == EKeys::Three || Key == EKeys::NumPadThree)
	{
		Index = 2;
	}

	if (Index != INDEX_NONE)
	{
		SelectCard(Index);
		return FReply::Handled();
	}

	if (Key == EKeys::R && RerollsLeft > 0)
	{
		RequestReroll();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UUSLevelUpWidget::FillCard(const FCardWidgets& Widgets, int32 Index)
{
	UButton* Button = Widgets.Button;
	UTextBlock* Title = Widgets.Title;
	UTextBlock* Desc = Widgets.Desc;
	UTextBlock* Detail = Widgets.Detail;
	UImage* Icon = Widgets.Icon;
	UTextBlock* Tag = Widgets.Tag;
	UPanelWidget* Pips = Widgets.Pips;

	if (Button == nullptr)
	{
		return;
	}

	if (!CardOptions.IsValidIndex(Index))
	{
		Button->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	Button->SetVisibility(ESlateVisibility::Visible);
	const FUSLevelUpCard& Option = CardOptions[Index];

	// 칩이 따로 있으면 제목 칸엔 이름만 (기체 이름이 두 번 보이지 않게)
	if (Tag)
	{
		// 보급·빌드 없는 능력은 띠가 이름을 대신하므로 칩이 비어 있다
		Tag->SetText(Option.Tag);
		Tag->SetVisibility(Option.Tag.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (Widgets.Band)
	{
		Widgets.Band->SetBrushColor(Option.BandColor);
	}
	if (Widgets.BandText)
	{
		Widgets.BandText->SetText(Option.BandText);
	}
	if (Widgets.BandIcon)
	{
		if (Option.BandIcon)
		{
			Widgets.BandIcon->SetBrushFromTexture(Option.BandIcon);
			Widgets.BandIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Widgets.BandIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (Title)
	{
		Title->SetText(Tag ? Option.Name : Option.Title);
	}

	if (Pips && WidgetTree)
	{
		USPipRow::FStyle Style;
		Style.Size = PipSize;
		Style.Spacing = PipSpacing;
		Style.FilledColor = Option.AccentColor;
		USPipRow::Refresh(*WidgetTree, *Pips, Option.NextLevel, Option.MaxLevel, Style);
	}

	if (Desc)
	{
		Desc->SetText(Option.Description);
	}

	if (Detail)
	{
		Detail->SetText(Option.Detail);
		Detail->SetVisibility(Option.Detail.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (Icon)
	{
		// 아이콘은 능력 애셋(보급 카드는 업그레이드 컴포넌트)이 정한다. 없으면 숨긴다
		if (Option.Icon)
		{
			Icon->SetBrushFromTexture(Option.Icon);
			Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Icon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UUSLevelUpWidget::SelectCard(int32 Index)
{
	if (bSelected || !CardOptions.IsValidIndex(Index))
	{
		return;
	}

	bSelected = true;

	if (SelectSound)
	{
		PlaySound(SelectSound);
	}

	OnCardSelected.ExecuteIfBound(CardOptions[Index]);
}

void UUSLevelUpWidget::SetRerollsLeft(int32 Count)
{
	RerollsLeft = FMath::Max(0, Count);

	if (RerollButton)
	{
		RerollButton->SetVisibility(RerollsLeft > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (RerollText)
	{
		RerollText->SetText(FText::Format(RerollFormat, FText::AsNumber(RerollsLeft)));
	}
}

void UUSLevelUpWidget::RequestReroll()
{
	// 고른 뒤(창이 닫히는 중)에는 다시 뽑지 않는다
	if (bSelected || RerollsLeft <= 0)
	{
		return;
	}

	if (USoundBase* Sound = RerollSound ? RerollSound.Get() : SelectSound.Get())
	{
		PlaySound(Sound);
	}

	OnRerollRequested.ExecuteIfBound();
}

void UUSLevelUpWidget::HandleRerollClicked()
{
	RequestReroll();
}

void UUSLevelUpWidget::HandleCard0Clicked()
{
	SelectCard(0);
}

void UUSLevelUpWidget::HandleCard1Clicked()
{
	SelectCard(1);
}

void UUSLevelUpWidget::HandleCard2Clicked()
{
	SelectCard(2);
}
