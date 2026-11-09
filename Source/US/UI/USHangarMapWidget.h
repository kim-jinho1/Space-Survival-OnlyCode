#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USHangarMapWidget.generated.h"

class UButton;
class UCanvasPanel;
class UPanelWidget;
class UTextBlock;
class UTexture2D;
class UUSMapOrbitWidget;
class UUSHangarOptionWidget;
class UUSZoneData;
class UWidget;

/** 출격 지도에서 구역을 정하고 "편대 구성"을 누름 */
DECLARE_DELEGATE_OneParam(FDelegateOnSquadRequested, const UUSZoneData*);
/** 출격 지도·편대 화면의 "뒤로" */
DECLARE_DELEGATE(FDelegateOnBackRequested);

/**
 * 격납고 흐름 2단계 출격 지도 (Into the Breach 섬 지도를 본뜸): 구역마다 섬 하나를 지도 위치에 띄우고,
 * 고른 섬은 크게, 잠긴 섬은 흐리게 + "정식판". 섬은 구역 데이터(UUSZoneData)대로 런타임에 만들므로 구역 추가는 데이터 작업만으로 끝난다.
 * 섬을 고르면 지도 전체(IslandLayer)가 그 섬 쪽으로 확대·이동해 고른 섬을 늘 같은 자리(FocusPoint)로 데려오고,
 * 그 옆 고정 자리의 정보 카드(ZoneCard)에 이름·설명·출몰하는 적·목표를 보인다. 잠긴 섬도 다가가서 보여 주되
 * 적·목표 대신 "정식판에서 열립니다"(LockedNotice)를 보이고 "아군 편성"을 끈다.
 * 섬 층 맨 아래에는 궤도 그림(UUSMapOrbitWidget)을 깔아 가운데 모함(OrbitCenter)에서 각 구역을 지나는 궤도를 그린다(B안 궤도 지도, 멘토링 2026-10-16).
 * 확대는 3D 카메라가 아니라 IslandLayer 렌더 변환(크기·위치) 보간이다 — 레이아웃(섬 위치)은 그대로라 섬 클릭 판정도 같이 따라간다.
 * 처음엔 아무 구역도 고르지 않고("아군 편성" 꺼짐) 사용자가 고른다. Esc나 빈 곳 클릭은 선택을 풀고 전체 지도로, 전체 지도에서 Esc는 격납고로.
 * 위젯 이름: IslandLayer(CanvasPanel), ZoneNameText, ZoneDescriptionText, EnemyIconRow(HorizontalBox), EnemyNamesText,
 * ObjectiveText, SquadButton, BackButton, (선택) ZoneCard, ZoneDetailBox, LockedNotice, Vignette.
 */
UCLASS()
class US_API UUSHangarMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSHangarMapWidget(const FObjectInitializer& ObjectInitializer);

	/** 구역 목록으로 섬을 만든다. 처음엔 아무 구역도 고르지 않는다 (사용자가 직접 고른다) */
	void SetupMap(const TArray<TObjectPtr<UUSZoneData>>& InZones);

	/**
	 * 지도 화면을 연다: 전체 지도에서 시작하고 키보드 포커스(Esc)를 가져온다.
	 * bKeepSelection이면(편성에서 돌아옴) 고르던 구역으로 다시 다가가고, 아니면(격납고에서 들어옴) 선택을 비운다.
	 */
	void ShowMap(bool bKeepSelection);

	/** "편대 구성" 클릭 (고른 구역) */
	FDelegateOnSquadRequested OnSquadRequested;

	/** "격납고" 클릭 */
	FDelegateOnBackRequested OnBackRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 섬 클릭 (구역 인덱스). 잠긴 섬도 고른다 (다가가서 "정식판에서 열립니다") */
	void HandleIslandClicked(int32 ZoneIndex);

	/** ZoneIndex 구역을 고르고 섬 크기·정보 카드를 맞추고 그 구역으로 다가간다 */
	void SelectZone(int32 ZoneIndex);

	/** 선택을 풀고 전체 지도로 물러난다 (Esc·빈 곳 클릭). 정보 카드 글자는 사라지는 동안 그대로 둔다 */
	void ClearSelection();

	/** 고른 구역으로 다가간다 (bFocus) / 전체 지도로 물러난다. 지금 모습에서 목표까지 FocusDuration 동안 보간 */
	void StartZoom(bool bFocus);

	/** 확대 정도(Zoom)대로 IslandLayer 렌더 변환과 정보 카드·둘레 어둡게의 불투명도를 맞춘다 */
	void ApplyZoom();

	/** 다가간 상태인가 (다가가는 중 포함) */
	bool IsFocused() const { return bTargetFocused; }

	/** 섬들의 크기·흐림을 지금 선택에 맞춘다 */
	void RefreshIslands();

	/** 정보 상자를 고른 구역으로 다시 쓴다 */
	void RefreshInfo();

	UFUNCTION()
	void HandleSquadClicked();

	UFUNCTION()
	void HandleBackClicked();

	/** 섬 위젯 클래스 (WBP_USHangarIsland: 버튼 + 섬 그림 OptionIcon + 이름 OptionText + 잠김 글자 SubText) */
	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TSubclassOf<UUSHangarOptionWidget> IslandClass;

	/** 섬 하나의 크기(px, 1080p 기준 DPI 배율 전) */
	UPROPERTY(EditDefaultsOnly, Category = "Map")
	FVector2D IslandSize = FVector2D(300.0f, 225.0f);

	/** 궤도 중심 = 모함 자리 (지도 영역 비율 0~1) */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Orbit")
	FVector2D OrbitCenter = FVector2D(0.5f, 0.5f);

	/** 궤도 타원의 세로/가로 비율 (기울어진 원반처럼 보이게, 1 = 원) */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Orbit", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float OrbitAspect = 0.42f;

	/** 열린 구역 궤도 색 */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Orbit")
	FLinearColor OrbitColor = FLinearColor(0.03f, 0.12f, 0.2f, 1.0f);

	/** 고른 구역 궤도 색 */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Orbit")
	FLinearColor SelectedOrbitColor = FLinearColor(0.08f, 0.62f, 0.56f, 1.0f);

	/** 잠긴 구역 궤도 색 (점선) */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Orbit")
	FLinearColor LockedOrbitColor = FLinearColor(0.025f, 0.05f, 0.08f, 1.0f);

	/** 궤도 선 굵기(px) */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Orbit", meta = (ClampMin = "0.5"))
	float OrbitThickness = 1.5f;

	/** 가운데 모함 그림 (없으면 궤도만) */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Orbit")
	TObjectPtr<UTexture2D> HubTexture;

	/** 모함 그림 크기(px) */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Orbit")
	FVector2D HubSize = FVector2D(110.0f, 110.0f);

	/** 다가갔을 때 지도 확대 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Focus", meta = (ClampMin = "1.0"))
	float FocusScale = 1.8f;

	/** 다가갔을 때 고른 섬이 올 자리 (지도 영역 비율 0~1). 정보 카드는 WBP에서 이 자리 오른쪽에 고정해 둔다 */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Focus")
	FVector2D FocusPoint = FVector2D(0.30f, 0.52f);

	/** 다가가기·물러나기 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Focus", meta = (ClampMin = "0.01"))
	float FocusDuration = 0.6f;

	/** 고른 섬 확대 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "Map", meta = (ClampMin = "1.0"))
	float SelectedIslandScale = 1.15f;

	/** 잠긴 섬 불투명도 */
	UPROPERTY(EditDefaultsOnly, Category = "Map", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LockedIslandOpacity = 0.45f;

	/** 적 그림 칸 크기(px) */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Enemy", meta = (ClampMin = "8.0"))
	float EnemyIconSize = 24.0f;

	/** 적 그림 칸 사이 간격(px) */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Enemy", meta = (ClampMin = "0.0"))
	float EnemyIconSpacing = 6.0f;

	/** 적 그림 색 (흰색 그림에 곱한다) */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Enemy")
	FLinearColor EnemyIconColor = FLinearColor(0.86f, 0.15f, 0.15f, 1.0f);

	/** 적 그림 바탕 칸 색 */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Enemy")
	FLinearColor EnemyIconBackgroundColor = FLinearColor(0.04f, 0.006f, 0.006f, 1.0f);

	/** 잠긴 섬 아래 글자 */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Text")
	FText LockedIslandText;

	/** 적 줄 사이 구분자 (한 줄에 한 종류) */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Text")
	FText EnemyNameSeparator;

	/** 특징이 있는 적의 한 줄. {0} = 이름, {1} = 특징 (R-41 5: 적 설명은 판 밖, 멈춰 있을 때 읽는다) */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Text")
	FText EnemyLineFormat = NSLOCTEXT("USHangarMapWidget", "EnemyLineFormat", "{0} — {1}");

	/** 섬을 놓는 지도 영역. 구역 MapPosition (0~1)이 이 영역 비율이다 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> IslandLayer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ZoneNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ZoneDescriptionText;

	/** 적 그림 줄 (HorizontalBox). 칸은 C++이 만든다 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> EnemyIconRow;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EnemyNamesText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ObjectiveText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SquadButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackButton;

	/** 정보 카드 (다가갈수록 나타난다) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ZoneCard;

	/** 정보 카드의 적·목표 묶음 (잠긴 구역이면 숨긴다) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ZoneDetailBox;

	/** "정식판에서 열립니다" (잠긴 구역일 때만) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> LockedNotice;

	/** 보스 줄 ("보스 · 모선"). 구역에 보스가 없으면 숨긴다 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BossText;

	/** 보스 줄 문구. {Boss} = 보스 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "Map|Info")
	FText BossFormat = NSLOCTEXT("USHangarMapWidget", "BossFormat", "보스 · {Boss}");

	/** 둘레를 어둡게 하는 그림 (다가갈수록 짙어진다) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Vignette;

private:
	UPROPERTY()
	TArray<TObjectPtr<UUSZoneData>> Zones;

	/** 섬 위젯 (Zones와 같은 인덱스, 구역이 비면 nullptr) */
	UPROPERTY()
	TArray<TObjectPtr<UUSHangarOptionWidget>> Islands;

	/** 섬 층 맨 아래 궤도 그림 (SetupMap에서 만든다) */
	UPROPERTY()
	TObjectPtr<UUSMapOrbitWidget> OrbitWidget;

	/** 고른 구역 인덱스 */
	int32 SelectedZone = INDEX_NONE;

	/** 지금 확대 상태. Scale = 배율, Offset = 이동(지도 영역 비율이라 창 크기와 상관없다). 전체 지도 = 1, (0, 0) */
	float CurrentScale = 1.0f;
	FVector2D CurrentOffset = FVector2D::ZeroVector;

	/** 보간 시작·목표 */
	float StartScale = 1.0f;
	FVector2D StartOffset = FVector2D::ZeroVector;
	float TargetScale = 1.0f;
	FVector2D TargetOffset = FVector2D::ZeroVector;

	/** 보간 진행(0~1). 1이면 멈춰 있다 */
	float ZoomAlpha = 1.0f;

	/** 목표가 다가간 상태인가 */
	bool bTargetFocused = false;

	/** 마지막으로 렌더 변환을 맞춘 지도 영역 크기 (창 크기가 바뀌면 멈춰 있어도 다시 맞춘다) */
	FVector2D AppliedLayerSize = FVector2D::ZeroVector;
};
