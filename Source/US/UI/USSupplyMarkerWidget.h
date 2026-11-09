#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USSupplyMarkerWidget.generated.h"

class AUSSupplyShip;
class UProgressBar;
class UTextBlock;

/**
 * 보급선 머리 위 표시. "보급선" 이름표 + 떠날 때까지 남은 시간(0:25)과 막대.
 * 보급선의 화면 공간 위젯 컴포넌트가 띄우고, 보급선이 BeginPlay에서 자신을 알려 준다.
 * 남은 시간은 보급선 상태 타이머에서 읽는다. 글자는 초가 바뀔 때만 다시 쓴다.
 * 대장이 상호작용 거리 안에 들어오면 "[Q] 상점" 안내를 보인다 (거리 비교 한 번이라 매 프레임 봐도 싸다).
 * 위젯 이름: TimeText, (선택) TimeBar, InteractText.
 */
UCLASS()
class US_API UUSSupplyMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSSupplyMarkerWidget(const FObjectInitializer& ObjectInitializer);

	/** 표시할 보급선 */
	void SetSupplyShip(AUSSupplyShip* InSupplyShip);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 남은 시간 서식 ({0} = 분, {1} = 두 자리 초) */
	UPROPERTY(EditDefaultsOnly, Category = "Marker")
	FText TimeFormat;

	/** 상호작용 안내 (대장이 거리 안에 있을 때만 보임) */
	UPROPERTY(EditDefaultsOnly, Category = "Marker")
	FText InteractPrompt;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TimeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InteractText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> TimeBar;

private:
	TWeakObjectPtr<AUSSupplyShip> SupplyShip;

	/** 마지막으로 쓴 남은 초 (같은 초면 글자를 다시 쓰지 않는다) */
	int32 LastShownSeconds = INDEX_NONE;

	/** 마지막으로 정한 안내 표시 여부 (바뀔 때만 SetVisibility) */
	bool bPromptShown = true;
};
