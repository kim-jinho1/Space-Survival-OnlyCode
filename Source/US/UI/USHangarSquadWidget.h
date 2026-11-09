#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Datas/USAllyTypeData.h"
#include "UI/USHangarMapWidget.h"
#include "USHangarSquadWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class UTexture2D;
class USoundBase;
class UUSHangarOptionWidget;
class UUSMetaProgressionSubsystem;
class UUSMetaTreeData;
class UUSZoneData;

/** 편대 화면에서 "출격"을 누름 (지도에서 고른 구역) */
DECLARE_DELEGATE_OneParam(FDelegateOnSquadLaunchRequested, const UUSZoneData*);

/**
 * 격납고 흐름 3단계 아군 편성 (Into the Breach 편대 고르기를 본뜬 2단): 왼쪽 = 자리 목록(내 전투기 고정 + 편성 칸마다 한 줄 + 다음에 열 칸은 잠긴 줄),
 * 오른쪽 = 고른 자리에 넣을 기종 카드(큰 그림 + 이름 + 역할 + 한 줄 설명)와 "이 자리 비우기". 고르는 칸은 늘 펼쳐져 있고
 * 고른 자리는 처음 1번, 자리 줄을 누르면 바뀐다. 기종을 누르면 바로 저장된다(UUSMetaProgressionSubsystem::SetLoadoutSlot).
 * 값은 메타 진행이 바뀔 때(OnMetaChanged)만 다시 쓴다.
 * 키보드: ↑/↓(W/S) 자리, ←/→(A/D) 기종, Enter/Space 넣기, Delete/Backspace 비우기, Esc 지도로 (게임패드 십자키·A·X·B).
 * 기종 칸의 키보드 테두리는 키를 누른 뒤에만 보이고, 마우스로 누르면 숨긴다 (마우스만 쓰는 사람에게는 안 보이게).
 * 위젯 이름: CardList(VerticalBox), PickerList, PickerTitleText, ClearButton, ZoneText, CountText, LaunchButton, BackButton.
 */
UCLASS()
class US_API UUSHangarSquadWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSHangarSquadWidget(const FObjectInitializer& ObjectInitializer);

	/** 트리(편성 칸 수·기종 후보)와 대장 카드 그림·이름을 받는다 */
	void SetupSquad(const UUSMetaTreeData* InTree, UTexture2D* InLeaderIcon, const FText& InLeaderName);

	/** Zone으로 출격하려고 화면을 연다: 구역 이름을 쓰고 카드를 다시 그린다 */
	void ShowForZone(const UUSZoneData* Zone);

	/** "출격" 클릭 */
	FDelegateOnSquadLaunchRequested OnLaunchRequested;

	/** "지도" 클릭 */
	FDelegateOnBackRequested OnBackRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** ↑/↓: 고른 자리를 Delta만큼 옮긴다 (칸 끝에서 멈춤). 기종 커서는 새 자리의 기종으로 */
	void MovePickingSlot(int32 Delta);

	/** ←/→: 기종 커서를 Delta만큼 옮긴다 (끝에서 멈춤) */
	void MovePickerCursor(int32 Delta);

	/** 기종 커서를 고른 자리에 지금 들어 있는 기종으로 (빈 자리면 첫 기종) */
	void ResetPickerCursor();

	/** 기종 카드의 키보드 테두리를 커서에 맞춰 다시 칠한다 */
	void RefreshPickerCursor();

	/** 자리 목록·기종 카드를 지금 편성으로 다시 그린다 (OnMetaChanged) */
	void HandleMetaChanged();

	/** 자리 목록과 기종 카드를 다시 만든다 */
	void Refresh();

	/** 왼쪽 자리 목록을 다시 만든다 */
	void RefreshCards();

	/** 오른쪽 기종 카드·제목·비우기 버튼을 고른 자리에 맞춰 다시 만든다 */
	void RefreshPicker();

	/** 자리 줄 클릭: 그 자리를 고른다. 잠긴 칸이면 거절음, 내 전투기 줄은 무시 */
	void HandleCardClicked(int32 CardIndex);

	/** 기종 카드 클릭: 커서를 옮기고 고른 자리에 넣는다 */
	void HandlePickerClicked(int32 OptionIndex);

	/** OptionIndex 기종을 고른 자리에 넣는다 (잠긴 기종은 거절음). 클릭·Enter가 같이 쓴다 */
	void PlaceType(int32 OptionIndex);

	/** "이 자리 비우기" */
	UFUNCTION()
	void HandleClearClicked();

	/** 편성에 넣을 수 있는 기종 후보 (기본 기종 + 해금 칸의 기종, 중복 없이) */
	void GetCandidates(TArray<const UUSAllyTypeData*>& OutTypes) const;

	/** 기종 역할 글자 (없으면 빈 글자) */
	FText GetRoleText(const UUSAllyTypeData* Type) const;

	/** 거절음 */
	void PlayDenied() const;

	UFUNCTION()
	void HandleLaunchClicked();

	UFUNCTION()
	void HandleBackClicked();

	/** 메타 진행 서브시스템 (없으면 nullptr) */
	UUSMetaProgressionSubsystem* GetMetaSubsystem() const;

	/** 자리 줄 클래스 (WBP_USHangarSquadCard: 자리 번호 CornerText, 그림 OptionIcon, 이름 OptionText, 역할 SubText) */
	UPROPERTY(EditDefaultsOnly, Category = "Squad")
	TSubclassOf<UUSHangarOptionWidget> CardClass;

	/** 기종 카드 클래스 (WBP_USHangarCraftCard: 큰 그림 OptionIcon, 이름 OptionText, 역할 CornerText, 한 줄 설명 SubText) */
	UPROPERTY(EditDefaultsOnly, Category = "Squad")
	TSubclassOf<UUSHangarOptionWidget> PickerOptionClass;

	/** 빈 칸 그림 ("+") */
	UPROPERTY(EditDefaultsOnly, Category = "Squad")
	TObjectPtr<UTexture2D> EmptySlotIcon;

	/** 잠긴 칸 그림 (자물쇠) */
	UPROPERTY(EditDefaultsOnly, Category = "Squad")
	TObjectPtr<UTexture2D> LockedSlotIcon;

	/** 대장 카드 버튼 색 (다른 칸과 구분되는 주황) */
	UPROPERTY(EditDefaultsOnly, Category = "Squad")
	FLinearColor LeaderCardColor = FLinearColor(0.75f, 0.37f, 0.03f, 1.0f);

	/** 역할별 카드 아래 글자 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Text")
	TMap<EUSAllyRole, FText> RoleTexts;

	/** 내 전투기 줄 아래 글자 (자리 번호 대신) */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Text")
	FText LeaderSubText;

	/** 빈 칸 카드 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Text")
	FText EmptySlotText;

	/** 고르는 중인 칸 카드 아래 글자 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Text")
	FText PickingSubText;

	/** 잠긴 칸 카드 아래 글자 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Text")
	FText LockedSlotSubText;

	/** 기종 카드 위 제목. {0} = 자리 번호 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Text")
	FText PickerTitleFormat;

	/** 머리 줄 대수. {0} = 채운 칸, {1} = 칸 수 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Text")
	FText CountFormat;

	/** 거절음 (잠긴 칸·기종) */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Sound")
	TObjectPtr<USoundBase> DeniedSound;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> CardList;

	/** 기종 카드 줄. 카드는 C++이 만든다 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> PickerList;

	/** "N번 자리에 넣을 전투기" */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PickerTitleText;

	/** "이 자리 비우기" (이미 빈 자리면 꺼짐) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ClearButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ZoneText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> LaunchButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackButton;

private:
	UPROPERTY()
	TObjectPtr<const UUSMetaTreeData> Tree;

	UPROPERTY()
	TObjectPtr<UTexture2D> LeaderIcon;

	FText LeaderName;

	/** 지금 출격하려는 구역 */
	UPROPERTY()
	TObjectPtr<const UUSZoneData> Zone;

	/** 기종 카드에 띄운 기종 (카드 번호 = 배열 인덱스) */
	UPROPERTY()
	TArray<TObjectPtr<const UUSAllyTypeData>> PickerTypes;

	/** 기종 카드가 바꿀 자리 (편성 칸이 없으면 INDEX_NONE) */
	int32 PickingSlot = 0;

	/** 키보드로 고른 기종 카드 (PickerTypes 인덱스) */
	int32 PickerCursor = 0;

	/** 키보드 테두리를 보일지 (키를 누르면 켜고 마우스로 누르면 끈다) */
	bool bShowKeyCursor = false;

	/** OnMetaChanged 구독 핸들 (NativeDestruct에서 해제) */
	FDelegateHandle MetaChangedHandle;
};
