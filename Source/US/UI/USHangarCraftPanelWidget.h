#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USHangarCraftPanelWidget.generated.h"

class UPanelWidget;
class UTextBlock;
class USoundBase;
class UUSAllyTypeData;
class UUSMetaNodeEntryWidget;
class UUSMetaProgressionSubsystem;
class UUSMetaTreeData;
class UUSMetaUpgradeData;

/**
 * 격납고 강화 패널 (Deep Rock Galactic 장비 터미널을 본뜸): 고른 기체 이름 + 그 기체의 강화 칸들.
 * 칸은 그 기체에 붙은 영구 강화 칸(UUSMetaUpgradeData::OwnerAllyType)을 트리 순서대로 하나씩 — 이름, 단계 칸, 가격 칩.
 * 아직 해금하지 않은 기종은 그 기종의 칸이 해금 칸 하나뿐이라 자연스럽게 "해금" 칸만 보인다.
 * 칸을 누르면 구매를 시도하고 결과음을 낸다.
 * 칸을 어디에 놓을지는 AddNodeEntry가 정한다: 이 클래스는 NodeList에 차례로 쌓고, 설계도 화면(UUSHangarBlueprintWidget)은 도면 좌우에 나눈다.
 * 위젯 이름: TitleText, (선택) NodeList(VerticalBox), DescriptionText.
 */
UCLASS()
class US_API UUSHangarCraftPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** OwnerType(nullptr = 대장기) 기체의 칸들로 패널을 채운다 */
	virtual void ShowCraft(const UUSMetaTreeData* Tree, const UUSAllyTypeData* OwnerType, const FText& Title, const FText& Description);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 칸들을 지금 상태로 다시 그린다 (OnMetaChanged) */
	virtual void HandleMetaChanged();

	/** 만든 칸 위젯을 화면에 놓는다 (기본: NodeList 끝에 추가) */
	virtual void AddNodeEntry(UUSMetaNodeEntryWidget* Entry, const UUSMetaUpgradeData* Node);

	/** 놓았던 칸 위젯을 모두 치운다 (기본: NodeList 비우기) */
	virtual void ClearNodeEntries();

	/** 칸 클릭 → 구매 시도, 결과음 */
	void HandleNodeClicked(const UUSMetaUpgradeData* Node);

	/** 메타 진행 서브시스템 (없으면 nullptr) */
	UUSMetaProgressionSubsystem* GetMetaSubsystem() const;

	/** 칸 한 줄 위젯 클래스 (WBP_USMetaNodeEntry) */
	UPROPERTY(EditDefaultsOnly, Category = "Panel")
	TSubclassOf<UUSMetaNodeEntryWidget> NodeEntryClass;

	/** 구매 성공음 */
	UPROPERTY(EditDefaultsOnly, Category = "Panel|Sound")
	TObjectPtr<USoundBase> BuySound;

	/** 구매 실패음 (스크랩 부족·잠김) */
	UPROPERTY(EditDefaultsOnly, Category = "Panel|Sound")
	TObjectPtr<USoundBase> DeniedSound;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	/** 기체 설명. 목업에는 없어 WBP에서 뺐다 (넣으면 다시 채워진다) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

	/** 칸을 쌓을 세로 상자. AddNodeEntry를 덮어쓴 자식 클래스는 없어도 된다 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> NodeList;

	/** 지금 보여 주는 칸 위젯들 */
	UPROPERTY()
	TArray<TObjectPtr<UUSMetaNodeEntryWidget>> NodeEntries;

private:
	/** OnMetaChanged 구독 핸들 (NativeDestruct에서 해제) */
	FDelegateHandle MetaChangedHandle;
};
