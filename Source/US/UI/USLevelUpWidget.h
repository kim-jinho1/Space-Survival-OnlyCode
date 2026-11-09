#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Progression/USLevelUpCard.h"
#include "USLevelUpWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UPanelWidget;
class UTextBlock;
class USoundBase;

/** 카드 선택 알림 (선택된 옵션) */
DECLARE_DELEGATE_OneParam(FDelagateOnCardSelected, const FUSLevelUpCard&);

/** 다시 뽑기 요청 (횟수 확인·새 카드는 조정자가 한다) */
DECLARE_DELEGATE(FDelegateOnRerollRequested);

/**
 * 레벨업 카드 3장 선택 위젯 (능력·보급 카드).
 * 카드 버튼·제목·설명·아이콘을 WBP에서 이름으로 연결(BindWidget)하고, C++이 글자를 채우고 클릭을 받는다.
 * 위젯 이름: Card0Button~Card2Button, Card0Title~, Card0Desc~, (선택) Card0Icon~, Card0Tag~, Card0Pips~, Card0Detail~.
 * 1·2·3 키(숫자패드 포함)로도 고른다 — 컨트롤러가 창을 띄운 뒤 키보드 포커스를 준다.
 * 칩(CardNTag)이 있으면 제목 칸에는 능력 이름만, 없으면 한 줄 제목("기관포기 · 확산 사격 1단계")을 쓴다.
 * 단계 점(CardNPips, HorizontalBox)은 될 단계까지 채운다 — 고르면 이렇게 된다는 뜻.
 * 빌드 (목업 B): 카드 위 색 띠(CardNBand 테두리 + CardNBandText·CardNBandIcon)에 빌드 이름, 칩(CardNTag)엔 기체 이름.
 * 화면 위 진행 줄은 BuildNName·BuildNPips(N = 0~2, UUSUpgradeComponent::Builds 순서)에 빌드 색으로 채운다.
 * 다시 뽑기 (목업 A, 카드 아래 가운데): RerollButton + RerollText("다시 뽑기 R · 남은 N"), R 키. 남은 횟수가 0이면 버튼을 숨긴다.
 */
UCLASS()
class US_API UUSLevelUpWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSLevelUpWidget(const FObjectInitializer& ObjectInitializer);

	/** 표시할 카드 옵션 설정 (조정자가 호출). 옵션이 3개보다 적으면 남는 카드는 숨긴다 */
	void SetupCards(const TArray<FUSLevelUpCard>& InOptions);

	/** 화면 위 빌드 진행 줄을 채운다. 진행이 3개보다 적으면 남는 칸은 숨긴다 */
	void SetupBuildProgress(const TArray<FUSBuildProgress>& Progress);

	/** Index 카드 선택 (버튼 클릭에서 호출) */
	UFUNCTION(BlueprintCallable, Category = LevelUp)
	void SelectCard(int32 Index);

	/** 남은 다시 뽑기 횟수를 보여 준다 (0이면 버튼을 숨기고 R 키도 받지 않는다) */
	void SetRerollsLeft(int32 Count);

	/** 다시 뽑기 (버튼 클릭·R 키에서 호출) */
	UFUNCTION(BlueprintCallable, Category = LevelUp)
	void RequestReroll();

	/** 카드 선택 시 조정자에 통지 */
	FDelagateOnCardSelected OnCardSelected;

	/** 다시 뽑기 요청 시 조정자에 통지 */
	FDelegateOnRerollRequested OnRerollRequested;

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** 카드 한 장의 위젯 묶음 (선택 위젯은 nullptr일 수 있다) */
	struct FCardWidgets
	{
		UButton* Button = nullptr;
		UTextBlock* Title = nullptr;
		UTextBlock* Desc = nullptr;
		UTextBlock* Detail = nullptr;
		UImage* Icon = nullptr;
		UTextBlock* Tag = nullptr;
		UPanelWidget* Pips = nullptr;
		UBorder* Band = nullptr;
		UTextBlock* BandText = nullptr;
		UImage* BandIcon = nullptr;
	};

	/** 카드 한 장을 채운다 (없는 옵션이면 숨김) */
	void FillCard(const FCardWidgets& Widgets, int32 Index);

	/** 진행 줄 한 칸을 채운다 (Progress가 nullptr이면 숨김) */
	void FillBuildEntry(UTextBlock* Name, UPanelWidget* Pips, const FUSBuildProgress* Progress);

	UFUNCTION()
	void HandleCard0Clicked();

	UFUNCTION()
	void HandleCard1Clicked();

	UFUNCTION()
	void HandleCard2Clicked();

	UFUNCTION()
	void HandleRerollClicked();

	/** 카드가 뜰 때 소리 */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp")
	TObjectPtr<USoundBase> OpenSound;

	/** 카드를 고를 때 소리 */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp")
	TObjectPtr<USoundBase> SelectSound;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Card0Button;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Card1Button;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Card2Button;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Card0Title;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Card1Title;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Card2Title;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Card0Desc;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Card1Desc;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Card2Desc;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Card0Icon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Card1Icon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Card2Icon;

	/** 기체 칩 ("기관포기"·"대장"·"보급") */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Card0Tag;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Card1Tag;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Card2Tag;

	/** 효과 아래 회색 보충 설명 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Card0Detail;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Card1Detail;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Card2Detail;

	/** 단계 점 줄 (HorizontalBox) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> Card0Pips;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> Card1Pips;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> Card2Pips;

	/** 카드 위 빌드 색 띠 (Border) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Card0Band;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Card1Band;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> Card2Band;

	/** 색 띠 글자 ("포격 빌드") */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Card0BandText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Card1BandText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Card2BandText;

	/** 색 띠 아이콘 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Card0BandIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Card1BandIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> Card2BandIcon;

	/** 화면 위 빌드 진행 줄: 이름 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Build0Name;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Build1Name;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Build2Name;

	/** 화면 위 빌드 진행 줄: 칸 (HorizontalBox) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> Build0Pips;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> Build1Pips;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> Build2Pips;

	/** 다시 뽑기 버튼 (카드 아래 가운데) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RerollButton;

	/** 다시 뽑기 버튼 글자 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RerollText;

	/** 다시 뽑기 버튼 글자. {0} = 남은 횟수 */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Reroll")
	FText RerollFormat;

	/** 다시 뽑을 때 소리 (비우면 SelectSound) */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Reroll")
	TObjectPtr<USoundBase> RerollSound;

	/** 완성한 빌드 이름. {0} = 빌드 이름, {1} = 보너스 설명 */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Build")
	FText BuildCompleteFormat;

	/** 진행 줄 칸 크기(px, 네모) */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Build", meta = (ClampMin = "1.0"))
	float BuildPipSize = 12.0f;

	/** 단계 점 크기·간격(px) */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Pips", meta = (ClampMin = "1.0"))
	float PipSize = 12.0f;

	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Pips", meta = (ClampMin = "0.0"))
	float PipSpacing = 5.0f;

private:
	UPROPERTY()
	TArray<FUSLevelUpCard> CardOptions;

	/** 한 번 고르면 연타로 두 번 적용되지 않게 막는다 */
	bool bSelected = false;

	/** 남은 다시 뽑기 횟수 (표시·R 키 판단용, 실제 횟수는 조정자가 든다) */
	int32 RerollsLeft = 0;
};
