#include "UI/USHangarMapWidget.h"
#include "US.h"
#include "Datas/USEnemyTypeData.h"
#include "Datas/USZoneData.h"
#include "UI/USHangarOptionWidget.h"
#include "UI/USMapOrbitWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"

namespace
{
	/** 정보 카드가 보이기 시작하는 확대 정도 (0 = 전체 지도, 1 = 다가감) */
	constexpr float CardFadeStart = 0.6f;

	/** 적 그림 바탕 칸 안쪽 여백 비율 (칸 크기 대비) */
	constexpr float EnemyIconPaddingRatio = 0.15f;

	/** 적 그림 바탕 칸 모서리 반지름 비율 */
	constexpr float EnemyIconCornerRatio = 0.2f;
}

UUSHangarMapWidget::UUSHangarMapWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LockedIslandText = NSLOCTEXT("USHangarMapWidget", "LockedIslandText", "정식판");
	EnemyNameSeparator = NSLOCTEXT("USHangarMapWidget", "EnemyLineSeparator", "\n");
}

void UUSHangarMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SquadButton)
	{
		SquadButton->OnClicked.AddUniqueDynamic(this, &UUSHangarMapWidget::HandleSquadClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &UUSHangarMapWidget::HandleBackClicked);
	}

	// Esc를 받으려면 포커스를 받을 수 있어야 한다 (포커스는 ShowMap에서 준다)
	SetIsFocusable(true);

	// 확대 이동은 지도 왼쪽 위를 기준으로 계산한다 (Offset = 화면에서 옮길 양)
	if (IslandLayer)
	{
		IslandLayer->SetRenderTransformPivot(FVector2D::ZeroVector);
	}
	ApplyZoom();
}

void UUSHangarMapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 움직이는 동안(0.6초)과 창 크기가 바뀐 때만 계산한다. 멈춰 있으면 바로 돌아간다
	const bool bMoving = ZoomAlpha < 1.0f;
	const bool bResized = IslandLayer && IslandLayer->GetCachedGeometry().GetLocalSize() != AppliedLayerSize;
	if (!bMoving && !bResized)
	{
		return;
	}

	if (bMoving)
	{
		ZoomAlpha = FMath::Min(ZoomAlpha + InDeltaTime / FocusDuration, 1.0f);
		// 처음엔 빠르게, 끝에서 천천히 멈춘다 (목업 cubic-bezier(.2,.7,.2,1)과 비슷한 느낌)
		const float Eased = FMath::InterpEaseOut(0.0f, 1.0f, ZoomAlpha, 3.0f);
		CurrentScale = FMath::Lerp(StartScale, TargetScale, Eased);
		CurrentOffset = FMath::Lerp(StartOffset, TargetOffset, Eased);
	}
	ApplyZoom();
}

FReply UUSHangarMapWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		// 다가가 있으면 전체 지도로, 전체 지도면 격납고로
		if (IsFocused())
		{
			ClearSelection();
		}
		else
		{
			HandleBackClicked();
		}
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UUSHangarMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 섬·버튼은 클릭을 먼저 먹으므로 여기 오는 건 빈 곳 클릭이다 → 전체 지도로
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && IsFocused())
	{
		ClearSelection();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UUSHangarMapWidget::ShowMap(bool bKeepSelection)
{
	// 늘 전체 지도에서 시작한다. 고르던 구역이 있으면 거기로 다가가 어디서 왔는지 보이게
	CurrentScale = 1.0f;
	CurrentOffset = FVector2D::ZeroVector;
	ZoomAlpha = 1.0f;
	bTargetFocused = false;
	ApplyZoom();

	if (!bKeepSelection)
	{
		ClearSelection();
	}
	else if (Zones.IsValidIndex(SelectedZone))
	{
		StartZoom(true);
	}
	SetKeyboardFocus();
}

void UUSHangarMapWidget::ClearSelection()
{
	SelectedZone = INDEX_NONE;
	RefreshIslands();
	if (SquadButton)
	{
		SquadButton->SetIsEnabled(false);
	}
	StartZoom(false);
}

void UUSHangarMapWidget::StartZoom(bool bFocus)
{
	const UUSZoneData* Zone = Zones.IsValidIndex(SelectedZone) ? Zones[SelectedZone].Get() : nullptr;
	bTargetFocused = bFocus && Zone != nullptr;

	StartScale = CurrentScale;
	StartOffset = CurrentOffset;
	if (bTargetFocused)
	{
		// 섬 자리 P(지도 비율)가 확대 S 뒤 FocusPoint에 오려면: P × S + Offset = FocusPoint
		TargetScale = FocusScale;
		TargetOffset = FocusPoint - Zone->GetMapPosition() * FocusScale;
	}
	else
	{
		TargetScale = 1.0f;
		TargetOffset = FVector2D::ZeroVector;
	}
	ZoomAlpha = 0.0f;
}

void UUSHangarMapWidget::ApplyZoom()
{
	if (IslandLayer)
	{
		AppliedLayerSize = IslandLayer->GetCachedGeometry().GetLocalSize();
		IslandLayer->SetRenderTransform(FWidgetTransform(CurrentOffset * AppliedLayerSize, FVector2D(CurrentScale), FVector2D::ZeroVector, 0.0f));
	}

	// 다가간 정도 0(전체 지도) ~ 1(다가감)로 정보 카드와 둘레 어둡게를 같이 띄운다
	const float FocusAmount = FocusScale > 1.0f ? FMath::Clamp((CurrentScale - 1.0f) / (FocusScale - 1.0f), 0.0f, 1.0f) : 0.0f;
	if (ZoneCard)
	{
		// 카드는 마지막 구간(CardFadeStart ~ 1)에서만 보인다: 물러날 때는 출발하자마자 통째로 사라지고 다가갈 때는 거의 도착해서 뜬다.
		// 확대와 같이 0~1로 흐리면 짙은 바탕이 지도 배경과 비슷해 먼저 안 보이고 흰 글자만 오래 남아 따로 사라지는 것처럼 보였다
		const float CardOpacity = FMath::Clamp((FocusAmount - CardFadeStart) / (1.0f - CardFadeStart), 0.0f, 1.0f);
		ZoneCard->SetRenderOpacity(CardOpacity);
		// 다 사라졌을 때는 클릭도 막지 않게 (뒤 섬을 누를 수 있게)
		ZoneCard->SetVisibility(CardOpacity > 0.0f ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Hidden);
	}
	if (Vignette)
	{
		Vignette->SetRenderOpacity(FocusAmount);
	}
}

void UUSHangarMapWidget::SetupMap(const TArray<TObjectPtr<UUSZoneData>>& InZones)
{
	Zones = InZones;
	Islands.Reset();
	SelectedZone = INDEX_NONE;

	if (IslandLayer == nullptr)
	{
		return;
	}
	IslandLayer->ClearChildren();

	// 궤도 그림을 섬보다 먼저(아래에) 넣는다. 지도 영역 전체를 덮어 섬과 같은 비율 좌표를 쓴다
	OrbitWidget = CreateWidget<UUSMapOrbitWidget>(this, UUSMapOrbitWidget::StaticClass());
	if (OrbitWidget)
	{
		if (UCanvasPanelSlot* OrbitSlot = IslandLayer->AddChildToCanvas(OrbitWidget))
		{
			OrbitSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			OrbitSlot->SetOffsets(FMargin(0.0f));
		}
		OrbitWidget->SetStyle(OrbitColor, SelectedOrbitColor, LockedOrbitColor, OrbitThickness, HubTexture, HubSize);
	}

	if (IslandClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSHangarMapWidget: IslandClass is not set. The map has no islands."));
	}

	Islands.SetNum(Zones.Num());
	for (int32 Index = 0; Index < Zones.Num(); ++Index)
	{
		const UUSZoneData* Zone = Zones[Index];
		if (Zone == nullptr || IslandClass == nullptr)
		{
			continue;
		}

		if (Zone->GetIslandImage() == nullptr)
		{
			UE_LOG(LogUS, Warning, TEXT("UUSHangarMapWidget: %s has no IslandImage. The island shows only its name."), *GetNameSafe(Zone));
		}

		UUSHangarOptionWidget* Island = CreateWidget<UUSHangarOptionWidget>(this, IslandClass);
		if (Island == nullptr)
		{
			continue;
		}

		// 구역 위치는 지도 영역 비율이라 앵커를 그 점에 모으고 섬 가운데를 맞춘다 → 창 크기가 바뀌어도 섬 배치 비율이 그대로
		if (UCanvasPanelSlot* IslandSlot = IslandLayer->AddChildToCanvas(Island))
		{
			const FVector2D& Position = Zone->GetMapPosition();
			IslandSlot->SetAnchors(FAnchors(Position.X, Position.Y));
			IslandSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			IslandSlot->SetPosition(FVector2D::ZeroVector);
			IslandSlot->SetSize(IslandSize);
		}

		// 확대는 렌더 변환이라 레이아웃(옆 섬 위치)을 흔들지 않는다. 가운데를 기준으로 커지게 한다
		Island->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Island->OnOptionClicked.BindUObject(this, &UUSHangarMapWidget::HandleIslandClicked);
		Islands[Index] = Island;
	}

	// 처음엔 아무것도 고르지 않는다: 지도 전체를 먼저 보고 사용자가 직접 고른다
	ClearSelection();
}

void UUSHangarMapWidget::HandleIslandClicked(int32 ZoneIndex)
{
	if (!Zones.IsValidIndex(ZoneIndex) || Zones[ZoneIndex] == nullptr)
	{
		return;
	}
	SelectZone(ZoneIndex);
}

void UUSHangarMapWidget::SelectZone(int32 ZoneIndex)
{
	SelectedZone = ZoneIndex;
	RefreshIslands();
	RefreshInfo();

	// 잠긴 구역은 보여 주기만 하고 편성으로 넘어가지 않는다
	const UUSZoneData* Zone = Zones.IsValidIndex(SelectedZone) ? Zones[SelectedZone].Get() : nullptr;
	if (SquadButton)
	{
		SquadButton->SetIsEnabled(Zone != nullptr && !Zone->IsLockedInDemo());
	}

	if (Zone)
	{
		StartZoom(true);
	}
}

void UUSHangarMapWidget::RefreshIslands()
{
	TArray<FUSMapOrbit> Orbits;
	for (int32 Index = 0; Index < Islands.Num(); ++Index)
	{
		UUSHangarOptionWidget* Island = Islands[Index];
		const UUSZoneData* Zone = Zones.IsValidIndex(Index) ? Zones[Index].Get() : nullptr;
		if (Island == nullptr || Zone == nullptr)
		{
			continue;
		}

		const bool bLocked = Zone->IsLockedInDemo();
		const bool bSelected = Index == SelectedZone;
		Island->SetupOption(Index, Zone->GetDisplayName(), bLocked ? LockedIslandText : FText::GetEmpty(), Zone->GetIslandImage(), !bLocked, bSelected);
		Island->SetRenderScale(FVector2D(bSelected ? SelectedIslandScale : 1.0f));
		Island->SetRenderOpacity(bLocked ? LockedIslandOpacity : 1.0f);

		FUSMapOrbit& Orbit = Orbits.AddDefaulted_GetRef();
		Orbit.Position = Zone->GetMapPosition();
		Orbit.bLocked = bLocked;
		Orbit.bSelected = bSelected;
	}

	if (OrbitWidget)
	{
		OrbitWidget->SetOrbits(OrbitCenter, OrbitAspect, Orbits);
	}
}

void UUSHangarMapWidget::RefreshInfo()
{
	const UUSZoneData* Zone = Zones.IsValidIndex(SelectedZone) ? Zones[SelectedZone].Get() : nullptr;

	if (ZoneNameText)
	{
		ZoneNameText->SetText(Zone ? Zone->GetDisplayName() : FText::GetEmpty());
	}
	if (ZoneDescriptionText)
	{
		ZoneDescriptionText->SetText(Zone ? Zone->GetDescription() : FText::GetEmpty());
	}
	if (ObjectiveText)
	{
		ObjectiveText->SetText(Zone ? Zone->GetObjectiveText() : FText::GetEmpty());
	}
	if (BossText)
	{
		const UUSEnemyTypeData* Boss = Zone ? Zone->GetBoss() : nullptr;
		FFormatNamedArguments Args;
		Args.Add(TEXT("Boss"), Boss ? Boss->GetDisplayName() : FText::GetEmpty());
		BossText->SetText(FText::Format(BossFormat, Args));
		BossText->SetVisibility(Boss ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	// 잠긴 구역: 적·목표 대신 "정식판에서 열립니다"
	const bool bLocked = Zone && Zone->IsLockedInDemo();
	if (ZoneDetailBox)
	{
		ZoneDetailBox->SetVisibility(bLocked ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
	}
	if (LockedNotice)
	{
		LockedNotice->SetVisibility(bLocked ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (EnemyIconRow == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	// 구역을 바꿀 때만 불리므로(클릭) 칸을 매번 새로 만들어도 비용이 작다
	EnemyIconRow->ClearChildren();
	TArray<FText> Names;
	if (Zone)
	{
		const float IconPadding = EnemyIconSize * EnemyIconPaddingRatio;
		const FSlateRoundedBoxBrush BackgroundBrush(EnemyIconBackgroundColor, EnemyIconSize * EnemyIconCornerRatio);

		for (const UUSEnemyTypeData* Enemy : Zone->GetEnemies())
		{
			if (Enemy == nullptr)
			{
				continue;
			}
			Names.Add(Enemy->GetSummary().IsEmpty() ? Enemy->GetDisplayName() : FText::Format(EnemyLineFormat, Enemy->GetDisplayName(), Enemy->GetSummary()));

			if (Enemy->GetIcon() == nullptr)
			{
				UE_LOG(LogUS, Warning, TEXT("UUSHangarMapWidget: %s has no Icon. Only its name is shown."), *GetNameSafe(Enemy));
				continue;
			}

			// 바탕 칸(Border) 안에 그림 하나. 크기는 SizeBox로 고정해 그림 원본 해상도와 상관없게 한다
			USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			Box->SetWidthOverride(EnemyIconSize);
			Box->SetHeightOverride(EnemyIconSize);

			UBorder* Background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			Background->SetBrush(BackgroundBrush);
			Background->SetPadding(FMargin(IconPadding));

			UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			Icon->SetBrushFromTexture(Enemy->GetIcon());
			Icon->SetColorAndOpacity(EnemyIconColor);

			Background->SetContent(Icon);
			Box->SetContent(Background);
			if (UHorizontalBoxSlot* IconSlot = Cast<UHorizontalBoxSlot>(EnemyIconRow->AddChild(Box)))
			{
				IconSlot->SetPadding(FMargin(0.0f, 0.0f, EnemyIconSpacing, 0.0f));
			}
		}
	}

	if (EnemyNamesText)
	{
		EnemyNamesText->SetText(FText::Join(EnemyNameSeparator, Names));
	}
}

void UUSHangarMapWidget::HandleSquadClicked()
{
	if (Zones.IsValidIndex(SelectedZone) && Zones[SelectedZone] && !Zones[SelectedZone]->IsLockedInDemo())
	{
		OnSquadRequested.ExecuteIfBound(Zones[SelectedZone]);
	}
}

void UUSHangarMapWidget::HandleBackClicked()
{
	OnBackRequested.ExecuteIfBound();
}
