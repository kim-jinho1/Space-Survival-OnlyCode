#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USBossHealthBarWidget.generated.h"

class AUSBossEnemyCharacter;
class UProgressBar;
class USoundBase;
class UTextBlock;
class UWidget;

/**
 * 보스 등장 경고 + 화면 위 가운데 보스 체력바 (출시 전 체크 R-36, 사용자 결정: 체력바 목업 A, 경고 목업 B).
 * 경고(ShowWarning): 공사장 줄무늬 띠 "WARNING"의 줄무늬가 부드럽게 깜빡이며 위는 오른쪽·아래는 왼쪽으로 계속 흐르고(사용자 요청), 화면 가장자리가 빨갛게 맥박치며,
 * 보스가 화면 밖이면 들어오는 쪽 가장자리에 화살표를 띄운다 (다라이어스 "WARNING" 식). WarningSeconds 뒤 사라진다.
 * 컨트롤러가 보스전 시작 때 ShowWarning → SetBoss, 격파 때 HideAll을 부른다. 뷰포트에는 판 내내 숨긴 채로 올려 둔다.
 * 위젯 이름: WarningPanel(띠 전체), BarPanel(이름 + 바), NameText, HealthBar.
 * 선택: StripeTop, StripeBottom(깜빡이는 줄무늬), EdgeGlow(화면 테두리 빨강), DirectionArrow(화면 가운데 기준 이동·회전, 오른쪽을 가리키는 그림).
 * 보스 신호(R-41 2, 사용자 결정 A): SignalBand(가운데 주황 띠 "보스 신호 포착", SignalBandSeconds 동안 페이드) + CallPanel(위 가운데 "[B] 이름 호출", B 키 그림 CallKey가 맥박),
 *   CallText(호출 문구). 셋 다 선택 — 없으면 ApproachText에 SignalFormat 글자만 띄운다.
 */
UCLASS()
class US_API UUSBossHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 경고를 WarningSeconds 동안 띄우고 경고음을 낸다. Focus가 화면 밖이면 그쪽을 가리키는 화살표 */
	void ShowWarning(AActor* Focus);

	/** 이 보스의 이름·체력을 보이고 체력 변화를 따라간다 */
	void SetBoss(AUSBossEnemyCharacter* Boss, const FText& BossName);

	/** 경고와 바를 모두 숨기고 보스 구독을 끊는다 */
	void HideAll();

	/** 보스 등장 전 "접근 중" 카운트다운을 보이거나 고친다 (ApproachText가 없으면 아무것도 안 함) */
	void ShowApproach(const FText& BossName, float SecondsLeft);

	/** 보스 신호 포착 → 가운데 띠를 잠깐, 위 가운데 호출 칸은 B를 누르거나 귀환할 때까지 띄운다 */
	void ShowSignal(const FText& BossName);

	/** 카운트다운·호출 칸을 숨긴다 */
	void HideApproach();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 경고 중에만 깜빡임·맥박·화살표를 갱신한다 */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void HandleBossHealthChanged(float CurrentHealth, float MaxHealth);
	void HideWarning();
	void Unbind();

	/** 화살표를 Focus 쪽 화면 가장자리에 놓는다. 화면 안이면 false */
	bool PlaceArrow(const FVector& WorldLocation) const;

	/** 경고가 떠 있는 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss", meta = (ClampMin = "0.1"))
	float WarningSeconds = 3.0f;

	/** 줄무늬가 한 번 밝아졌다 어두워지는 주기(초, 사인으로 부드럽게) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss", meta = (ClampMin = "0.05"))
	float StripeBlinkPeriod = 0.6f;

	/** 줄무늬가 흐르는 속도 (위젯 좌표/초). 위는 오른쪽, 아래는 왼쪽 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	float StripeScrollSpeed = 120.0f;

	/** 줄무늬 그림 한 칸 너비 (위젯 좌표, 브러시 ImageSize.X와 같게). 이만큼 움직이면 처음 모양과 같아 이어진다 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss", meta = (ClampMin = "1.0"))
	float StripeTileWidth = 48.0f;

	/** 줄무늬가 가장 어두울 때 불투명도 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StripeDimOpacity = 0.15f;

	/** 화면 테두리가 한 번 밝아졌다 어두워지는 주기(초) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss", meta = (ClampMin = "0.1"))
	float EdgeGlowPeriod = 0.8f;

	/** 화살표가 화면 끝에서 떨어진 거리 (위젯 좌표) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss", meta = (ClampMin = "0.0"))
	float ArrowEdgeMargin = 90.0f;

	/** 신호 카운트다운 문구. {Boss} = 보스 이름, {Seconds} = 남은 초 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	FText ApproachFormat = NSLOCTEXT("USBossHealthBarWidget", "SignalCountdownFormat", "{Boss} 신호 포착까지 · {Seconds}초");

	/** 호출 칸이 없을 때 ApproachText에 쓰는 문구. {Boss} = 보스 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	FText SignalFormat = NSLOCTEXT("USBossHealthBarWidget", "SignalFormat", "{Boss} 신호 포착 · B 호출");

	/** 호출 칸 문구. {Boss} = 보스 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Signal")
	FText CallFormat = NSLOCTEXT("USBossHealthBarWidget", "CallFormat", "{Boss} 호출");

	/** 가운데 신호 띠가 떠 있는 시간(초). 끝 0.5초는 페이드아웃 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Signal", meta = (ClampMin = "0.5"))
	float SignalBandSeconds = 2.0f;

	/** 호출 칸 B 키가 한 번 밝아졌다 어두워지는 주기(초) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Signal", meta = (ClampMin = "0.1"))
	float CallKeyPulsePeriod = 1.0f;

	/** 신호 포착 알림음 (비우면 없음) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Signal")
	TObjectPtr<USoundBase> SignalSound;

	/** 경고음 (UI 소리라 일시정지·슬로모션과 무관) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss")
	TObjectPtr<USoundBase> WarningSound;

	/** 경고 동안 경고음을 울리는 횟수 (사용자: 한 번은 약하다, 2~3번) */
	UPROPERTY(EditDefaultsOnly, Category = "Boss", meta = (ClampMin = "1"))
	int32 WarningSoundCount = 3;

	/** 경고음 사이 간격(초). 소리 길이보다 짧으면 앞 소리 꼬리와 겹친다 */
	UPROPERTY(EditDefaultsOnly, Category = "Boss", meta = (ClampMin = "0.05"))
	float WarningSoundInterval = 1.15f;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> WarningPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> StripeTop;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> StripeBottom;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> EdgeGlow;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DirectionArrow;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> BarPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> HealthBar;

	/** 보스 등장 전 카운트다운 (선택) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ApproachText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SignalBand;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CallPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CallKey;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CallText;

private:
	TWeakObjectPtr<AUSBossEnemyCharacter> BoundBoss;
	TWeakObjectPtr<AActor> WarningFocus;
	FDelegateHandle HealthChangedHandle;
	FTimerHandle WarningTimerHandle;

	/** 경고 중인가 (NativeTick이 이때만 일한다) */
	bool bWarningActive = false;

	/** 경고가 시작된 뒤 흐른 시간(초) */
	float WarningElapsed = 0.0f;

	/** 이번 경고에서 울린 경고음 수 (NativeTick이 간격마다 하나씩 더 울린다) */
	int32 WarningSoundsPlayed = 0;

	/** 호출 칸이 떠 있는가 (NativeTick이 B 키 맥박·신호 띠 페이드를 그린다) */
	bool bSignalActive = false;

	/** 신호가 뜬 뒤 흐른 시간(초) */
	float SignalElapsed = 0.0f;
};
