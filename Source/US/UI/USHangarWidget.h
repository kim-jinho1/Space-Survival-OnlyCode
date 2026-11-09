#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USHangarWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class UTexture2D;
class UWidgetSwitcher;
class UUSAllyTypeData;
class UUSHangarCraftPanelWidget;
class UUSHangarMapWidget;
class UUSHangarOptionWidget;
class UUSHangarSquadWidget;
class UUSMetaProgressionSubsystem;
class UUSMetaTreeData;
class UUSStageData;
class UUSStageMapWidget;
class UUSZoneData;

/** 격납고에서 버튼을 누름 */
DECLARE_DELEGATE_OneParam(FDelegateOnLaunchRequested, const UUSZoneData*);
DECLARE_DELEGATE(FDelegateOnHangarTitleRequested);
DECLARE_DELEGATE_OneParam(FDelegateOnCraftSelectRequested, int32);

/** 격납고 흐름의 화면 (ScreenSwitcher 자식 순서와 같다) */
UENUM()
enum class EUSHangarScreen : uint8
{
	/** 1단계: 기체 강화 (설계도 + 강화 카드) */
	Hangar,
	/** 2단계: 출격 지도 */
	Map,
	/** 3단계: 편대 */
	Squad,
	/** 2-2단계: 행성 지역 지도 (스테이지가 있는 구역만, 지도 → 지역 지도 → 편대) */
	Stage
};

/**
 * 격납고 화면 UI. 흐름은 격납고(강화) → 출격 지도 → 편대 → 출격 세 화면을 ScreenSwitcher로 넘기고, 단계 탭 없이 버튼으로 들어가고
 * 각 화면 왼쪽 위 버튼으로 한 단계 뒤로 간다 (레퍼런스: Deep Rock Galactic 장비 터미널 / Into the Breach 섬 지도 / Helldivers 2 장비 카드).
 * 이 위젯은 1단계 화면 요소(스크랩 칩, 위쪽 기종 아이콘 줄 CraftList, 강화 패널 CraftPanel = 설계도 WBP_USHangarBlueprint, "출격 지도"·"타이틀로")와 화면 전환만 맡고,
 * 지도·편대는 자식 위젯(UUSHangarMapWidget, UUSHangarSquadWidget)이 맡는다.
 * 기체 고르기는 컨트롤러 한 곳(SelectCraft)을 거친다: 아이콘 클릭은 OnCraftSelectRequested로 올려 보내고, 컨트롤러가 ShowCraft로 내려보낸다.
 * 값은 메타 진행이 바뀔 때(OnMetaChanged)만 다시 쓴다.
 * 위젯 이름: ScreenSwitcher, ScrapText, CraftList, CraftPanel, MapButton, TitleButton, MapPanel, SquadPanel.
 */
UCLASS()
class US_API UUSHangarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSHangarWidget(const FObjectInitializer& ObjectInitializer);

	/** 트리·구역 목록을 받아 강화 패널·출격 지도·편대 화면을 준비한다 */
	void SetupHangar(const UUSMetaTreeData* InTree, const TArray<TObjectPtr<UUSZoneData>>& InZones);

	/** OwnerType(nullptr = 대장기) 기체 패널을 연다 */
	void ShowCraft(const UUSAllyTypeData* OwnerType);

	/** 기종 아이콘 줄을 왼쪽부터 채운다. Types[i]가 i번 기체의 기종 (nullptr = 대장기) */
	void SetupCraftList(const TArray<const UUSAllyTypeData*>& Types);

	/** 기종 아이콘 줄에서 CraftIndex를 강조한다 */
	void SetSelectedCraft(int32 CraftIndex);

	/** 스테이지를 깨고 돌아왔을 때: "출격"이 출격 지도 대신 Zone 지역 지도로 바로 간다 (다음 스테이지가 골라져 있다). nullptr = 평소대로 */
	void SetResumeZone(const UUSZoneData* Zone);

	/** 편대 화면의 "출격" 클릭 (지도에서 고른 구역) */
	FDelegateOnLaunchRequested OnLaunchRequested;

	/** "타이틀로" 클릭 */
	FDelegateOnHangarTitleRequested OnTitleRequested;

	/** 기종 아이콘 클릭 → 컨트롤러가 그 기체를 고른다 (기체 인덱스) */
	FDelegateOnCraftSelectRequested OnCraftSelectRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 기종 아이콘 클릭 */
	void HandleCraftOptionClicked(int32 CraftIndex);

	/** 기종 아이콘들을 지금 잠김·선택 상태로 다시 쓴다 */
	void RefreshCraftList();

	/** 기종 아이콘 아래 줄: 해금했으면 "강화 3/15"(그 기종 패널 칸들의 단계 합), 잠겼으면 "잠김 · 해금 가격" (WBP에 SubText가 있을 때만 보인다) */
	FText GetCraftSubLabel(const UUSAllyTypeData* Type, bool bLocked) const;

	/** 기종 그림 (nullptr = 대장기 → LeaderIcon, 잠김 → LockedCraftIcon) */
	UTexture2D* GetCraftIcon(const UUSAllyTypeData* Type, bool bLocked) const;

	/** 스크랩 칩·기종 아이콘 다시 쓰기 (OnMetaChanged) */
	void HandleMetaChanged();

	/** Screen 화면으로 넘긴다 */
	void ShowScreen(EUSHangarScreen Screen);

	/** 출격 지도 화면으로 넘기고 지도를 연다 (UUSHangarMapWidget::ShowMap) */
	void ShowMapScreen(bool bKeepSelection);

	/** 지도 "편대 구성" → 편대 화면 */
	void HandleSquadRequested(const UUSZoneData* Zone);

	/** 지역 지도 "아군 편성" → 편대 화면 (이번 출격 스테이지를 메타 진행에 기록) */
	void HandleStageSquadRequested(const UUSZoneData* Zone, const UUSStageData* Stage);

	/** 지역 지도 "출격 지도" → 2단계 (고르던 행성 그대로) */
	void HandleStageBackRequested();

	/** 편대 화면을 Zone으로 연다 */
	void ShowSquadScreen(const UUSZoneData* Zone);

	/** 지도 "격납고" → 1단계 */
	void HandleMapBackRequested();

	/** 편대 "지도" → 2단계 */
	void HandleSquadBackRequested();

	/** 편대 "출격" → 컨트롤러로 */
	void HandleSquadLaunchRequested(const UUSZoneData* Zone);

	UFUNCTION()
	void HandleMapClicked();

	UFUNCTION()
	void HandleTitleClicked();

	/** 메타 진행 서브시스템 (없으면 nullptr) */
	UUSMetaProgressionSubsystem* GetMetaSubsystem() const;

	/** 위쪽 기종 아이콘 클래스: 아이콘 + 이름 (WBP_USHangarCraftOption) */
	UPROPERTY(EditDefaultsOnly, Category = "Hangar")
	TSubclassOf<UUSHangarOptionWidget> CraftOptionClass;

	/** 대장기 그림 (대장기는 기종 데이터가 없어 여기 둔다) */
	UPROPERTY(EditDefaultsOnly, Category = "Hangar")
	TObjectPtr<UTexture2D> LeaderIcon;

	/** 잠긴 기종 아이콘 그림 (자물쇠) */
	UPROPERTY(EditDefaultsOnly, Category = "Hangar")
	TObjectPtr<UTexture2D> LockedCraftIcon;

	/** 대장기 패널 제목·설명 */
	UPROPERTY(EditDefaultsOnly, Category = "Hangar|Text")
	FText LeaderTitle;

	UPROPERTY(EditDefaultsOnly, Category = "Hangar|Text")
	FText LeaderDescription;

	/** 기종 아이콘 아래 줄 (해금됨). {0} = 강화 단계 합, {1} = 최대 단계 합 */
	UPROPERTY(EditDefaultsOnly, Category = "Hangar|Text")
	FText CraftUpgradeFormat;

	/** 기종 아이콘 아래 줄 (잠김). {0} = 해금 가격(스크랩) */
	UPROPERTY(EditDefaultsOnly, Category = "Hangar|Text")
	FText LockedCraftSubFormat;

	/** 0 = 격납고, 1 = 출격 지도, 2 = 편대, 3 = 지역 지도 (EUSHangarScreen 순서) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidgetSwitcher> ScreenSwitcher;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ScrapText;

	/** 위쪽 기종 아이콘 줄 (HorizontalBox, 칸은 CraftOptionClass) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> CraftList;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUSHangarCraftPanelWidget> CraftPanel;

	/** "출격 지도" */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MapButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TitleButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUSHangarMapWidget> MapPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUSHangarSquadWidget> SquadPanel;

	/** 지역 지도 (ScreenSwitcher 4번째). 없으면 예전처럼 지도 → 편대 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UUSStageMapWidget> StageMapPanel;

private:
	UPROPERTY()
	TObjectPtr<const UUSMetaTreeData> Tree;

	/** 기종 아이콘에 띄운 기종 (기체 인덱스 순, nullptr = 대장기) */
	UPROPERTY()
	TArray<TObjectPtr<const UUSAllyTypeData>> CraftTypes;

	/** 기종 아이콘 위젯 (CraftTypes와 같은 인덱스) */
	UPROPERTY()
	TArray<TObjectPtr<UUSHangarOptionWidget>> CraftOptions;

	/** 기종 아이콘에서 강조한 기체 인덱스 */
	int32 SelectedCraft = INDEX_NONE;

	/** 지도에서 고른 구역 (편대에서 "뒤로"가 지역 지도로 갈지 정한다) */
	UPROPERTY()
	TObjectPtr<const UUSZoneData> SortieZone;

	/** 이어 하던 행성 (SetResumeZone). 스테이지가 있는 구역만 */
	UPROPERTY()
	TObjectPtr<const UUSZoneData> ResumeZone;

	/** OnMetaChanged 구독 핸들 (NativeDestruct에서 해제) */
	FDelegateHandle MetaChangedHandle;
};
