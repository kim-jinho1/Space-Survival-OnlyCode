#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USMetaNodeEntryWidget.generated.h"

class UBorder;
class UButton;
class UPanelWidget;
class UTextBlock;
class UUSMetaProgressionSubsystem;
class UUSMetaUpgradeData;
class UWidget;

/** 칸을 클릭함 (트리 위젯이 바인딩해 구매를 시도한다) */
DECLARE_DELEGATE_OneParam(FDelegateOnNodeClicked, const UUSMetaUpgradeData*);

/** 칸 하나가 지금 어떤 상태인가 (색과 버튼 활성화를 정한다) */
UENUM()
enum class EUSMetaNodeState : uint8
{
	/** 선행 칸을 아직 안 찍음 */
	Locked,
	/** 지금 살 수 있음 */
	Affordable,
	/** 열렸지만 스크랩 부족 */
	Unaffordable,
	/** 최대 단계 */
	Maxed
};

/**
 * 업그레이드 트리의 칸 하나: 이름, 단계(칸 ■■□□□ 또는 글자 x/y), 효과 설명, 가격 칩(또는 잠김·최대 표시), 클릭 버튼.
 * 트리 위젯이 칸 데이터를 한 번 넣어 주고(SetupNode), 메타 진행이 바뀔 때마다 RefreshNode로 다시 그린다.
 * ActionText가 있으면 가격 칩이 "강화 버튼" 모양이 된다(멘토링 2026-10-16: 칸이 버튼인지 헷갈림, Brotato 상점·Vampire Survivors 파워업처럼
 * 가격 + 화폐 그림이 든 버튼): 상태 글자(강화·부족·MAX·잠김)는 ActionText, 가격 숫자는 CostText, 스크랩 그림은 ScrapIcon.
 * 위젯 이름: NodeButton, NameText, CostText, (선택) DescriptionText, PartText, LevelText, LevelPips, CostChip, ActionText, ScrapIcon.
 */
UCLASS()
class US_API UUSMetaNodeEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSMetaNodeEntryWidget(const FObjectInitializer& ObjectInitializer);

	/** 이 칸이 보여 줄 데이터 */
	void SetupNode(const UUSMetaUpgradeData* InNode);

	/** 지금 단계·가격·상태로 다시 그린다 */
	void RefreshNode(const UUSMetaProgressionSubsystem& Meta);

	FORCEINLINE const UUSMetaUpgradeData* GetNode() const { return Node; }

	/** 클릭 알림 */
	FDelegateOnNodeClicked OnNodeClicked;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	UFUNCTION()
	void HandleButtonClicked();

	/** 단계 글자. {0} = 지금 단계, {1} = 최대 단계 */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Text")
	FText LevelFormat;

	/** 가격 글자. {0} = 스크랩 */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Text")
	FText CostFormat;

	/** 최대 단계일 때 가격 자리 글자 */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Text")
	FText MaxedText;

	/** 잠겼을 때 선행 칸 안내. ActionText가 있으면 부위 줄(PartText)에, 없으면 가격 자리에 쓴다. {0} = 선행 칸 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Text")
	FText LockedFormat;

	/** 부위 줄 글자 (효과가 있는 칸). {0} = 부위, {1} = 지금 효과, {2} = 다음 단계 효과 */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Text")
	FText PartEffectFormat;

	/** 부위 줄 글자 (최대 단계). {0} = 부위, {1} = 지금 효과 */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Text")
	FText PartMaxedFormat;

	/** 강화 버튼 글자 (ActionText): 지금 살 수 있음 / 스크랩 부족 / 최대 / 잠김 */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Text")
	FText BuyText;
	UPROPERTY(EditDefaultsOnly, Category = "Node|Text")
	FText ShortText;
	UPROPERTY(EditDefaultsOnly, Category = "Node|Text")
	FText ActionMaxedText;
	UPROPERTY(EditDefaultsOnly, Category = "Node|Text")
	FText ActionLockedText;

	/** 단계 칸 하나의 너비(px) */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Pips", meta = (ClampMin = "1.0"))
	float PipSize = 9.0f;

	/** 단계 칸 높이(px). 0이면 너비와 같다 (정사각형·원) */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Pips", meta = (ClampMin = "0.0"))
	float PipHeight = 0.0f;

	/** 단계 칸 모서리 반지름(px). 음수면 짧은 변의 절반 = 둥근 점 */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Pips")
	float PipCornerRadius = -1.0f;

	/** 다음에 살 칸의 테두리 색 (알파 0 = 표시 안 함) */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Pips")
	FLinearColor PipNextOutlineColor = FLinearColor::Transparent;

	/** 점 사이 간격(px) */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Pips", meta = (ClampMin = "0.0"))
	float PipSpacing = 3.0f;

	/** 찍은 단계 점 색 */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Pips")
	FLinearColor PipFilledColor = FLinearColor(0.37f, 0.88f, 0.95f, 1.0f);

	/** 남은 단계 점 색 */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Pips")
	FLinearColor PipEmptyColor = FLinearColor(0.16f, 0.23f, 0.31f, 1.0f);

	/** 상태별 버튼 배경색 */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Color")
	FLinearColor LockedColor = FLinearColor(0.25f, 0.25f, 0.28f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Node|Color")
	FLinearColor AffordableColor = FLinearColor(0.0f, 0.9f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Node|Color")
	FLinearColor UnaffordableColor = FLinearColor(0.55f, 0.6f, 0.7f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Node|Color")
	FLinearColor MaxedColor = FLinearColor(1.0f, 0.8f, 0.3f, 1.0f);

	/** 가격 칩 바탕·글자색: 지금 살 수 있음 */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Chip")
	FLinearColor ChipAffordableColor = FLinearColor(0.008f, 0.05f, 0.06f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Node|Chip")
	FLinearColor ChipAffordableTextColor = FLinearColor(0.21f, 0.79f, 0.81f, 1.0f);

	/** 가격 칩 바탕·글자색: 스크랩 부족·잠김·최대 */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Chip")
	FLinearColor ChipIdleColor = FLinearColor(0.02f, 0.025f, 0.04f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Node|Chip")
	FLinearColor ChipIdleTextColor = FLinearColor(0.18f, 0.2f, 0.25f, 1.0f);

	/** 강화 버튼(ActionText 있음) 상태 글자색: 지금 살 수 있음 (바탕 ChipAffordableColor 위) */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Chip")
	FLinearColor ActionAffordableTextColor = FLinearColor(0.002f, 0.02f, 0.025f, 1.0f);

	/** 스크랩 부족 글자색 ("부족") */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Chip")
	FLinearColor ActionShortTextColor = FLinearColor(0.8f, 0.16f, 0.08f, 1.0f);

	/** 최대 단계 칩 바탕·글자색 */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Chip")
	FLinearColor ChipMaxedColor = FLinearColor(0.06f, 0.04f, 0.005f, 1.0f);
	UPROPERTY(EditDefaultsOnly, Category = "Node|Chip")
	FLinearColor ChipMaxedTextColor = FLinearColor(0.89f, 0.53f, 0.07f, 1.0f);

	/** 잠겼을 때 부위 줄(선행 칸 안내) 글자색 — 잠긴 칸 중 눈에 띄어야 할 유일한 글자라 주황 */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Color")
	FLinearColor LockedPartTextColor = FLinearColor(0.89f, 0.48f, 0.07f, 1.0f);

	/** 마우스를 올렸을 때 칩 바탕을 밝히는 배율 (살 수 있음·부족일 때만) */
	UPROPERTY(EditDefaultsOnly, Category = "Node|Chip", meta = (ClampMin = "1.0"))
	float ChipHoverBrightness = 1.25f;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NodeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	/** 단계 글자 "x / y". LevelPips가 있으면 없어도 된다 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	/** 단계 점을 담을 가로 상자 (HorizontalBox). 점은 최대 단계 수만큼 C++이 만든다 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> LevelPips;

	/** 효과 설명. 격납고 강화 줄은 한 줄이라 없어도 된다 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

	/** 고치는 부위 이름 (설계도 카드에만 있다) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PartText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CostText;

	/** 가격 글자를 감싼 칩 (Border). 있으면 상태에 따라 바탕·글자색을 바꾼다 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CostChip;

	/** 강화 버튼 상태 글자 (강화·부족·MAX·잠김). 있으면 CostText는 가격 숫자만 보인다 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ActionText;

	/** 가격 옆 스크랩 그림. 가격이 보일 때(살 수 있음·부족)만 보인다 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ScrapIcon;

private:
	/** 단계 점을 Level/MaxLevel로 다시 칠한다. 점 개수가 다르면 새로 만든다 */
	void RefreshPips(int32 Level, int32 MaxLevel);

	/** 가격 칩·글자를 지금 상태와 마우스 호버로 칠한다 (CurrentState·bHovered) */
	void RefreshChip();

	/** 마지막 RefreshNode의 상태 (호버가 바뀔 때 칩을 다시 칠하려고 들고 있다) */
	EUSMetaNodeState CurrentState = EUSMetaNodeState::Locked;

	/** BP에서 정한 부위 줄 글자색 (잠김이 풀리면 되돌린다) */
	FSlateColor DefaultPartTextColor;

	/** 마우스가 칸 위에 있는가 */
	bool bHovered = false;

	/** 이 칸의 데이터 (트리 애셋이 붙잡고 있어 GC되지 않지만 UPROPERTY로 안전하게 든다) */
	UPROPERTY()
	TObjectPtr<const UUSMetaUpgradeData> Node;
};
