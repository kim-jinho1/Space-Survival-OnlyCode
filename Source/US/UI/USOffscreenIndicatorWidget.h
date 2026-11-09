#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USOffscreenIndicatorWidget.generated.h"

class UImage;
class UWidget;

/**
 * 화면 밖 대상 방향 표시기. 대상이 화면 안이면 숨고, 화면 밖이면 화면 가장자리에서 대상 쪽을 가리키는 화살표를 그린다.
 * 위젯 이름: ArrowImage, (선택) BadgeWidget — 둘 다 화면 중앙에 앵커(0.5, 0.5)·정렬(0.5, 0.5)로 둔다. C++이 중앙 기준 이동·회전만 바꾼다.
 * 표식(BadgeWidget)이 있으면 표식은 돌지 않고 가장자리에 놓이고, 화살표는 표식 바깥쪽 ArrowDistance만큼에서 대상 쪽을 가리킨다.
 * 대상의 화면 위치는 카메라와 함께 매 프레임 바뀌므로 Tick으로 투영하되, 대상이 없으면 첫 줄에서 바로 빠진다.
 */
UCLASS()
class US_API UUSOffscreenIndicatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 가리킬 대상 (nullptr이면 숨김) */
	void SetTarget(AActor* InTarget);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 화살표를 숨기거나 보인다 (바뀔 때만 설정) */
	void SetArrowVisible(bool bVisible);

	/** 화면 가장자리에서 화살표까지 띄우는 거리 (위젯 좌표). 대상이 이만큼 안쪽에 들어오면 화면 안으로 본다 */
	UPROPERTY(EditDefaultsOnly, Category = "Indicator", meta = (ClampMin = "0.0"))
	float EdgeMargin = 60.0f;

	/** 화살표 텍스처가 기본으로 가리키는 방향 보정 (도). 오른쪽을 가리키는 그림이면 0 */
	UPROPERTY(EditDefaultsOnly, Category = "Indicator")
	float ArrowAngleOffset = 0.0f;

	/** 표식 중심에서 화살표까지 거리 (위젯 좌표). 표식이 없으면 쓰지 않는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Indicator", meta = (ClampMin = "0.0"))
	float ArrowDistance = 26.0f;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ArrowImage;

	/** 돌지 않는 표식 (원 + 아이콘). 없으면 화살표만 그린다 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BadgeWidget;

private:
	/** 가리킬 대상. 파괴되면 자동으로 무효가 된다 */
	TWeakObjectPtr<AActor> Target;

	/** 화살표가 지금 보이는가 (같은 값을 매 프레임 다시 설정하지 않으려고 기록) */
	bool bArrowVisible = true;
};
