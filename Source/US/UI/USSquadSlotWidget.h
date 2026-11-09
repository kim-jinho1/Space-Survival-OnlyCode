#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USSquadSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

/**
 * 분대 칸 하나: 기종 실루엣(체력 색으로 칠함), 왼쪽 위 부활 키 번호, 격추됐을 때 부활 가격.
 * 색·글자는 분대 위젯(UUSSquadWidget)이 정해서 넣어 준다. 이 위젯은 그리기만 한다.
 * 위젯 이름: SlotIcon, KeyText, (선택) CostText.
 */
UCLASS()
class US_API UUSSquadSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 키 번호와 그림 (분대 구성이 바뀔 때 한 번) */
	void SetupSlot(int32 KeyNumber, UTexture2D* Icon);

	/** 실루엣 색 (체력 단계 또는 격추 회색) */
	void SetIconColor(const FLinearColor& Color);

	/** 부활 가격 글자. Cost가 비면 숨긴다 */
	void SetCost(const FText& Cost, const FLinearColor& Color);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SlotIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KeyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText;
};
