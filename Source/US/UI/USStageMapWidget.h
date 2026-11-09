#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Fonts/SlateFontInfo.h"
#include "UI/USHangarMapWidget.h"
#include "USStageMapWidget.generated.h"

class UButton;
class UCanvasPanel;
class UImage;
class UTextBlock;
class UUSMetaProgressionSubsystem;
class UUSStageData;
class UUSZoneData;

/** 지역 지도에서 스테이지를 정하고 "아군 편성"을 누름 */
DECLARE_DELEGATE_TwoParams(FDelegateOnStageSquadRequested, const UUSZoneData*, const UUSStageData*);

/**
 * 격납고 흐름 2-2단계 지역 지도: 행성 하나를 땅 모양 지역(스테이지)으로 나눠 고른다
 * (Into the Breach 섬 지역 고르기 + 사용자 참고 그림 "도 단위 지도", 목업 A 평면 대륙 2026-10-26).
 * 바탕(바다·해안선) 위에 지역 그림을 스테이지마다 한 장씩 얹고 상태 색을 곱한다: 깬 곳·고를 수 있는 곳·잠긴 곳·보스·고른 곳.
 * 고른 지역은 위로 들어 올리고 그림자를 깐다. 지역 그림은 사각형이라 클릭은 구역 데이터의 지역 번호 지도(RegionIdMap)로 가린다.
 * 벽 스테이지(체력 덤)는 아직 안 깼으면 빨간 테두리(같은 그림을 조금 키워 빨갛게 깐 층)와 붉은 지형, 정보 카드 "위험" 줄 (목업 3A, 2026-10-27).
 * 그림·글자는 ShowZone에서 C++이 만든다 — WBP는 지도 영역과 정보 카드만 둔다.
 * 위젯 이름: MapLayer(CanvasPanel, 지도 원본 비율을 지키는 SizeBox·ScaleBox 안), StageNameText, StageInfoText, SquadButton, BackButton,
 * (선택) ZoneNameText, ProgressText, LockedNotice, WallText.
 */
UCLASS()
class US_API UUSStageMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSStageMapWidget(const FObjectInitializer& ObjectInitializer);

	/** Zone의 지역 지도를 연다. bKeepSelection이면(편성에서 돌아옴) 고르던 지역 그대로, 아니면 다음에 할 스테이지를 고른다 */
	void ShowZone(const UUSZoneData* InZone, bool bKeepSelection);

	/** "아군 편성" 클릭 (고른 지역) */
	FDelegateOnStageSquadRequested OnSquadRequested;

	/** "출격 지도" 클릭 / Esc */
	FDelegateOnBackRequested OnBackRequested;

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** 지도 그림·글자를 새로 만든다 (구역이 바뀔 때만) */
	void BuildLayers();

	/** 화면 좌표의 스테이지 번호 (Stages 인덱스, 없으면 INDEX_NONE) */
	int32 FindStageIndexAt(const FVector2D& ScreenPosition) const;

	/** Index 지역을 고르고 정보 카드를 다시 쓴다 */
	void SelectStage(int32 Index);

	/** 지역 색·들어 올림·글자를 지금 상태로 */
	void RefreshRegions();

	/** 정보 카드 */
	void RefreshInfo();

	/**
	 * 지역 글자가 지역 밖으로 나가지 않게 줄인다 (사용자 피드백 2026-10-26: 가는 지역에서 "1 / CLEAR"가 삐져나옴, B안).
	 * 글자 자리를 가운데로 글자 비율의 사각형을 지역 번호 지도 칸 단위로 키워 가며 그 지역 칸만 덮는 가장 큰 크기를 찾는다.
	 * 글자가 바뀔 때만 부른다 (레이아웃 계산 한 번)
	 */
	void FitLabel(int32 StageIndex, UTextBlock* Label) const;

	UFUNCTION()
	void HandleSquadClicked();

	UFUNCTION()
	void HandleBackClicked();

	UUSMetaProgressionSubsystem* GetMeta() const;

	/** 깬 지역에 섞는 색과 비율 (지형 색이 남되 "끝난 곳"처럼 가라앉게) */
	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Color")
	FLinearColor ClearedTint = FLinearColor(0.08f, 0.24f, 0.28f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Color", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ClearedBlend = 0.45f;

	/** 잠긴 지역 밝기 (지형 색에 곱한다) */
	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Color", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LockedBrightness = 0.28f;

	/** 고른 지역 색 (게임 UI 하늘색) */
	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Color")
	FLinearColor SelectedColor = FLinearColor(0.3f, 0.85f, 0.95f, 1.0f);

	/** 마우스를 올린 지역 밝기 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Color", meta = (ClampMin = "1.0"))
	float HoverBrightness = 1.25f;

	/** 고른 지역을 들어 올리는 높이(px, 지도 원본 크기 기준) */
	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Lift", meta = (ClampMin = "0.0"))
	float LiftOffset = 10.0f;

	/** 들어 올린 지역 그림자 색 */
	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Lift")
	FLinearColor ShadowColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.65f);

	/** 벽 스테이지 테두리 색 */
	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Wall")
	FLinearColor WallRimColor;

	/** 벽 테두리 층 크기 (지역 그림 대비 배율, 가운데 기준) */
	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Wall", meta = (ClampMin = "1.0"))
	float WallRimScale = 1.04f;

	/** 벽 스테이지 지형에 섞는 색과 비율 */
	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Wall")
	FLinearColor WallTint;

	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Wall", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WallBlend = 0.45f;

	/** 정보 카드 "위험" 줄. {0} = 체력 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Text")
	FText WallInfoFormat;

	/** 지역 번호 글꼴 (외곽선 포함) */
	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Label")
	FSlateFontInfo LabelFont;

	/** 지역 안에 맞출 때 남기는 여유 (1 = 경계에 딱 붙음, 외곽선 두께만큼 줄인다) */
	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Label", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float LabelFitMargin = 0.85f;

	/** 줄여도 이보다 작게는 안 한다 (읽을 수 있는 크기) */
	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Label", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float MinLabelScale = 0.45f;

	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Label")
	FLinearColor ClearedLabelColor = FLinearColor(0.35f, 0.7f, 0.75f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Label")
	FLinearColor OpenLabelColor = FLinearColor(0.85f, 0.97f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Label")
	FLinearColor SelectedLabelColor = FLinearColor(0.002f, 0.008f, 0.012f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Label")
	FLinearColor LockedLabelColor = FLinearColor(0.07f, 0.08f, 0.1f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Label")
	FLinearColor BossLabelColor = FLinearColor(1.0f, 0.4f, 0.28f, 1.0f);

	/** 지역 글자. {0} = 번호 */
	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Text")
	FText RegionLabelFormat;

	/** 깬 지역 글자. {0} = 번호 */
	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Text")
	FText ClearedLabelFormat;

	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Text")
	FText BossLabel;

	/** 정보 카드 제목. {0} = 번호, {1} = 지역 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Text")
	FText StageNameFormat;

	/** 정보 카드 본문. {0} = 적 이름들, {1} = 목표 */
	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Text")
	FText StageInfoFormat;

	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Text")
	FText SurviveObjective;

	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Text")
	FText BossObjective;

	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Text")
	FText ClearedSuffix;

	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Text")
	FText LockedText;

	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Text")
	FText BossLockedText;

	/** 진행 줄. {0} = 깬 수, {1} = 전체 */
	UPROPERTY(EditDefaultsOnly, Category = "Stage Map|Text")
	FText ProgressFormat;

	/** 지도 영역 (원본 비율로 늘어나는 캔버스) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> MapLayer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StageNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StageInfoText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SquadButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ZoneNameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ProgressText;

	/** 잠긴 지역 안내 (글자) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LockedNotice;

	/** 벽 스테이지 "위험" 줄 (빨강). 없으면 본문 끝에 붙인다 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> WallText;

private:
	UPROPERTY()
	TObjectPtr<const UUSZoneData> Zone;

	/** 바탕·지역·그림자·글자 (Zone의 Stages와 같은 인덱스) */
	UPROPERTY()
	TObjectPtr<UImage> BackgroundImage;

	UPROPERTY()
	TArray<TObjectPtr<UImage>> RegionImages;

	UPROPERTY()
	TArray<TObjectPtr<UImage>> ShadowImages;

	/** 벽 테두리 층 (벽이 아닌 지역은 nullptr) */
	UPROPERTY()
	TArray<TObjectPtr<UImage>> WallRimImages;

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> Labels;

	/** 그림을 만든 구역 (같은 구역이면 다시 만들지 않는다) */
	UPROPERTY()
	TObjectPtr<const UUSZoneData> BuiltZone;

	int32 SelectedIndex = INDEX_NONE;
	int32 HoveredIndex = INDEX_NONE;
};
