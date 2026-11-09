#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USSquadWidget.generated.h"

class UPanelWidget;
class UUSAttributeComponent;
class UUSCurrencyComponent;
class UUSSquadComponent;
class UUSSquadSlotWidget;
class UTextBlock;
class USoundBase;

/**
 * 분대 표시: 부하 한 기마다 칸 하나(기종 실루엣 + 부활 키 번호). 실루엣은 체력에 따라 색이 바뀐다 —
 * 스타크래프트에서 유닛을 고르면 와이어프레임이 초록 → 노랑 → 빨강으로 바뀌는 방식. 격추되면 회색 + 보급선 부활 가격.
 * 분대 구성이 바뀌면(OnSquadChanged) 칸을 다시 만들고 부하마다 체력 알림(OnHealthChanged)을 구독한다.
 * 체력이 바뀌면 그 칸만 다시 칠한다 (매 프레임 폴링 없음). 위젯 이름: SlotList (HorizontalBox), (선택) DownNoticeText.
 * 부하가 격추되면 그 칸이 빨강·회색으로 몇 번 깜빡이고 "○○ 격추" 문구와 소리가 난다 (R-41 11, 스타크래프트 와이어프레임 깜빡임).
 */
UCLASS()
class US_API UUSSquadWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 표시할 분대·골드를 지정하고 구독한다 (컨트롤러가 생성 직후 호출) */
	void InitializeForSquad(UUSSquadComponent* InSquad, UUSCurrencyComponent* InCurrency);

protected:
	virtual void NativeDestruct() override;

	/** 분대가 바뀜 → 칸을 다시 만든다 (부활하면 부하 액터가 바뀌므로 체력 구독도 새로) */
	void HandleSquadChanged();

	/** 골드가 바뀜 → 격추 칸의 부활 가격 색만 다시 (살 수 있는지가 바뀐다) */
	void HandleGoldChanged(int32 NewGold);

	/** MemberIndex 부하의 체력이 바뀜 → 그 칸만 다시 칠한다 */
	void HandleMemberHealthChanged(float CurrentHealth, float MaxHealth, int32 MemberIndex);

	/** 부하 격추 → 깜빡임·문구·소리 */
	void HandleMemberDown(int32 MemberIndex);

	/** 깜빡임 한 번 (타이머) */
	void HandleDownFlashTick();

	/** 격추 문구를 숨긴다 (타이머) */
	void HideDownNotice();

	/** 칸 위젯을 분대 구성대로 다시 만든다 */
	void RebuildSlots();

	/** MemberIndex 칸의 색·가격을 지금 상태로 칠한다 */
	void RefreshSlot(int32 MemberIndex);

	/** 체력 비율 → 실루엣 색 */
	FLinearColor GetHealthColor(float HealthRatio) const;

	/** 부하 체력 구독을 모두 푼다 */
	void UnbindMemberHealth();

	/** 칸 위젯 클래스 (WBP_USSquadSlot) */
	UPROPERTY(EditDefaultsOnly, Category = "Squad")
	TSubclassOf<UUSSquadSlotWidget> SlotClass;

	/** 체력 단계 색. 비율이 HealthyThreshold를 넘으면 Healthy, CriticalThreshold를 넘으면 Damaged, 아니면 Critical */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Color")
	FLinearColor HealthyColor = FLinearColor(0.12f, 0.85f, 0.3f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Squad|Color")
	FLinearColor DamagedColor = FLinearColor(0.95f, 0.75f, 0.05f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Squad|Color")
	FLinearColor CriticalColor = FLinearColor(0.95f, 0.2f, 0.18f, 1.0f);

	/** 격추된 칸 실루엣 색 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Color")
	FLinearColor DownColor = FLinearColor(0.25f, 0.28f, 0.33f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Squad|Color", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float HealthyThreshold = 0.66f;

	UPROPERTY(EditDefaultsOnly, Category = "Squad|Color", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CriticalThreshold = 0.33f;

	/** 부활 가격 글자색 (살 수 있음 / 골드 부족) */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Color")
	FLinearColor AffordableCostColor = FLinearColor(1.0f, 0.78f, 0.25f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Squad|Color")
	FLinearColor UnaffordableCostColor = FLinearColor(0.45f, 0.5f, 0.58f, 1.0f);

	/** 격추 깜빡임 색 (DownColor와 번갈아) */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Down")
	FLinearColor DownFlashColor = FLinearColor(1.0f, 0.15f, 0.1f, 1.0f);

	/** 깜빡임 한 번(색 바뀜) 간격(초)과 바뀌는 횟수. 끝나면 격추 회색으로 남는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Down", meta = (ClampMin = "0.02"))
	float DownFlashInterval = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Squad|Down", meta = (ClampMin = "0"))
	int32 DownFlashToggles = 8;

	/** 격추 문구. {0} = 부하 종류 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Down")
	FText DownNoticeFormat = NSLOCTEXT("USSquadWidget", "DownNoticeFormat", "{0} 격추");

	/** 격추 문구가 떠 있는 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Down", meta = (ClampMin = "0.1"))
	float DownNoticeSeconds = 2.0f;

	/** 격추 소리 (비우면 없음) */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Down")
	TObjectPtr<USoundBase> DownSound;

	/** 격추 문구 (선택, 분대 칸 위) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DownNoticeText;

	/** 칸을 담는 가로 상자 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> SlotList;

private:
	UPROPERTY()
	TObjectPtr<UUSSquadComponent> Squad;

	UPROPERTY()
	TObjectPtr<UUSCurrencyComponent> Currency;

	/** 칸 위젯 (분대 Members와 같은 인덱스, 빈 자리는 nullptr) */
	UPROPERTY()
	TArray<TObjectPtr<UUSSquadSlotWidget>> Slots;

	/** 체력을 구독한 부하 속성 컴포넌트 (부하가 먼저 사라질 수 있어 약한 참조) */
	TArray<TWeakObjectPtr<UUSAttributeComponent>> BoundAttributes;

	/** 깜빡이는 칸 (없으면 INDEX_NONE). 칸 위젯은 격추 직후 다시 만들어지므로 인덱스로 기억한다 */
	int32 FlashIndex = INDEX_NONE;

	/** 남은 색 바뀜 횟수 */
	int32 FlashTogglesLeft = 0;

	FTimerHandle DownFlashTimerHandle;
	FTimerHandle DownNoticeTimerHandle;
};
