#include "UI/USHangarBlueprintWidget.h"
#include "US.h"
#include "Agents/USPlayerCharacter.h"
#include "Components/USAttributeComponent.h"
#include "Datas/USMetaUpgradeData.h"
#include "Progression/USMetaProgressionSubsystem.h"
#include "UI/USMetaNodeEntryWidget.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

namespace
{
	/** 카드를 도면 왼쪽에 둘지 (아니면 오른쪽) */
	bool IsLeftCard(const UUSMetaUpgradeData* Node)
	{
		return Node && Node->GetBlueprintAnchor().X < 0.5f;
	}

	/**
	 * 카드에서 실제로 보이는 버튼(NodeButton)의 위치·크기. 카드 위젯 전체에는 카드 사이 간격(아래 여백)과 버튼 둘레 여백이
	 * 들어 있어, 전체 크기로 테두리를 그리면 위는 버튼 테두리와 겹치고 아래만 떨어져 보인다. 버튼을 못 찾으면 카드 전체를 쓴다
	 */
	const FGeometry& GetCardGeometry(const UUSMetaNodeEntryWidget& Entry)
	{
		const UWidget* Button = Entry.GetWidgetFromName(TEXT("NodeButton"));
		return Button ? Button->GetCachedGeometry() : Entry.GetCachedGeometry();
	}

	/** 점선 고리 한 바퀴를 몇 토막으로 나눌지 (토막마다 절반은 비운다) */
	constexpr int32 RingDashCount = 12;

	/** 점선 토막 하나를 몇 개의 직선으로 근사할지 */
	constexpr int32 RingDashSegments = 3;

	/** 치수선 화살촉 길이(px) */
	constexpr float ArrowLength = 6.0f;

	/** 치수선 양 끝 눈금 길이의 절반(px) */
	constexpr float TickHalfLength = 6.0f;

	/** 도면 틀 글자를 꺾쇠 끝에서 띄우는 거리(px) */
	constexpr float FrameLabelInset = 8.0f;

	/** 글자 가림막 좌우 여유(px) */
	constexpr float MaskPadding = 4.0f;

	/** 도면 틀·치수선 굵기(px) */
	constexpr float FrameThickness = 1.0f;

	FText FormatNumber(float Value, int32 FractionalDigits)
	{
		FNumberFormattingOptions Options;
		Options.MinimumFractionalDigits = FractionalDigits;
		Options.MaximumFractionalDigits = FractionalDigits;
		return FText::AsNumber(Value, &Options);
	}

	void SetTextIfBound(UTextBlock* TextBlock, const FText& Text)
	{
		if (TextBlock)
		{
			TextBlock->SetText(Text);
		}
	}

	/** 두 점을 잇는 직선 하나 */
	void PaintSegment(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry,
		const FVector2f& From, const FVector2f& To, const FLinearColor& Color, float Thickness)
	{
		TArray<FVector2f> Points;
		Points.Reserve(2);
		Points.Add(From);
		Points.Add(To);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), MoveTemp(Points),
			ESlateDrawEffect::None, Color, true, Thickness);
	}

	/** Tip을 향한 열린 화살촉 (Direction = 화살이 가리키는 단위 방향) */
	void PaintArrowHead(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry,
		const FVector2f& Tip, const FVector2f& Direction, const FLinearColor& Color)
	{
		const FVector2f Back = Tip - Direction * ArrowLength;
		const FVector2f Side(-Direction.Y * ArrowLength * 0.5f, Direction.X * ArrowLength * 0.5f);

		TArray<FVector2f> Points;
		Points.Reserve(3);
		Points.Add(Back + Side);
		Points.Add(Tip);
		Points.Add(Back - Side);
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), MoveTemp(Points),
			ESlateDrawEffect::None, Color, true, FrameThickness);
	}
}

UUSHangarBlueprintWidget::UUSHangarBlueprintWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DrawingCodeFormat = NSLOCTEXT("USHangarBlueprintWidget", "DrawingCodeFormat", "DWG {0}");
	FrameNote = NSLOCTEXT("USHangarBlueprintWidget", "FrameNote", "REV A");
	SpanFormat = NSLOCTEXT("USHangarBlueprintWidget", "SpanFormat", "전폭 {0} m");
	LengthFormat = NSLOCTEXT("USHangarBlueprintWidget", "LengthFormat", "전장 {0} m");
	DamageFormat = NSLOCTEXT("USHangarBlueprintWidget", "DamageFormat", "×{0}");
	FireRateFormat = NSLOCTEXT("USHangarBlueprintWidget", "FireRateFormat", "{0}/s");
	NoValueText = NSLOCTEXT("USHangarBlueprintWidget", "NoValueText", "—");
	FrameFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);
}

void UUSHangarBlueprintWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 둥근 상자의 속은 MakeBox에 넘긴 틴트로, 테두리는 브러시의 OutlineSettings 색으로 칠해진다 → 테두리 색을 브러시에 넣고 틴트는 투명으로 넘긴다
	CardOutlineBrush = FSlateRoundedBoxBrush(FLinearColor::Transparent, CardCornerRadius, HoverColor, 2.0f);
}

void UUSHangarBlueprintWidget::ShowCraft(const UUSMetaTreeData* Tree, const UUSAllyTypeData* OwnerType, const FText& Title, const FText& Description)
{
	ShownTree = Tree;
	ShownType = OwnerType;
	HoveredEntryIndex = INDEX_NONE;

	if (BlueprintImage)
	{
		UTexture2D* Texture = OwnerType ? OwnerType->GetBlueprintImage() : LeaderBlueprintImage.Get();
		if (Texture)
		{
			// 그림 크기 그대로 두고 WBP의 ScaleBox가 비율을 유지해 맞춘다 (늘이면 BlueprintAnchor 좌표가 부위에서 어긋난다)
			BlueprintImage->SetBrushFromTexture(Texture, true);
			BlueprintImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			UE_LOG(LogUS, Warning, TEXT("[%s] No blueprint image for %s. Set BlueprintImage on the ally type (or LeaderBlueprintImage)."),
				*GetName(), OwnerType ? *GetNameSafe(OwnerType) : TEXT("leader"));
			BlueprintImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	RefreshSpecBlock();

	// 부모가 칸을 만들어 AddNodeEntry로 넘기고, 끝에서 HandleMetaChanged로 잠김 모양까지 맞춘다
	Super::ShowCraft(Tree, OwnerType, Title, Description);
}

const FUSBlueprintSheet& UUSHangarBlueprintWidget::GetShownSheet() const
{
	return ShownType ? ShownType->GetBlueprintSheet() : LeaderSheet;
}

void UUSHangarBlueprintWidget::RefreshSpecBlock()
{
	SetTextIfBound(ClassText, GetShownSheet().ClassName);

	float MaxHealth = 0.0f;
	float DamageMultiplier = 0.0f;
	float FireInterval = 0.0f;
	float MoveSpeed = 0.0f;
	bool bHasGun = true;

	if (ShownType)
	{
		MaxHealth = ShownType->GetMaxHealth();
		DamageMultiplier = ShownType->GetAttackDamageMultiplier();
		FireInterval = ShownType->GetRapidFireInterval();
		MoveSpeed = ShownType->GetMoveSpeed();
		// 방패기(Tank)·EMP기(Slow)는 총 대신 능력 컴포넌트로 싸워 RapidFireInterval이 쓰이지 않는다
		bHasGun = ShownType->GetRole() == EUSAllyRole::Ranged;
	}
	else
	{
		// 대장기는 기종 데이터가 없어 BP 기본값(CDO)에서 읽는다. CDO의 기본 서브오브젝트(AttributeComponent)에도 BP에서 고친 값이 들어 있다
		const AUSPlayerCharacter* Leader = LeaderClass ? LeaderClass->GetDefaultObject<AUSPlayerCharacter>() : nullptr;
		if (Leader == nullptr)
		{
			UE_LOG(LogUS, Warning, TEXT("[%s] LeaderClass is not set. Leader specs stay empty."), *GetName());
			SetTextIfBound(HealthText, NoValueText);
			SetTextIfBound(DamageText, NoValueText);
			SetTextIfBound(FireRateText, NoValueText);
			SetTextIfBound(SpeedText, NoValueText);
			return;
		}

		const UUSAttributeComponent* Attribute = Leader->GetAttributeComponent();
		MaxHealth = Attribute ? Attribute->GetMaxHealth() : 0.0f;
		DamageMultiplier = Attribute ? Attribute->GetAttackDamageMultiplier() : 0.0f;
		FireInterval = Leader->GetRapidFireInterval();
		MoveSpeed = Leader->GetNormalSpeed();
	}

	SetTextIfBound(HealthText, FormatNumber(MaxHealth, 0));
	SetTextIfBound(DamageText, FText::Format(DamageFormat, FormatNumber(DamageMultiplier, 1)));
	// 연사 간격은 데이터에서 ClampMin 0.01이지만 0 나눗셈은 한 번 더 막는다 (초당 발사 수라 0~1 비율용 GetSafeRatio는 맞지 않는다)
	SetTextIfBound(FireRateText, bHasGun && FireInterval > KINDA_SMALL_NUMBER
		? FText::Format(FireRateFormat, FormatNumber(1.0f / FireInterval, 1)) : NoValueText);
	SetTextIfBound(SpeedText, FormatNumber(MoveSpeed, 0));
}

void UUSHangarBlueprintWidget::HandleMetaChanged()
{
	Super::HandleMetaChanged();
	RefreshLockedLook();
}

void UUSHangarBlueprintWidget::RefreshLockedLook()
{
	if (BlueprintImage == nullptr)
	{
		return;
	}

	const UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem();
	const bool bLocked = Meta && ShownType && !Meta->IsAllyTypeUnlocked(ShownTree, ShownType);
	BlueprintImage->SetOpacity(bLocked ? LockedImageOpacity : 1.0f);
}

void UUSHangarBlueprintWidget::AddNodeEntry(UUSMetaNodeEntryWidget* Entry, const UUSMetaUpgradeData* Node)
{
	UPanelWidget* Column = IsLeftCard(Node) ? LeftCards.Get() : RightCards.Get();
	if (Column == nullptr || Entry == nullptr)
	{
		return;
	}

	// 같은 쪽 안에서는 부위가 위에 있는 카드가 위로 오게 끼워 넣는다 (트리 순서와 도면 위치가 달라도 선이 꼬이지 않게)
	const float AnchorY = Node ? Node->GetBlueprintAnchor().Y : 0.5f;
	int32 InsertIndex = Column->GetChildrenCount();
	for (int32 Index = 0; Index < Column->GetChildrenCount(); ++Index)
	{
		const UUSMetaNodeEntryWidget* Other = Cast<UUSMetaNodeEntryWidget>(Column->GetChildAt(Index));
		if (Other && Other->GetNode() && Other->GetNode()->GetBlueprintAnchor().Y > AnchorY)
		{
			InsertIndex = Index;
			break;
		}
	}
	Column->InsertChildAt(InsertIndex, Entry);
	bCardsDirty = true;
}

void UUSHangarBlueprintWidget::ClearNodeEntries()
{
	if (LeftCards)
	{
		LeftCards->ClearChildren();
	}
	if (RightCards)
	{
		RightCards->ClearChildren();
	}
	HoveredEntryIndex = INDEX_NONE;
	bCardsDirty = true;
}

FVector2f UUSHangarBlueprintWidget::DrawingToAbsolute(const FVector2D& DrawingCoordinates) const
{
	// 그림 사방의 발광 여백을 빼고 안쪽 사각형에 대 본다. 여백 비율은 그림 크기마다 달라 px로 받아 여기서 나눈다
	const FVector2D TextureSize = BlueprintImage->GetBrush().GetImageSize();
	FVector2D PaddingRatio = FVector2D::ZeroVector;
	if (TextureSize.X > DrawingPadding * 2.0f && TextureSize.Y > DrawingPadding * 2.0f)
	{
		PaddingRatio = FVector2D(DrawingPadding / TextureSize.X, DrawingPadding / TextureSize.Y);
	}
	const FVector2D ImageCoordinates = PaddingRatio + DrawingCoordinates * (FVector2D::UnitVector - PaddingRatio * 2.0);
	return BlueprintImage->GetCachedGeometry().GetAbsolutePositionAtCoordinates(FVector2f(ImageCoordinates));
}

void UUSHangarBlueprintWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (BlueprintImage == nullptr || !BlueprintImage->IsVisible())
	{
		return;
	}

	UpdateHoveredEntry();

	// 평소엔 비교 두 번으로 끝난다. 칸이 바뀌었거나 도면 자리(창 크기)가 바뀐 프레임에만 다시 놓는다
	const FGeometry& ImageGeometry = BlueprintImage->GetCachedGeometry();
	const FVector2f ImagePosition = ImageGeometry.GetAbsolutePosition();
	const FVector2f ImageSize = ImageGeometry.GetAbsoluteSize();
	if (!bCardsDirty && ImagePosition.Equals(LastImagePosition) && ImageSize.Equals(LastImageSize))
	{
		return;
	}

	// 두 쪽 모두 돌려야 하므로 &&(단락 평가) 대신 따로 받는다
	const bool bLeftDone = LayoutColumn(LeftCards);
	const bool bRightDone = LayoutColumn(RightCards);
	bCardsDirty = !(bLeftDone && bRightDone);
	LastImagePosition = ImagePosition;
	LastImageSize = ImageSize;
}

void UUSHangarBlueprintWidget::UpdateHoveredEntry()
{
	// 카드 버튼은 잠김·최대일 때 꺼져 있어 버튼의 IsHovered로는 못 잡는다 → 커서 위치를 카드 영역과 직접 비교 (카드 대여섯 장이라 싸다)
	const FVector2f CursorPosition = FSlateApplication::Get().GetCursorPos();
	int32 NewHovered = INDEX_NONE;
	for (int32 Index = 0; Index < NodeEntries.Num(); ++Index)
	{
		const UUSMetaNodeEntryWidget* Entry = NodeEntries[Index];
		if (Entry && GetCardGeometry(*Entry).IsUnderLocation(CursorPosition))
		{
			NewHovered = Index;
			break;
		}
	}

	if (NewHovered != HoveredEntryIndex)
	{
		HoveredEntryIndex = NewHovered;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

bool UUSHangarBlueprintWidget::LayoutColumn(UPanelWidget* Column)
{
	if (Column == nullptr || Column->GetChildrenCount() == 0)
	{
		return true;
	}

	const FGeometry& ColumnGeometry = Column->GetCachedGeometry();
	const float ColumnHeight = ColumnGeometry.GetLocalSize().Y;
	if (ColumnHeight <= 0.0f)
	{
		return false;
	}

	// 자식 순서는 AddNodeEntry가 부위 Y 순으로 맞춰 두었다
	const int32 Count = Column->GetChildrenCount();
	TArray<float> Tops;
	TArray<float> Heights;
	Tops.Reserve(Count);
	Heights.Reserve(Count);

	// 위에서부터: 카드 가운데를 부위 높이에 맞추되 앞 카드와 CardGap 이상 띄운다
	float PreviousBottom = -CardGap;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const UUSMetaNodeEntryWidget* Card = Cast<UUSMetaNodeEntryWidget>(Column->GetChildAt(Index));
		const float Height = Card ? Card->GetDesiredSize().Y : 0.0f;
		if (Card == nullptr || Card->GetNode() == nullptr || Height <= 0.0f)
		{
			// 방금 넣은 카드는 첫 레이아웃 전이라 원하는 크기가 0이다
			return false;
		}

		const float AnchorY = FVector2f(ColumnGeometry.AbsoluteToLocal(DrawingToAbsolute(Card->GetNode()->GetBlueprintAnchor()))).Y;
		const float Top = FMath::Max(AnchorY - Height * 0.5f, PreviousBottom + CardGap);
		Tops.Add(Top);
		Heights.Add(Height);
		PreviousBottom = Top + Height;
	}

	// 아래에서부터: 칸 아래로 넘친 카드를 위로 당긴다 (맨 위는 0 아래로 내려가지 않게)
	float NextTop = ColumnHeight + CardGap;
	for (int32 Index = Count - 1; Index >= 0; --Index)
	{
		Tops[Index] = FMath::Max(0.0f, FMath::Min(Tops[Index], NextTop - CardGap - Heights[Index]));
		NextTop = Tops[Index];
	}

	for (int32 Index = 0; Index < Count; ++Index)
	{
		UCanvasPanelSlot* CardSlot = Cast<UCanvasPanelSlot>(Column->GetChildAt(Index)->Slot);
		if (CardSlot == nullptr)
		{
			if (!bWarnedColumnType)
			{
				UE_LOG(LogUS, Warning, TEXT("[%s] %s is not a CanvasPanel. Cards cannot follow part heights."), *GetName(), *Column->GetName());
				bWarnedColumnType = true;
			}
			return true;
		}

		// 가로는 칸 폭에 꽉 채우고(왼쪽·오른쪽 앵커), 세로는 위에서 Top, 높이 Height
		CardSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 0.0f));
		CardSlot->SetAlignment(FVector2D::ZeroVector);
		CardSlot->SetOffsets(FMargin(0.0f, Tops[Index], 0.0f, Heights[Index]));
	}
	return true;
}

int32 UUSHangarBlueprintWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// 자식(도면·카드)을 먼저 그려야 이번 프레임의 위치(GetCachedGeometry)가 정해지고, 선이 그 위에 올라간다
	const int32 ChildLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (BlueprintImage == nullptr || !BlueprintImage->IsVisible())
	{
		return ChildLayer;
	}

	const int32 FrameLayer = ChildLayer + 1;
	const int32 LineLayer = ChildLayer + 3;
	const int32 DotLayer = ChildLayer + 4;
	PaintDrawingFrame(AllottedGeometry, OutDrawElements, FrameLayer);

	const FVector2f DotExtent(DotSize, DotSize);
	for (int32 Index = 0; Index < NodeEntries.Num(); ++Index)
	{
		const UUSMetaNodeEntryWidget* Entry = NodeEntries[Index];
		if (Entry == nullptr || Entry->GetNode() == nullptr)
		{
			continue;
		}

		// 도면 위 부위: 0~1 좌표를 도면이 실제 그려진 안쪽 사각형에 대 본다. 카드 쪽 끝: 도면을 향한 변의 가운데
		const UUSMetaUpgradeData* Node = Entry->GetNode();
		const bool bHovered = Index == HoveredEntryIndex;
		const FLinearColor& Color = bHovered ? HoverColor : LineColor;
		const FGeometry& CardGeometry = GetCardGeometry(*Entry);
		const FVector2f AnchorLocal = AllottedGeometry.AbsoluteToLocal(DrawingToAbsolute(Node->GetBlueprintAnchor()));
		const FVector2f CardEdge(IsLeftCard(Node) ? 1.0f : 0.0f, 0.5f);
		const FVector2f CardLocal = AllottedGeometry.AbsoluteToLocal(CardGeometry.GetAbsolutePositionAtCoordinates(CardEdge));

		// 마우스 올린 선은 다른 선 위에 오게 한 칸 위 레이어
		PaintSegment(OutDrawElements, bHovered ? LineLayer + 1 : LineLayer, AllottedGeometry, CardLocal, AnchorLocal,
			Color, bHovered ? HoverLineThickness : LineThickness);

		// 카드 쪽 끝의 짧은 세로 눈금: 선이 카드에 "꽂힌" 느낌
		const FVector2f TickHalf(0.0f, TickHalfLength);
		PaintSegment(OutDrawElements, LineLayer, AllottedGeometry, CardLocal - TickHalf, CardLocal + TickHalf, Color, 2.0f);

		if (bHovered)
		{
			const FVector2f CardTopLeft = AllottedGeometry.AbsoluteToLocal(CardGeometry.GetAbsolutePositionAtCoordinates(FVector2f::ZeroVector));
			const FVector2f CardSize = FVector2f(AllottedGeometry.AbsoluteToLocal(CardGeometry.GetAbsolutePositionAtCoordinates(FVector2f::UnitVector))) - CardTopLeft;
			FSlateDrawElement::MakeBox(OutDrawElements, DotLayer, AllottedGeometry.ToPaintGeometry(CardSize, FSlateLayoutTransform(CardTopLeft)),
				&CardOutlineBrush, ESlateDrawEffect::None, FLinearColor::Transparent);

			const FVector2f HaloExtent(HoverHaloSize, HoverHaloSize);
			FLinearColor HaloColor = HoverColor;
			HaloColor.A *= 0.1f;
			FSlateDrawElement::MakeBox(OutDrawElements, LineLayer, AllottedGeometry.ToPaintGeometry(HaloExtent, FSlateLayoutTransform(AnchorLocal - HaloExtent * 0.5f)),
				&CircleBrush, ESlateDrawEffect::None, HaloColor);

			// 점선 고리: 한 바퀴를 RingDashCount 토막으로 나눠 토막마다 앞 절반만 짧은 꺾은선으로 긋는다
			const float Radius = HoverRingSize * 0.5f;
			const float DashAngle = static_cast<float>(UE_TWO_PI) / RingDashCount;
			for (int32 Dash = 0; Dash < RingDashCount; ++Dash)
			{
				TArray<FVector2f> Arc;
				Arc.Reserve(RingDashSegments + 1);
				for (int32 Step = 0; Step <= RingDashSegments; ++Step)
				{
					const float Angle = DashAngle * (Dash + 0.5f * Step / RingDashSegments);
					Arc.Add(AnchorLocal + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
				}
				FSlateDrawElement::MakeLines(OutDrawElements, DotLayer, AllottedGeometry.ToPaintGeometry(), MoveTemp(Arc),
					ESlateDrawEffect::None, HoverColor, true, 1.5f);
			}
		}

		const FVector2f Extent = bHovered ? DotExtent * 1.2f : DotExtent;
		FSlateDrawElement::MakeBox(OutDrawElements, DotLayer, AllottedGeometry.ToPaintGeometry(Extent, FSlateLayoutTransform(AnchorLocal - Extent * 0.5f)),
			&CircleBrush, ESlateDrawEffect::None, bHovered ? HoverColor : DotColor);
	}

	return DotLayer;
}

void UUSHangarBlueprintWidget::PaintDrawingFrame(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	// 도면이 실제 그려진 안쪽 사각형 (발광 여백 제외)
	const FVector2f TopLeft = AllottedGeometry.AbsoluteToLocal(DrawingToAbsolute(FVector2D::ZeroVector));
	const FVector2f BottomRight = AllottedGeometry.AbsoluteToLocal(DrawingToAbsolute(FVector2D::UnitVector));
	if (BottomRight.X <= TopLeft.X || BottomRight.Y <= TopLeft.Y)
	{
		return;
	}

	// 도면 틀: 네 모서리 꺾쇠만 (전체 사각형은 카드 선과 겹쳐 지저분해진다)
	const FVector2f Margin(FrameMargin);
	const FVector2f FrameMin = TopLeft - Margin;
	const FVector2f FrameMax = BottomRight + Margin;
	const float L = FrameCornerLength;
	const FVector2f Corners[4] = { FrameMin, FVector2f(FrameMax.X, FrameMin.Y), FrameMax, FVector2f(FrameMin.X, FrameMax.Y) };
	for (const FVector2f& Corner : Corners)
	{
		// 모서리에서 틀 안쪽으로 가로·세로 한 줄씩
		const float StepX = Corner.X == FrameMin.X ? L : -L;
		const float StepY = Corner.Y == FrameMin.Y ? L : -L;
		TArray<FVector2f> Points;
		Points.Reserve(3);
		Points.Add(Corner + FVector2f(0.0f, StepY));
		Points.Add(Corner);
		Points.Add(Corner + FVector2f(StepX, 0.0f));
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), MoveTemp(Points),
			ESlateDrawEffect::None, FrameColor, true, 1.5f);
	}

	// 위쪽 변의 글자: 왼쪽 도면 번호, 오른쪽 메모 (꺾쇠 끝 바깥, 틀 선 높이)
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FUSBlueprintSheet& Sheet = GetShownSheet();
	if (!Sheet.DrawingCode.IsEmpty())
	{
		const FText Code = FText::Format(DrawingCodeFormat, Sheet.DrawingCode);
		const float Width = FVector2f(FontMeasure->Measure(Code, FrameFont)).X;
		PaintLabel(AllottedGeometry, OutDrawElements, LayerId, Code, FVector2f(FrameMin.X + L + FrameLabelInset + Width * 0.5f, FrameMin.Y), false, false);
	}
	if (!FrameNote.IsEmpty())
	{
		const float Width = FVector2f(FontMeasure->Measure(FrameNote, FrameFont)).X;
		PaintLabel(AllottedGeometry, OutDrawElements, LayerId, FrameNote, FVector2f(FrameMax.X - L - FrameLabelInset - Width * 0.5f, FrameMin.Y), false, false);
	}

	if (Sheet.Size.X <= 0.0 || Sheet.Size.Y <= 0.0)
	{
		return;
	}

	// 전폭: 도면 아래 가로 치수선 (양 끝 눈금 + 화살촉 + 가운데 글자)
	const float SpanY = BottomRight.Y + DimensionOffset;
	const FVector2f SpanFrom(TopLeft.X, SpanY);
	const FVector2f SpanTo(BottomRight.X, SpanY);
	const FVector2f Tick(0.0f, TickHalfLength);
	PaintSegment(OutDrawElements, LayerId, AllottedGeometry, SpanFrom - Tick, SpanFrom + Tick, FrameColor, FrameThickness);
	PaintSegment(OutDrawElements, LayerId, AllottedGeometry, SpanTo - Tick, SpanTo + Tick, FrameColor, FrameThickness);
	PaintSegment(OutDrawElements, LayerId, AllottedGeometry, SpanFrom, SpanTo, FrameColor, FrameThickness);
	PaintArrowHead(OutDrawElements, LayerId, AllottedGeometry, SpanFrom, FVector2f(-1.0f, 0.0f), FrameColor);
	PaintArrowHead(OutDrawElements, LayerId, AllottedGeometry, SpanTo, FVector2f(1.0f, 0.0f), FrameColor);
	PaintLabel(AllottedGeometry, OutDrawElements, LayerId + 1, FText::Format(SpanFormat, FormatNumber(Sheet.Size.X, 1)),
		FVector2f((TopLeft.X + BottomRight.X) * 0.5f, SpanY), false, true);

	// 전장: 도면 왼쪽 세로 치수선 (글자는 세워서)
	const float LengthX = TopLeft.X - DimensionOffset;
	const FVector2f LengthFrom(LengthX, TopLeft.Y);
	const FVector2f LengthTo(LengthX, BottomRight.Y);
	const FVector2f SideTick(TickHalfLength, 0.0f);
	PaintSegment(OutDrawElements, LayerId, AllottedGeometry, LengthFrom - SideTick, LengthFrom + SideTick, FrameColor, FrameThickness);
	PaintSegment(OutDrawElements, LayerId, AllottedGeometry, LengthTo - SideTick, LengthTo + SideTick, FrameColor, FrameThickness);
	PaintSegment(OutDrawElements, LayerId, AllottedGeometry, LengthFrom, LengthTo, FrameColor, FrameThickness);
	PaintArrowHead(OutDrawElements, LayerId, AllottedGeometry, LengthFrom, FVector2f(0.0f, -1.0f), FrameColor);
	PaintArrowHead(OutDrawElements, LayerId, AllottedGeometry, LengthTo, FVector2f(0.0f, 1.0f), FrameColor);
	PaintLabel(AllottedGeometry, OutDrawElements, LayerId + 1, FText::Format(LengthFormat, FormatNumber(Sheet.Size.Y, 1)),
		FVector2f(LengthX, (TopLeft.Y + BottomRight.Y) * 0.5f), true, true);
}

void UUSHangarBlueprintWidget::PaintLabel(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FText& Text, const FVector2f& Center, bool bVertical, bool bMask) const
{
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FVector2f TextSize = FontMeasure->Measure(Text, FrameFont);
	const FSlateLayoutTransform Layout(Center - TextSize * 0.5f);

	// 세로 글자: 가로로 잰 상자를 가운데 기준으로 -90도 돌린다 (렌더 변환이라 레이아웃 크기는 가로 그대로)
	const FSlateRenderTransform Rotation(FQuat2f(FMath::DegreesToRadians(bVertical ? -90.0f : 0.0f)));

	if (bMask)
	{
		const FVector2f MaskSize = TextSize + FVector2f(MaskPadding * 2.0f, 0.0f);
		const FSlateLayoutTransform MaskLayout(Center - MaskSize * 0.5f);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(MaskSize, MaskLayout, Rotation),
			&MaskBrush, ESlateDrawEffect::None, FrameMaskColor);
	}

	FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(TextSize, Layout, Rotation),
		Text, FrameFont, ESlateDrawEffect::None, FrameTextColor);
}
