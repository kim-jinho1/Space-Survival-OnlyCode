#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USControlsGuideWidget.generated.h"

class UTextBlock;
class UVerticalBox;

/** 조작 안내 한 줄: 키 그림 여러 개 + 하는 일 */
USTRUCT(BlueprintType)
struct FUSKeyGuideEntry
{
	GENERATED_BODY()

	/** 키 그림 하나에 들어갈 글자들 (W, A, S, D처럼 여러 개면 나란히) */
	UPROPERTY(EditAnywhere, Category = "Guide")
	TArray<FText> Keys;

	UPROPERTY(EditAnywhere, Category = "Guide")
	FText Action;
};

/** 조작 안내 카드를 닫음 */
DECLARE_DELEGATE(FDelegateOnControlsGuideDismissed);

/**
 * 조작 안내 카드 (출시 전 체크 R-14, 사용자 결정 A — 첫 판 시작 때 게임을 멈추고 가운데에 한 번).
 * 키 목록은 C++ 기본값(Entries)으로 줄마다 [키 그림][하는 일]을 만들어 채운다 — 키가 바뀌면 한 곳만 고치게.
 * 첫 판 카드는 아무 키·클릭으로 닫히고(bDismissible), 일시정지 창 안에 넣은 것은 목록만 보여 준다.
 * 위젯 이름: KeyList(세로 상자), (선택) HintText.
 */
UCLASS()
class US_API UUSControlsGuideWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSControlsGuideWidget(const FObjectInitializer& ObjectInitializer);

	/** 아무 키·클릭 (bDismissible일 때만) */
	FDelegateOnControlsGuideDismissed OnDismissed;

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 줄마다 키 그림과 설명을 만들어 KeyList에 넣는다 */
	void BuildKeyList();

	void Dismiss();

	UPROPERTY(EditAnywhere, Category = "Guide")
	TArray<FUSKeyGuideEntry> Entries;

	/** 아무 키로 닫히는 카드인가 (일시정지 창 안의 목록은 끔 → 안내 글도 숨김) */
	UPROPERTY(EditAnywhere, Category = "Guide")
	bool bDismissible = true;

	UPROPERTY(EditAnywhere, Category = "Guide|Style")
	FSlateFontInfo KeyFont;

	UPROPERTY(EditAnywhere, Category = "Guide|Style")
	FSlateFontInfo ActionFont;

	UPROPERTY(EditAnywhere, Category = "Guide|Style")
	FSlateColor KeyTextColor = FLinearColor(0.75f, 0.92f, 1.0f, 1.0f);

	/** 키 그림 테두리 (하늘색 = HUD·외곽선과 같은 아군 색) */
	UPROPERTY(EditAnywhere, Category = "Guide|Style")
	FLinearColor KeyOutlineColor = FLinearColor(0.37f, 0.78f, 0.91f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "Guide|Style")
	FLinearColor KeyFillColor = FLinearColor(0.02f, 0.05f, 0.09f, 0.9f);

	UPROPERTY(EditAnywhere, Category = "Guide|Style")
	FSlateColor ActionTextColor = FLinearColor(0.78f, 0.83f, 0.9f, 1.0f);

	/** 키 그림 칸과 설명 사이 간격. 키 칸 너비는 가장 긴 키 줄에 자동으로 맞춘다 (고정 너비 190은 키 2개 줄이 넘쳐 설명을 덮었다, 2026-10-31) */
	UPROPERTY(EditAnywhere, Category = "Guide|Style", meta = (ClampMin = "0.0"))
	float KeyActionGap = 32.0f;

	UPROPERTY(EditAnywhere, Category = "Guide|Text")
	FText HintLabel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> KeyList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HintText;

private:
	/** 닫기를 한 번만 알린다 (키 누름과 클릭이 같은 프레임에 겹쳐도) */
	bool bDismissed = false;
};
