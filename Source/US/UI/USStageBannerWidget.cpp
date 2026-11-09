#include "UI/USStageBannerWidget.h"
#include "US.h"
#include "Core/USLoadingScreenSettings.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
	/** 흐림 갱신 간격(초) */
	constexpr float FadeStepInterval = 0.03f;
}

UUSStageBannerWidget::UUSStageBannerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	StageClearText = NSLOCTEXT("USStageBannerWidget", "StageClearText", "클리어");
	StageClearSubText = NSLOCTEXT("USStageBannerWidget", "StageClearSubText", "보상 회수 중");
	StageLabelFormat = NSLOCTEXT("USStageBannerWidget", "StageLabelFormat", "STAGE {0}");
	WallSuffix = NSLOCTEXT("USStageBannerWidget", "WallSuffix", " · 위험");

	// 목업 색 (#5fd4ff 하늘, #e6f6ff 글자, #9fb3c4 보조, #ff8a80 위험)
	AccentColor = FLinearColor::FromSRGBColor(FColor(95, 212, 255));
	TextColor = FLinearColor::FromSRGBColor(FColor(230, 246, 255));
	SubTextColor = FLinearColor::FromSRGBColor(FColor(159, 179, 196));
	WallColor = FLinearColor::FromSRGBColor(FColor(255, 138, 128));
	FlashColor = FLinearColor::FromSRGBColor(FColor(230, 246, 255));
}

TSharedRef<SWidget> UUSStageBannerWidget::RebuildWidget()
{
	// WBP가 없는 위젯이라 트리를 여기서 만든다 (Initialize가 빈 WidgetTree를 만들어 둔다)
	if (WidgetTree && WidgetTree->RootWidget == nullptr)
	{
		BuildTree();
	}
	return Super::RebuildWidget();
}

void UUSStageBannerWidget::NativeDestruct()
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HoldTimerHandle);
		World->GetTimerManager().ClearTimer(FadeTimerHandle);
		World->GetTimerManager().ClearTimer(FlashTimerHandle);
	}

	Super::NativeDestruct();
}

void UUSStageBannerWidget::BuildTree()
{
	Font = GetDefault<UUSLoadingScreenSettings>()->Font.LoadSynchronous();
	if (Font == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSStageBannerWidget: loading screen font is not set. Korean text will not render."));
	}
	auto MakeFont = [this](int32 Size, int32 LetterSpacing = 0)
	{
		FSlateFontInfo FontInfo = Font ? FSlateFontInfo(Font, Size) : FCoreStyle::GetDefaultFontStyle("Regular", Size);
		FontInfo.LetterSpacing = LetterSpacing;
		return FontInfo;
	};
	auto MakeText = [this, &MakeFont](int32 Size, const FLinearColor& Color, int32 LetterSpacing = 0)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetFont(MakeFont(Size, LetterSpacing));
		Text->SetColorAndOpacity(FSlateColor(Color));
		return Text;
	};
	// 선은 SizeBox로 크기를 못 박는다. 빈 브러시 UImage는 SetDesiredSizeOverride를 줘도 기본 32px 두께로 그려진 것으로 보인다 (2px 선이 굵은 막대로 나옴)
	auto MakeLine = [this](const FVector2D& Size, const FLinearColor& Color)
	{
		USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Box->SetWidthOverride(Size.X);
		Box->SetHeightOverride(Size.Y);
		UImage* Line = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Line->SetColorAndOpacity(Color);
		Box->SetContent(Line);
		return Box;
	};
	auto AddCentered = [](UCanvasPanel* Canvas, UWidget* Child, float HeightRatio)
	{
		if (UCanvasPanelSlot* ChildSlot = Canvas->AddChildToCanvas(Child))
		{
			ChildSlot->SetAnchors(FAnchors(0.5f, HeightRatio));
			ChildSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			ChildSlot->SetPosition(FVector2D::ZeroVector);
			ChildSlot->SetAutoSize(true);
		}
	};

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	Root->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = Root;

	// 클리어: 띠 없이 "STAGE N" / "클리어" / 얇은 선 / "보상 회수 중" (뒤 전투 화면을 가리지 않는다)
	UVerticalBox* Clear = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ClearLabelText = MakeText(TitleLabelFontSize, AccentColor, 400);
	Clear->AddChildToVerticalBox(ClearLabelText)->SetHorizontalAlignment(HAlign_Center);
	ClearText = MakeText(ClearFontSize, TextColor, 100);
	Clear->AddChildToVerticalBox(ClearText)->SetHorizontalAlignment(HAlign_Center);
	UVerticalBoxSlot* ClearLineSlot = Clear->AddChildToVerticalBox(MakeLine(FVector2D(ClearLineWidth, 1.0f), FLinearColor(AccentColor.R, AccentColor.G, AccentColor.B, 0.7f)));
	ClearLineSlot->SetHorizontalAlignment(HAlign_Center);
	ClearLineSlot->SetPadding(FMargin(0.0f, 6.0f));
	UTextBlock* ClearSub = MakeText(TitleLabelFontSize, SubTextColor);
	ClearSub->SetText(StageClearSubText);
	Clear->AddChildToVerticalBox(ClearSub)->SetHorizontalAlignment(HAlign_Center);
	AddCentered(Root, Clear, ClearHeightRatio);
	ClearBand = Clear;

	// 진입 자막: "── 이름 ──" / "STAGE N · 위험"
	UVerticalBox* Title = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	UHorizontalBox* NameRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	NameRow->AddChildToHorizontalBox(MakeLine(FVector2D(TitleLineWidth, 1.0f), AccentColor))->SetVerticalAlignment(VAlign_Center);
	TitleNameText = MakeText(TitleNameFontSize, TextColor);
	NameRow->AddChildToHorizontalBox(TitleNameText)->SetPadding(FMargin(16.0f, 0.0f));
	NameRow->AddChildToHorizontalBox(MakeLine(FVector2D(TitleLineWidth, 1.0f), AccentColor))->SetVerticalAlignment(VAlign_Center);
	Title->AddChildToVerticalBox(NameRow)->SetHorizontalAlignment(HAlign_Center);
	UHorizontalBox* LabelRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	TitleLabelText = MakeText(TitleLabelFontSize, SubTextColor, 300);
	LabelRow->AddChildToHorizontalBox(TitleLabelText);
	TitleWallText = MakeText(TitleLabelFontSize, WallColor);
	LabelRow->AddChildToHorizontalBox(TitleWallText);
	Title->AddChildToVerticalBox(LabelRow)->SetHorizontalAlignment(HAlign_Center);
	AddCentered(Root, Title, TitleHeightRatio);
	TitleBox = Title;

	// 섬광: 화면 전체를 덮는 흰 판 (띠·자막보다 위)
	FlashImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	FlashImage->SetColorAndOpacity(FlashColor);
	FlashImage->SetRenderOpacity(0.0f);
	FlashImage->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* FlashSlot = Root->AddChildToCanvas(FlashImage))
	{
		FlashSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		FlashSlot->SetOffsets(FMargin(0.0f));
	}

	ClearBand->SetVisibility(ESlateVisibility::Collapsed);
	TitleBox->SetVisibility(ESlateVisibility::Collapsed);
}

void UUSStageBannerWidget::ShowFlash(float PeakOpacity, float InSeconds, float OutSeconds)
{
	UWorld* World = GetWorld();
	if (FlashImage == nullptr || World == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSStageBannerWidget: flash skipped (not built yet)."));
		return;
	}

	FlashPeak = FMath::Clamp(PeakOpacity, 0.0f, 1.0f);
	FlashIn = FMath::Max(0.0f, InSeconds);
	FlashOut = OutSeconds;
	FlashStartTime = World->GetTimeSeconds();
	FlashImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	StepFlash();
	World->GetTimerManager().SetTimer(FlashTimerHandle, this, &UUSStageBannerWidget::StepFlash, FadeStepInterval, true);
}

void UUSStageBannerWidget::StepFlash()
{
	UWorld* World = GetWorld();
	if (World == nullptr || FlashImage == nullptr)
	{
		return;
	}

	const float Elapsed = World->GetTimeSeconds() - FlashStartTime;
	if (Elapsed < FlashIn)
	{
		FlashImage->SetRenderOpacity(FlashPeak * Elapsed / FlashIn);
		return;
	}

	// 밝은 채 남기기 (결산 창이 뜨며 게임이 멈추면 그 위로 창이 덮는다)
	if (FlashOut < 0.0f)
	{
		FlashImage->SetRenderOpacity(FlashPeak);
		World->GetTimerManager().ClearTimer(FlashTimerHandle);
		return;
	}

	const float Alpha = FlashOut > 0.0f ? FMath::Clamp((Elapsed - FlashIn) / FlashOut, 0.0f, 1.0f) : 1.0f;
	FlashImage->SetRenderOpacity(FlashPeak * (1.0f - Alpha));
	if (Alpha >= 1.0f)
	{
		World->GetTimerManager().ClearTimer(FlashTimerHandle);
		FlashImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UUSStageBannerWidget::ShowStageClear(int32 StageNumber, float HoldSeconds)
{
	if (ClearLabelText)
	{
		ClearLabelText->SetText(FText::Format(StageLabelFormat, FText::AsNumber(StageNumber)));
	}
	if (ClearText)
	{
		ClearText->SetText(StageClearText);
	}
	ShowElement(ClearBand, HoldSeconds);
}

void UUSStageBannerWidget::ShowStageTitle(int32 StageNumber, const FText& StageName, bool bWall, float HoldSeconds)
{
	if (TitleLabelText)
	{
		TitleLabelText->SetText(FText::Format(StageLabelFormat, FText::AsNumber(StageNumber)));
	}
	if (TitleNameText)
	{
		TitleNameText->SetText(StageName);
	}
	if (TitleWallText)
	{
		TitleWallText->SetText(WallSuffix);
		TitleWallText->SetVisibility(bWall ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	ShowElement(TitleBox, HoldSeconds);
}

void UUSStageBannerWidget::ShowElement(UWidget* Element, float HoldSeconds)
{
	UWorld* World = GetWorld();
	if (Element == nullptr || World == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSStageBannerWidget: not built yet (add to viewport before showing)."));
		return;
	}

	for (UWidget* Other : { ClearBand.Get(), TitleBox.Get() })
	{
		if (Other && Other != Element)
		{
			Other->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	ActiveElement = Element;
	Element->SetRenderOpacity(1.0f);
	Element->SetVisibility(ESlateVisibility::HitTestInvisible);

	World->GetTimerManager().ClearTimer(FadeTimerHandle);
	World->GetTimerManager().SetTimer(HoldTimerHandle, this, &UUSStageBannerWidget::BeginFade, FMath::Max(HoldSeconds, KINDA_SMALL_NUMBER), false);
}

void UUSStageBannerWidget::BeginFade()
{
	if (UWorld* World = GetWorld())
	{
		FadeStartTime = World->GetTimeSeconds();
		World->GetTimerManager().SetTimer(FadeTimerHandle, this, &UUSStageBannerWidget::StepFade, FadeStepInterval, true);
	}
}

void UUSStageBannerWidget::StepFade()
{
	const UWorld* World = GetWorld();
	if (World == nullptr || ActiveElement == nullptr)
	{
		return;
	}

	const float Alpha = FadeSeconds > 0.0f ? FMath::Clamp((World->GetTimeSeconds() - FadeStartTime) / FadeSeconds, 0.0f, 1.0f) : 1.0f;
	ActiveElement->SetRenderOpacity(1.0f - Alpha);
	if (Alpha >= 1.0f)
	{
		World->GetTimerManager().ClearTimer(FadeTimerHandle);
		ActiveElement->SetVisibility(ESlateVisibility::Collapsed);
		ActiveElement = nullptr;
	}
}
