#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Datas/USAllyTypeData.h"
#include "Fonts/SlateFontInfo.h"
#include "UI/USHangarCraftPanelWidget.h"
#include "USHangarBlueprintWidget.generated.h"

class AUSPlayerCharacter;
class UImage;
class UTextBlock;
class UTexture2D;

/**
 * 격납고 설계도 화면 (목업 B2 → 다듬기 목업 A, 레퍼런스: FTL 함선 화면): 가운데 기체 도면, 좌우에 강화 카드, 카드에서 도면 위 부위로 선을 긋는다.
 * 칸 만들기·구매·다시 그리기는 부모(UUSHangarCraftPanelWidget)가 하고, 이 클래스는 놓는 자리와 그림(선·도면 틀·치수선)과 표제란만 맡는다.
 * 카드 자리는 칸 데이터 UUSMetaUpgradeData::BlueprintAnchor로 정한다: X < 0.5면 LeftCards, 아니면 RightCards.
 * 세로 위치는 부위 높이(Y)에 카드 가운데를 맞추고, 겹치면 아래로(넘치면 위로) CardGap만큼 밀어낸다 → 선이 거의 수평이 된다.
 * 강화 칸을 늘려도 데이터 작업만으로 끝난다.
 * 카드 높이와 도면이 그려진 자리는 레이아웃이 끝나야 알 수 있어 NativeTick에서 맞춘다 (바뀌었을 때만 다시 계산).
 * 선·도면 틀·치수선·마우스 올림 강조는 NativePaint에서 그린다. 잠긴 기종은 도면을 흐리게 한다.
 * 도면 그림은 사방 DrawingPadding px 여백(발광 자리)을 두고 뽑으므로, 부위 좌표(0~1)는 여백을 뺀 안쪽 사각형 기준이다.
 * 위젯 이름: TitleText, BlueprintImage(ScaleBox 안에 두어 비율 유지), LeftCards, RightCards (CanvasPanel, 세로로 꽉 채움),
 * (선택) 표제란 ClassText, HealthText, DamageText, FireRateText, SpeedText.
 */
UCLASS()
class US_API UUSHangarBlueprintWidget : public UUSHangarCraftPanelWidget
{
	GENERATED_BODY()

public:
	UUSHangarBlueprintWidget(const FObjectInitializer& ObjectInitializer);

	virtual void ShowCraft(const UUSMetaTreeData* Tree, const UUSAllyTypeData* OwnerType, const FText& Title, const FText& Description) override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual void HandleMetaChanged() override;
	virtual void AddNodeEntry(UUSMetaNodeEntryWidget* Entry, const UUSMetaUpgradeData* Node) override;
	virtual void ClearNodeEntries() override;

	/** 지금 기종이 잠겼으면 도면을 흐리게 한다 */
	void RefreshLockedLook();

	/** 표제란(분류·사양 4칸)을 지금 기종으로 채운다 */
	void RefreshSpecBlock();

	/** 마우스 아래 카드를 찾아 바뀌었으면 다시 그리게 한다 */
	void UpdateHoveredEntry();

	/** Column의 카드를 부위 높이에 맞춰 놓는다. 카드 크기가 아직 정해지지 않았으면 false (다음 틱에 다시) */
	bool LayoutColumn(UPanelWidget* Column);

	/** 도면 좌표(0~1, 여백 뺀 안쪽 사각형 기준)를 화면 절대 좌표로 */
	FVector2f DrawingToAbsolute(const FVector2D& DrawingCoordinates) const;

	/** 도면 틀·치수선·도면 번호를 그린다 */
	void PaintDrawingFrame(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

	/** 글자를 Center 가운데에 그린다. bVertical이면 아래에서 위로 읽게 세운다. bMask면 뒤에 바탕색 사각형을 깔아 선을 가린다 */
	void PaintLabel(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FText& Text, const FVector2f& Center, bool bVertical, bool bMask) const;

	/** 지금 기종의 도면 정보 (대장기는 LeaderSheet) */
	const FUSBlueprintSheet& GetShownSheet() const;

	/** 대장기 도면 (대장기는 기종 데이터가 없어 여기 둔다) */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint")
	TObjectPtr<UTexture2D> LeaderBlueprintImage;

	/** 대장기 도면 번호·분류·치수 */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint")
	FUSBlueprintSheet LeaderSheet;

	/** 대장기 BP. 표제란 사양(체력·공격 배율·연사·속도)을 이 클래스의 CDO에서 읽는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint")
	TSubclassOf<AUSPlayerCharacter> LeaderClass;

	/** 도면 그림 사방의 여백(px, 그림 원본 기준). Tools/blueprint_from_obj.py의 GLOW_PADDING과 같아야 한다 */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint", meta = (ClampMin = "0.0"))
	float DrawingPadding = 24.0f;

	/** 잠긴 기종 도면의 불투명도 */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LockedImageOpacity = 0.4f;

	/** 같은 쪽 카드 사이 최소 간격(px) */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint", meta = (ClampMin = "0.0"))
	float CardGap = 10.0f;

	/** 카드 → 부위 선 색 */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Line")
	FLinearColor LineColor = FLinearColor(0.08f, 0.4f, 0.48f, 1.0f);

	/** 선 굵기(px) */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Line", meta = (ClampMin = "0.5"))
	float LineThickness = 1.5f;

	/** 부위 점 색 */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Line")
	FLinearColor DotColor = FLinearColor(0.89f, 0.55f, 0.15f, 1.0f);

	/** 부위 점 지름(px) */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Line", meta = (ClampMin = "1.0"))
	float DotSize = 10.0f;

	/** 마우스를 올린 카드의 테두리·선·점 색 */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Hover")
	FLinearColor HoverColor = FLinearColor(FColor(255, 178, 74));

	/** 마우스를 올린 카드의 선 굵기(px) */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Hover", meta = (ClampMin = "0.5"))
	float HoverLineThickness = 2.2f;

	/** 카드 테두리 모서리 반지름(px). WBP_USMetaNodeEntry 버튼 모양에 맞춘다 */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Hover", meta = (ClampMin = "0.0"))
	float CardCornerRadius = 6.0f;

	/** 부위 점선 고리 지름(px) */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Hover", meta = (ClampMin = "1.0"))
	float HoverRingSize = 32.0f;

	/** 고리 바깥 은은한 빛 지름(px) */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Hover", meta = (ClampMin = "1.0"))
	float HoverHaloSize = 52.0f;

	/** 도면 틀이 도면에서 떨어진 거리(px, X = 좌우, Y = 위아래) */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Frame")
	FVector2D FrameMargin = FVector2D(34.0, 22.0);

	/** 도면 틀 모서리 꺾쇠 길이(px) */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Frame", meta = (ClampMin = "1.0"))
	float FrameCornerLength = 18.0f;

	/** 치수선이 도면에서 떨어진 거리(px) */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Frame", meta = (ClampMin = "0.0"))
	float DimensionOffset = 14.0f;

	/** 도면 틀·치수선 색 (목업 #4F8FA3. FColor는 sRGB라 FLinearColor로 바꿀 때 감마가 풀린다) */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Frame")
	FLinearColor FrameColor = FLinearColor(FColor(79, 143, 163));

	/** 도면 틀·치수선 글자 색 */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Frame")
	FLinearColor FrameTextColor = FLinearColor(FColor(143, 198, 214));

	/** 치수 글자 뒤에 깔아 선을 가리는 색 (화면 바탕 모눈과 같은 색) */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Frame")
	FLinearColor FrameMaskColor = FLinearColor(FColor(13, 23, 38));

	/** 도면 틀·치수선 글꼴 */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Frame")
	FSlateFontInfo FrameFont;

	/** 도면 번호 글자. {0} = DrawingCode */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Text")
	FText DrawingCodeFormat;

	/** 도면 틀 오른쪽 위 글자 */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Text")
	FText FrameNote;

	/** 전폭 치수 글자. {0} = m */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Text")
	FText SpanFormat;

	/** 전장 치수 글자. {0} = m */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Text")
	FText LengthFormat;

	/** 공격 배율 글자. {0} = 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Text")
	FText DamageFormat;

	/** 연사 글자. {0} = 초당 발사 수 */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Text")
	FText FireRateFormat;

	/** 값이 없는 칸 (총이 없는 기종의 연사) */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint|Text")
	FText NoValueText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BlueprintImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> LeftCards;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> RightCards;

	/** 표제란: 분류 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ClassText;

	/** 표제란: 체력 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText;

	/** 표제란: 공격 배율 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DamageText;

	/** 표제란: 연사 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> FireRateText;

	/** 표제란: 속도 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SpeedText;

private:
	UPROPERTY()
	TObjectPtr<const UUSMetaTreeData> ShownTree;

	/** 지금 보여 주는 기종 (nullptr = 대장기) */
	UPROPERTY()
	TObjectPtr<const UUSAllyTypeData> ShownType;

	/** 마우스를 올린 카드 (NodeEntries 번호, 없으면 INDEX_NONE) */
	int32 HoveredEntryIndex = INDEX_NONE;

	/** 카드를 다시 놓아야 하는가 (칸이 바뀜) */
	bool bCardsDirty = true;

	/** 지난번 카드를 놓을 때 도면이 그려진 자리 (창 크기가 바뀌면 다시 놓는다) */
	FVector2f LastImagePosition = FVector2f::ZeroVector;
	FVector2f LastImageSize = FVector2f::ZeroVector;

	/** 카드 칸이 CanvasPanel이 아니라는 경고를 한 번만 낸다 */
	bool bWarnedColumnType = false;

	/** 원 (부위 점·은은한 빛). 둥근 상자 기본값이 "높이 절반 반지름"이라 정사각형에 그리면 원이 된다 */
	FSlateRoundedBoxBrush CircleBrush = FSlateRoundedBoxBrush(FLinearColor::White);

	/** 카드 테두리 (속 빈 둥근 상자, CardCornerRadius·HoverColor로 NativeConstruct에서 만든다) */
	FSlateRoundedBoxBrush CardOutlineBrush = FSlateRoundedBoxBrush(FLinearColor::Transparent, 6.0f, FLinearColor::White, 2.0f);

	/** 글자 뒤 가림막 (단색) */
	FSlateColorBrush MaskBrush = FSlateColorBrush(FLinearColor::White);
};
