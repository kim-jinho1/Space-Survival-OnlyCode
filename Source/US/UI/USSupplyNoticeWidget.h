#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Shop/USSupplyShip.h"
#include "USSupplyNoticeWidget.generated.h"

class UTextBlock;
class USoundBase;

/**
 * 보급선 안내 문구 (화면 위쪽). "N초 후 도착 → 도착! → N초 후 떠남 → 떠났음"을 보여 준다.
 * 보급선 상태 델리게이트로만 갱신하고, 초 단위 숫자는 1초 간격 타이머로 줄인다 (매 프레임 폴링 없음).
 * 월드 타이머라 상점·카드로 게임이 멈추면 숫자도 같이 멈춘다 (보급선 타이머와 맞는다).
 * 위젯 이름: NoticeText.
 */
UCLASS()
class US_API UUSSupplyNoticeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSSupplyNoticeWidget(const FObjectInitializer& ObjectInitializer);

	/** 보급선 상태가 바뀜 → 문구를 바꾼다 (컨트롤러가 전달) */
	void HandleSupplyStateChanged(EUSSupplyShipState NewState, float SecondsRemaining);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 1초마다 남은 초를 줄이고 문구를 다시 쓴다 */
	void HandleCountdownTick();

	/** 잠깐 보여 주는 문구가 끝남 → 숨김 */
	void HandleHoldFinished();

	/** 서식 문구에 남은 초를 넣어 보여 준다 */
	void ShowCountdownText();

	/** 문구를 보여 주고, HoldTime이 0보다 크면 그만큼 뒤에 숨긴다 */
	void ShowMessage(const FText& Message, float HoldTime);

	/** 타이머를 모두 멈춘다 */
	void ClearTimers();

	/** 도착 예고. {0} = 남은 초 */
	UPROPERTY(EditDefaultsOnly, Category = "Notice")
	FText ArrivingFormat;

	/** 도착 */
	UPROPERTY(EditDefaultsOnly, Category = "Notice")
	FText ArrivedText;

	/** 출발 예고. {0} = 남은 초 */
	UPROPERTY(EditDefaultsOnly, Category = "Notice")
	FText DepartingFormat;

	/** 출발 */
	UPROPERTY(EditDefaultsOnly, Category = "Notice")
	FText DepartedText;

	/** 도착·출발 문구를 보여 주는 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Notice", meta = (ClampMin = "0.0"))
	float MessageHoldTime = 3.0f;

	/** 예고가 시작될 때와 도착할 때 소리 (UI 소리) */
	UPROPERTY(EditDefaultsOnly, Category = "Notice")
	TObjectPtr<USoundBase> NoticeSound;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NoticeText;

private:
	FTimerHandle CountdownTimerHandle;
	FTimerHandle HoldTimerHandle;

	/** 지금 보여 주는 서식 (도착 예고 / 출발 예고) */
	FText CountdownFormat;

	/** 남은 초 (정수로 보여 준다) */
	int32 RemainingSeconds = 0;

	/** 출발 예고를 받았는가. 판 시작 때 첫 도착을 늦추려고 들어오는 '없음' 상태에는 "떠났습니다"를 띄우지 않는다 */
	bool bDepartureAnnounced = false;
};
