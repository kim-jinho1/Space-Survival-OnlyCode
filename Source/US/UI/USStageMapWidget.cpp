#include "UI/USStageMapWidget.h"
#include "US.h"
#include "Datas/USEnemyTypeData.h"
#include "Datas/USStageData.h"
#include "Datas/USZoneData.h"
#include "Progression/USMetaProgressionSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"

namespace
{
	/** 캔버스 층 순서: 바탕 < 그림자·벽 테두리 < 지역 < 고른 지역 < 글자 */
	constexpr int32 BackgroundZOrder = 0;
	constexpr int32 ShadowZOrder = 1;
	constexpr int32 WallRimZOrder = 1;
	constexpr int32 RegionZOrder = 2;
	constexpr int32 SelectedRegionZOrder = 3;
	constexpr int32 LabelZOrder = 4;

	/** 지역 상태 (색·글자) */
	enum class EStageState : uint8
	{
		Cleared,
		Open,
		Locked
	};
}

UUSStageMapWidget::UUSStageMapWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RegionLabelFormat = NSLOCTEXT("USStageMapWidget", "RegionLabelFormat", "{0}");
	ClearedLabelFormat = NSLOCTEXT("USStageMapWidget", "ClearedLabelFormat", "{0}\nCLEAR");
	BossLabel = NSLOCTEXT("USStageMapWidget", "BossLabel", "보스");
	StageNameFormat = NSLOCTEXT("USStageMapWidget", "StageNameFormat", "스테이지 {0} · {1}");
	StageInfoFormat = NSLOCTEXT("USStageMapWidget", "StageInfoFormat", "{0}\n{1}");
	SurviveObjective = NSLOCTEXT("USStageMapWidget", "SurviveObjective", "1:30 버티기");
	BossObjective = NSLOCTEXT("USStageMapWidget", "BossObjective", "1:30 뒤 보스 격파 → 행성 클리어");
	ClearedSuffix = NSLOCTEXT("USStageMapWidget", "ClearedSuffix", "클리어함 · 다시 할 수 있습니다");
	LockedText = NSLOCTEXT("USStageMapWidget", "LockedText", "깬 지역과 맞닿은 곳만 갈 수 있습니다");
	BossLockedText = NSLOCTEXT("USStageMapWidget", "BossLockedText", "나머지 지역을 모두 깨면 열립니다");
	ProgressFormat = NSLOCTEXT("USStageMapWidget", "ProgressFormat", "깬 지역 {0} / {1}");
	WallInfoFormat = NSLOCTEXT("USStageMapWidget", "WallInfoFormat", "위험 · 적 체력 ×{0}");
	// 목업 3A 색 (#ff5a4f 테두리, 지형은 어두운 빨강 쪽으로)
	WallRimColor = FLinearColor::FromSRGBColor(FColor(255, 90, 79));
	WallTint = FLinearColor::FromSRGBColor(FColor(120, 30, 26));
	LabelFont.Size = 22;
	LabelFont.OutlineSettings.OutlineSize = 2;
	LabelFont.OutlineSettings.OutlineColor = FLinearColor(0.002f, 0.003f, 0.005f, 1.0f);
}

void UUSStageMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SquadButton)
	{
		SquadButton->OnClicked.AddUniqueDynamic(this, &UUSStageMapWidget::HandleSquadClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &UUSStageMapWidget::HandleBackClicked);
	}

	// Esc를 받으려면 포커스를 받을 수 있어야 한다 (포커스는 ShowZone에서 준다)
	SetIsFocusable(true);
}

UUSMetaProgressionSubsystem* UUSStageMapWidget::GetMeta() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr;
}

void UUSStageMapWidget::ShowZone(const UUSZoneData* InZone, bool bKeepSelection)
{
	Zone = InZone;
	if (Zone == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSStageMapWidget: ShowZone without a zone."));
		return;
	}

	if (BuiltZone != Zone)
	{
		BuildLayers();
		bKeepSelection = false;
	}

	if (ZoneNameText)
	{
		ZoneNameText->SetText(Zone->GetDisplayName());
	}

	// 처음 들어오면 "다음에 할 곳"을 골라 둔다 (바로 편성으로 갈 수 있게)
	const TArray<TObjectPtr<UUSStageData>>& Stages = Zone->GetStages();
	if (!bKeepSelection || !Stages.IsValidIndex(SelectedIndex))
	{
		const UUSMetaProgressionSubsystem* Meta = GetMeta();
		const UUSStageData* Next = Meta ? Meta->FindNextStage(Zone, nullptr) : nullptr;
		SelectedIndex = Next ? Stages.IndexOfByKey(Next) : 0;
	}

	HoveredIndex = INDEX_NONE;
	SelectStage(SelectedIndex);
	SetKeyboardFocus();
}

void UUSStageMapWidget::BuildLayers()
{
	BuiltZone = Zone;
	RegionImages.Reset();
	ShadowImages.Reset();
	WallRimImages.Reset();
	Labels.Reset();
	BackgroundImage = nullptr;

	if (MapLayer == nullptr || WidgetTree == nullptr || Zone == nullptr)
	{
		return;
	}
	MapLayer->ClearChildren();

	auto AddStretched = [this](UWidget* Widget, const FVector2D& Min, const FVector2D& Max, int32 ZOrder)
	{
		if (UCanvasPanelSlot* CanvasSlot = MapLayer->AddChildToCanvas(Widget))
		{
			CanvasSlot->SetAnchors(FAnchors(Min.X, Min.Y, Max.X, Max.Y));
			CanvasSlot->SetOffsets(FMargin(0.0f));
			CanvasSlot->SetZOrder(ZOrder);
		}
	};

	if (Zone->GetStageMapImage())
	{
		BackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		BackgroundImage->SetBrushFromTexture(Zone->GetStageMapImage());
		// 바탕만 클릭을 받는다 (처리는 안 하고 이 위젯의 마우스 함수로 올라온다). 다 HitTestInvisible이면 이동·클릭이 오지 않는다
		BackgroundImage->SetVisibility(ESlateVisibility::Visible);
		AddStretched(BackgroundImage, FVector2D::ZeroVector, FVector2D(1.0f, 1.0f), BackgroundZOrder);
	}
	else
	{
		UE_LOG(LogUS, Warning, TEXT("UUSStageMapWidget: %s has no StageMapImage. The map shows only regions."), *GetNameSafe(Zone));
	}

	// 글꼴을 비워 두면 정보 카드 제목 글꼴을 크기만 바꿔 쓴다 (게임 UI 글꼴 하나로)
	FSlateFontInfo Font = LabelFont;
	if (Font.FontObject == nullptr && StageNameText)
	{
		Font.FontObject = StageNameText->GetFont().FontObject;
		Font.TypefaceFontName = StageNameText->GetFont().TypefaceFontName;
	}

	const TArray<TObjectPtr<UUSStageData>>& Stages = Zone->GetStages();
	RegionImages.SetNum(Stages.Num());
	ShadowImages.SetNum(Stages.Num());
	WallRimImages.SetNum(Stages.Num());
	Labels.SetNum(Stages.Num());
	for (int32 Index = 0; Index < Stages.Num(); ++Index)
	{
		const UUSStageData* Stage = Stages[Index];
		if (Stage == nullptr)
		{
			continue;
		}
		if (Stage->GetRegionImage() == nullptr)
		{
			UE_LOG(LogUS, Warning, TEXT("UUSStageMapWidget: %s has no RegionImage."), *GetNameSafe(Stage));
		}

		// 그림자는 지역과 같은 그림을 검게. 고른 지역이 들려 올라갈 때만 보인다
		UImage* Shadow = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Shadow->SetBrushFromTexture(Stage->GetRegionImage());
		Shadow->SetColorAndOpacity(ShadowColor);
		Shadow->SetVisibility(ESlateVisibility::Hidden);
		AddStretched(Shadow, Stage->GetRegionMin(), Stage->GetRegionMax(), ShadowZOrder);
		ShadowImages[Index] = Shadow;

		// 벽 테두리: 같은 그림을 조금 키워 빨갛게 아래에 깐다 → 지역 둘레로 빨간 띠가 보인다 (모양이 제각각이라 선을 따로 그리지 않는다)
		if (Stage->IsWallStage())
		{
			UImage* Rim = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Rim->SetBrushFromTexture(Stage->GetRegionImage());
			Rim->SetColorAndOpacity(WallRimColor);
			Rim->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			Rim->SetRenderScale(FVector2D(WallRimScale));
			Rim->SetVisibility(ESlateVisibility::HitTestInvisible);
			AddStretched(Rim, Stage->GetRegionMin(), Stage->GetRegionMax(), WallRimZOrder);
			WallRimImages[Index] = Rim;
		}

		// 지역 그림은 사각형이라 클릭은 지역 번호 지도로 가린다 → 그림 자체는 클릭을 받지 않는다
		UImage* Region = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Region->SetBrushFromTexture(Stage->GetRegionImage());
		Region->SetVisibility(ESlateVisibility::HitTestInvisible);
		AddStretched(Region, Stage->GetRegionMin(), Stage->GetRegionMax(), RegionZOrder);
		RegionImages[Index] = Region;

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetFont(Font);
		Label->SetJustification(ETextJustify::Center);
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UCanvasPanelSlot* LabelSlot = MapLayer->AddChildToCanvas(Label))
		{
			const FVector2D& Position = Stage->GetLabelPosition();
			LabelSlot->SetAnchors(FAnchors(Position.X, Position.Y));
			LabelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			LabelSlot->SetAutoSize(true);
			LabelSlot->SetPosition(FVector2D::ZeroVector);
			LabelSlot->SetZOrder(LabelZOrder);
		}
		Labels[Index] = Label;
	}
}

int32 UUSStageMapWidget::FindStageIndexAt(const FVector2D& ScreenPosition) const
{
	if (MapLayer == nullptr || Zone == nullptr)
	{
		return INDEX_NONE;
	}

	const FGeometry& Geometry = MapLayer->GetCachedGeometry();
	const FVector2D Size = Geometry.GetLocalSize();
	if (Size.X <= 0.0f || Size.Y <= 0.0f)
	{
		return INDEX_NONE;
	}

	const FVector2D UV = Geometry.AbsoluteToLocal(ScreenPosition) / Size;
	const UUSStageData* Stage = Zone->FindStageAt(UV);
	return Stage ? Zone->GetStages().IndexOfByKey(Stage) : INDEX_NONE;
}

FReply UUSStageMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 버튼은 클릭을 먼저 먹으므로 여기 오는 건 지도(또는 빈 곳) 클릭이다
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const int32 Index = FindStageIndexAt(InMouseEvent.GetScreenSpacePosition());
		if (Index != INDEX_NONE)
		{
			SelectStage(Index);
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UUSStageMapWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 마우스가 움직일 때만 찾는다 (칸 하나 읽기라 가볍다). 지역이 바뀔 때만 다시 칠한다
	const int32 Index = FindStageIndexAt(InMouseEvent.GetScreenSpacePosition());
	if (Index != HoveredIndex)
	{
		HoveredIndex = Index;
		RefreshRegions();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UUSStageMapWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (HoveredIndex != INDEX_NONE)
	{
		HoveredIndex = INDEX_NONE;
		RefreshRegions();
	}
}

FReply UUSStageMapWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		HandleBackClicked();
		return FReply::Handled();
	}
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
	{
		HandleSquadClicked();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UUSStageMapWidget::SelectStage(int32 Index)
{
	SelectedIndex = Index;
	RefreshRegions();
	RefreshInfo();
}

void UUSStageMapWidget::RefreshRegions()
{
	const UUSMetaProgressionSubsystem* Meta = GetMeta();
	if (Zone == nullptr || Meta == nullptr)
	{
		return;
	}

	const TArray<TObjectPtr<UUSStageData>>& Stages = Zone->GetStages();
	for (int32 Index = 0; Index < Stages.Num(); ++Index)
	{
		const UUSStageData* Stage = Stages[Index];
		UImage* Region = RegionImages.IsValidIndex(Index) ? RegionImages[Index].Get() : nullptr;
		if (Stage == nullptr || Region == nullptr)
		{
			continue;
		}

		const EStageState State = Meta->IsStageCleared(Stage) ? EStageState::Cleared
			: Meta->IsStageOpen(Zone, Stage) ? EStageState::Open : EStageState::Locked;
		const bool bSelected = Index == SelectedIndex;

		// 벽은 깨기 전까지만 붉게 (깬 뒤에는 다른 깬 곳처럼 가라앉는다)
		const bool bWallAhead = Stage->IsWallStage() && State != EStageState::Cleared;
		FLinearColor Color = bWallAhead ? FLinearColor::LerpUsingHSV(Stage->GetRegionColor(), WallTint, WallBlend) : Stage->GetRegionColor();
		switch (State)
		{
		case EStageState::Cleared:
			Color = FLinearColor::LerpUsingHSV(Color, ClearedTint, ClearedBlend);
			break;
		case EStageState::Locked:
			Color *= LockedBrightness;
			break;
		default:
			break;
		}
		if (bSelected)
		{
			Color = SelectedColor;
		}
		else if (Index == HoveredIndex)
		{
			Color *= HoverBrightness;
		}
		Color.A = 1.0f;
		Region->SetColorAndOpacity(Color);

		// 고른 지역만 위로 들고 그림자를 깐다 (참고 그림의 튀어나온 도)
		const FVector2D Lift(0.0f, bSelected ? -LiftOffset : 0.0f);
		Region->SetRenderTranslation(Lift);
		if (UCanvasPanelSlot* RegionSlot = Cast<UCanvasPanelSlot>(Region->Slot))
		{
			RegionSlot->SetZOrder(bSelected ? SelectedRegionZOrder : RegionZOrder);
		}
		if (UImage* Shadow = ShadowImages.IsValidIndex(Index) ? ShadowImages[Index].Get() : nullptr)
		{
			Shadow->SetVisibility(bSelected && LiftOffset > 0.0f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		}
		if (UImage* Rim = WallRimImages.IsValidIndex(Index) ? WallRimImages[Index].Get() : nullptr)
		{
			Rim->SetVisibility(bWallAhead ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
			Rim->SetRenderTranslation(Lift);
			if (UCanvasPanelSlot* RimSlot = Cast<UCanvasPanelSlot>(Rim->Slot))
			{
				// 들린 지역 밑에서도 테두리가 따라 올라오게 고른 지역 바로 아래 층으로
				RimSlot->SetZOrder(bSelected ? SelectedRegionZOrder - 1 : WallRimZOrder);
			}
		}

		if (UTextBlock* Label = Labels.IsValidIndex(Index) ? Labels[Index].Get() : nullptr)
		{
			const FText Number = FText::AsNumber(Stage->GetStageNumber());
			// 보스 칸은 번호 대신 "보스"를 쓰고, 깬 뒤에는 다른 칸처럼 CLEAR를 붙인다
			const FText Name = Stage->IsBossStage() ? BossLabel : Number;
			const FText Text = State == EStageState::Cleared ? FText::Format(ClearedLabelFormat, Name)
				: Stage->IsBossStage() ? BossLabel : FText::Format(RegionLabelFormat, Number);
			const FLinearColor& LabelColor = bSelected ? SelectedLabelColor
				: State == EStageState::Locked ? LockedLabelColor
				: State == EStageState::Cleared ? ClearedLabelColor
				: Stage->IsBossStage() ? BossLabelColor : OpenLabelColor;
			// 글자가 바뀔 때만 다시 맞춘다 (마우스 올림마다 레이아웃을 계산하지 않게)
			if (!Label->GetText().EqualTo(Text))
			{
				Label->SetText(Text);
				FitLabel(Index, Label);
			}
			Label->SetColorAndOpacity(FSlateColor(LabelColor));
			Label->SetRenderTranslation(Lift);
		}
	}
}

void UUSStageMapWidget::FitLabel(int32 StageIndex, UTextBlock* Label) const
{
	const UUSStageData* Stage = Zone && Zone->GetStages().IsValidIndex(StageIndex) ? Zone->GetStages()[StageIndex].Get() : nullptr;
	const FIntPoint& Grid = Zone ? Zone->GetRegionIdMapSize() : FIntPoint::ZeroValue;
	if (Stage == nullptr || Label == nullptr || Grid.X <= 0 || Grid.Y <= 0)
	{
		return;
	}

	// 원래 크기 글자의 크기 (지도 영역은 원본 px 크기의 SizeBox라 위젯 단위 = 지도 px)
	Label->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	Label->SetRenderScale(FVector2D(1.0f));
	Label->ForceLayoutPrepass();
	const FVector2D TextSize = Label->GetDesiredSize();
	if (TextSize.X <= 0.0f || TextSize.Y <= 0.0f)
	{
		return;
	}

	const FVector2D CellSize = Zone->GetStageMapSize() / FVector2D(Grid);
	const FVector2D& Center = Stage->GetLabelPosition();
	const int32 CenterX = FMath::FloorToInt(Center.X * Grid.X);
	const int32 CenterY = FMath::FloorToInt(Center.Y * Grid.Y);
	const float Aspect = TextSize.X / TextSize.Y;

	// 반높이를 한 칸씩 늘리며 글자 비율 사각형이 이 지역 칸만 덮는지 본다. 처음 벗어나는 크기 바로 앞이 답
	float FitHalfHeight = 0.0f;
	const float MaxHalfHeight = TextSize.Y * 0.5f;
	for (int32 Step = 1; Step * CellSize.Y <= MaxHalfHeight + CellSize.Y; ++Step)
	{
		const float HalfHeight = Step * CellSize.Y;
		const int32 HalfCellsX = FMath::CeilToInt(HalfHeight * Aspect / CellSize.X);
		bool bInside = true;
		for (int32 Y = CenterY - Step; Y <= CenterY + Step && bInside; ++Y)
		{
			for (int32 X = CenterX - HalfCellsX; X <= CenterX + HalfCellsX; ++X)
			{
				if (Zone->GetRegionIdAt(X, Y) != StageIndex)
				{
					bInside = false;
					break;
				}
			}
		}
		if (!bInside)
		{
			break;
		}
		FitHalfHeight = HalfHeight;
	}

	const float Scale = FMath::Clamp(FitHalfHeight * 2.0f * LabelFitMargin / TextSize.Y, MinLabelScale, 1.0f);
	Label->SetRenderScale(FVector2D(Scale));
}

void UUSStageMapWidget::RefreshInfo()
{
	const UUSMetaProgressionSubsystem* Meta = GetMeta();
	const TArray<TObjectPtr<UUSStageData>>* Stages = Zone ? &Zone->GetStages() : nullptr;
	const UUSStageData* Stage = Stages && Stages->IsValidIndex(SelectedIndex) ? (*Stages)[SelectedIndex].Get() : nullptr;
	const bool bOpen = Meta && Stage && Meta->IsStageOpen(Zone, Stage);
	const bool bCleared = Meta && Stage && Meta->IsStageCleared(Stage);

	FText WallInfo;
	if (Stage && Stage->IsWallStage())
	{
		FNumberFormattingOptions OneDecimal;
		OneDecimal.MinimumFractionalDigits = 1;
		OneDecimal.MaximumFractionalDigits = 1;
		WallInfo = FText::Format(WallInfoFormat, FText::AsNumber(Stage->GetHealthBonus(), &OneDecimal));
	}
	if (WallText)
	{
		WallText->SetText(WallInfo);
		WallText->SetVisibility(WallInfo.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (StageNameText)
	{
		StageNameText->SetText(Stage ? FText::Format(StageNameFormat, FText::AsNumber(Stage->GetStageNumber()), Stage->GetDisplayName()) : FText::GetEmpty());
	}

	if (StageInfoText)
	{
		TArray<FText> Names;
		if (Stage)
		{
			for (const UUSEnemyTypeData* Enemy : Stage->GetEnemyTypes())
			{
				Names.Add(Enemy->GetDisplayName());
			}
		}
		const FText Objective = Stage && Stage->IsBossStage() ? BossObjective : SurviveObjective;
		FText Body = FText::Format(StageInfoFormat, FText::Join(NSLOCTEXT("USStageMapWidget", "EnemySeparator", " · "), Names), Objective);
		// "위험" 줄은 빨간 WallText가 따로 있으면 거기, 없으면 본문 끝에
		if (WallText == nullptr && !WallInfo.IsEmpty())
		{
			Body = FText::Format(NSLOCTEXT("USStageMapWidget", "WallInfoAppendFormat", "{0}\n{1}"), Body, WallInfo);
		}
		StageInfoText->SetText(bCleared ? FText::Format(NSLOCTEXT("USStageMapWidget", "ClearedInfoFormat", "{0}\n{1}"), Body, ClearedSuffix) : Body);
	}

	if (LockedNotice)
	{
		LockedNotice->SetText(Stage && Stage->IsBossStage() ? BossLockedText : LockedText);
		LockedNotice->SetVisibility(Stage && !bOpen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (SquadButton)
	{
		SquadButton->SetIsEnabled(bOpen);
	}

	if (ProgressText && Zone && Meta)
	{
		ProgressText->SetText(FText::Format(ProgressFormat, FText::AsNumber(Meta->GetClearedStageCount(Zone)), FText::AsNumber(Zone->GetStages().Num())));
	}
}

void UUSStageMapWidget::HandleSquadClicked()
{
	const UUSMetaProgressionSubsystem* Meta = GetMeta();
	const UUSStageData* Stage = Zone && Zone->GetStages().IsValidIndex(SelectedIndex) ? Zone->GetStages()[SelectedIndex].Get() : nullptr;
	if (Meta && Stage && Meta->IsStageOpen(Zone, Stage))
	{
		OnSquadRequested.ExecuteIfBound(Zone, Stage);
	}
}

void UUSStageMapWidget::HandleBackClicked()
{
	OnBackRequested.ExecuteIfBound();
}
