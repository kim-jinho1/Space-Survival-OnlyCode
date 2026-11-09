#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USHangarOptionWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UTexture2D;

/** 선택지를 누름 (Index = 만든 쪽이 정한 번호) */
DECLARE_DELEGATE_OneParam(FDelegateOnOptionClicked, int32);

/**
 * 격납고의 버튼 하나: 위쪽 기종 아이콘, 출격 지도의 섬, 편대 카드, 편대 기종 고르기 줄이 같이 쓴다.
 * 만든 쪽이 번호·글자·상태를 넣고(SetupOption), 클릭은 번호로 돌려준다.
 * 모양은 WBP마다 다르다: 기종 아이콘(WBP_USHangarCraftOption), 섬(WBP_USHangarIsland), 편대 카드(WBP_USHangarSquadCard), 고르기 줄 칸(WBP_USHangarSlot).
 * 그래서 OptionButton만 필수이고 OptionText·OptionIcon·SubText·CornerText는 없어도 된다(BindWidgetOptional).
 */
UCLASS()
class US_API UUSHangarOptionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 번호·글자·상태. bEnabled가 false면 흐리게(누르면 클릭은 알리되 만든 쪽이 거절), bHighlighted면 강조색 */
	void SetupOption(int32 InIndex, const FText& Label, bool bEnabled, bool bHighlighted);

	/** 아이콘과 아래 줄 글자까지. Icon이 nullptr이면 아이콘 칸을 숨기고, SubLabel이 비면 아래 줄을 숨긴다 */
	void SetupOption(int32 InIndex, const FText& Label, const FText& SubLabel, UTexture2D* Icon, bool bEnabled, bool bHighlighted);

	/** 위 구석 작은 글자 (편대 카드의 칸 번호 "1"·"대장 · 고정"). 비면 숨긴다 */
	void SetCornerText(const FText& Text);

	/** 버튼 배경색을 상태 색 대신 Color로 (편대의 대장 카드 주황 테두리). SetupOption 뒤에 부른다 */
	void SetButtonColor(const FLinearColor& Color);

	/** 키보드로 고른 칸 표시 (KeyFocusFrame 보이기/숨기기, 없으면 무시) */
	void SetKeyFocused(bool bFocused);

	/** 클릭 알림 */
	FDelegateOnOptionClicked OnOptionClicked;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleButtonClicked();

	/** 보통 버튼 배경색 */
	UPROPERTY(EditDefaultsOnly, Category = "Option|Color")
	FLinearColor NormalColor = FLinearColor(0.55f, 0.6f, 0.7f, 1.0f);

	/** 강조(선택됨·편성됨) 버튼 배경색 */
	UPROPERTY(EditDefaultsOnly, Category = "Option|Color")
	FLinearColor HighlightColor = FLinearColor(0.0f, 0.9f, 1.0f, 1.0f);

	/** 잠긴 선택지 배경색 */
	UPROPERTY(EditDefaultsOnly, Category = "Option|Color")
	FLinearColor DisabledColor = FLinearColor(0.25f, 0.25f, 0.28f, 1.0f);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> OptionButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OptionText;

	/** 기종 그림 (기종 카드·편성 칸) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> OptionIcon;

	/** 이름 아래 작은 글자 (기종 카드의 "강화 3/15"·"잠김 · 120 스크랩") */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SubText;

	/** 위 구석 작은 글자 (편대 카드) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CornerText;

	/** 키보드로 고른 칸 테두리 (편성 기종 카드). 버튼 위에 겹쳐 두고 평소엔 숨긴다 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> KeyFocusFrame;

private:
	/** 클릭 때 돌려줄 번호 */
	int32 OptionIndex = INDEX_NONE;
};
