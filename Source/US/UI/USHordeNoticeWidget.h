#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Spawning/USEnemySpawner.h"
#include "USHordeNoticeWidget.generated.h"

class UImage;
class UTextBlock;
class USoundBase;

/**
 * 적 무리 예고 (M-3). 스포너의 OnHordeWarning을 컨트롤러가 전달하면 문구(비어 있으면 생략)를 띄우고,
 * Rush(한 방향 돌진)면 무리 쪽 화면 끝에 깜빡이는 화살표를 둔다. Ring(포위)은 스포너가 바닥에 X를 찍으므로 문구만.
 * 화살표는 화살표가 보이는 동안 매 프레임 다시 투영한다 — 대장이 빠르게 움직이면 1~2초 사이에도 방향이 크게 바뀐다
 * (예전엔 예고 순간 한 번만 계산해 엉뚱한 곳을 가리켰다, 플레이테스트 2026-10-20). 화살표가 없으면 Tick은 첫 줄에서 빠진다.
 * 깜빡임은 BlinkInterval 타이머로 투명도만 바꾼다.
 * 위젯 이름: NoticeText(화면 위쪽), WarningArrow(화면 중앙 앵커(0.5, 0.5)·정렬(0.5, 0.5)).
 */
UCLASS()
class US_API UUSHordeNoticeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSHordeNoticeWidget(const FObjectInitializer& ObjectInitializer);

	/** 무리 예고 → 문구·띠를 보이고 HoldTime 뒤 숨긴다 (컨트롤러가 전달) */
	void HandleHordeWarning(EUSHordeShape Shape, const FVector& HordeCenter, float SecondsUntilSpawn);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 보이는 시간이 끝남 → 숨김 */
	void HandleHoldFinished();

	/** 무리 자리가 화면 밖이면 화살표를 그쪽 화면 끝에 두고 보이고, 화면 안이거나 투영에 실패하면 숨긴다 */
	void UpdateWarningArrow();

	/** 화살표 깜빡임 한 번 (밝음 ↔ 흐림) */
	void HandleBlink();

	/** 한 방향 돌진 예고 문구. 비우면 글자 없이 화살표만 */
	UPROPERTY(EditDefaultsOnly, Category = "Notice")
	FText RushText;

	/** 포위 예고 문구. 비우면 글자 없이 바닥 X만 */
	UPROPERTY(EditDefaultsOnly, Category = "Notice")
	FText RingText;

	/** 예고를 보여 주는 최소 시간(초). 무리가 나오기까지 남은 시간이 더 길면 그만큼 */
	UPROPERTY(EditDefaultsOnly, Category = "Notice", meta = (ClampMin = "0.0"))
	float MinHoldTime = 2.0f;

	/** 화면 가장자리에서 화살표까지 띄우는 거리 (위젯 좌표) */
	UPROPERTY(EditDefaultsOnly, Category = "Notice", meta = (ClampMin = "0.0"))
	float EdgeMargin = 70.0f;

	/** 화살표 텍스처가 기본으로 가리키는 방향 보정 (도). 오른쪽을 가리키는 그림이면 0 (보급선 화살표와 같은 규칙) */
	UPROPERTY(EditDefaultsOnly, Category = "Notice")
	float ArrowAngleOffset = 0.0f;

	/** 깜빡임 반 주기(초) */
	UPROPERTY(EditDefaultsOnly, Category = "Notice", meta = (ClampMin = "0.05"))
	float BlinkInterval = 0.2f;

	/** 깜빡임에서 흐린 쪽 투명도 */
	UPROPERTY(EditDefaultsOnly, Category = "Notice", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BlinkDimOpacity = 0.25f;

	/** 예고 소리 (UI 소리) */
	UPROPERTY(EditDefaultsOnly, Category = "Notice")
	TObjectPtr<USoundBase> WarningSound;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> NoticeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> WarningArrow;

private:
	FTimerHandle HoldTimerHandle;
	FTimerHandle BlinkTimerHandle;

	/** 깜빡임에서 지금 밝은 쪽인가 */
	bool bBlinkBright = true;

	/** 화살표가 따라가는 무리 중심 (월드). 스폰 자리는 예고 때 고정되므로 이 점도 고정 */
	FVector ArrowTargetLocation = FVector::ZeroVector;

	/** 돌진 예고 중인가 (NativeTick이 이때만 일한다). 무리 자리가 화면 안이면 추적 중에도 화살표는 숨는다 */
	bool bTrackingArrow = false;
};
