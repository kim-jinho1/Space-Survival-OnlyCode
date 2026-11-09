#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USStageBannerWidget.generated.h"

class UFont;
class UImage;
class UTextBlock;
class UWidget;

/**
 * 스테이지 클리어 글자와 진입 자막 (목업 2026-10-27 재선택: 클리어 A 띠 없이 글자 + 선, 진입 B 가운데 이름 양옆 선), 워프 섬광(화면 전체 흰 판).
 * 글자 몇 줄뿐이라 WBP 없이 C++이 위젯 트리를 만든다. 글꼴은 로딩 화면 글꼴(Pretendard)을 빌린다 (엔진 기본 글꼴엔 한글이 없다).
 * 한 번에 하나만 보이고, HoldSeconds 뒤 FadeSeconds 동안 흐려진다 (월드 타이머).
 */
UCLASS()
class US_API UUSStageBannerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSStageBannerWidget(const FObjectInitializer& ObjectInitializer);

	/** 화면 가운데 "STAGE N / 클리어 / 선 / 보상 회수 중" */
	void ShowStageClear(int32 StageNumber, float HoldSeconds);

	/** 화면 가운데 위 "── 이름 ── / STAGE N · 위험" (bWall이면 위험 붙임) */
	void ShowStageTitle(int32 StageNumber, const FText& StageName, bool bWall, float HoldSeconds);

	/** 워프 섬광: 화면 전체가 InSeconds 동안 PeakOpacity까지 밝아졌다가 OutSeconds 동안 사라진다. OutSeconds가 음수면 밝은 채 남는다 */
	void ShowFlash(float PeakOpacity, float InSeconds, float OutSeconds);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

	/** 위젯 트리를 만든다 (처음 한 번) */
	void BuildTree();

	/** Element를 보이고 HoldSeconds 뒤 흐려지기 시작한다 (다른 쪽은 숨긴다) */
	void ShowElement(UWidget* Element, float HoldSeconds);

	void BeginFade();
	void StepFade();
	void StepFlash();

	/** 클리어 큰 글자 */
	UPROPERTY(EditDefaultsOnly, Category = "Banner|Text")
	FText StageClearText;

	/** 클리어 아랫줄 (남은 보상이 대장에게 빨려 오는 동안) */
	UPROPERTY(EditDefaultsOnly, Category = "Banner|Text")
	FText StageClearSubText;

	/** 클리어 윗줄·진입 자막 아랫줄. {0} = 스테이지 번호 */
	UPROPERTY(EditDefaultsOnly, Category = "Banner|Text")
	FText StageLabelFormat;

	/** 진입 자막 벽 표시 */
	UPROPERTY(EditDefaultsOnly, Category = "Banner|Text")
	FText WallSuffix;

	/** 진입 자막 세로 자리 (화면 높이 비율, 가운데 정렬) */
	UPROPERTY(EditDefaultsOnly, Category = "Banner|Layout", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TitleHeightRatio = 0.25f;

	/** 클리어 글자 세로 자리 (화면 높이 비율, 가운데 정렬) */
	UPROPERTY(EditDefaultsOnly, Category = "Banner|Layout", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ClearHeightRatio = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Banner|Layout", meta = (ClampMin = "1"))
	int32 ClearFontSize = 48;

	UPROPERTY(EditDefaultsOnly, Category = "Banner|Layout", meta = (ClampMin = "1"))
	int32 TitleLabelFontSize = 16;

	UPROPERTY(EditDefaultsOnly, Category = "Banner|Layout", meta = (ClampMin = "1"))
	int32 TitleNameFontSize = 36;

	/** 클리어 글자 아래 선 길이 (px) */
	UPROPERTY(EditDefaultsOnly, Category = "Banner|Layout", meta = (ClampMin = "0.0"))
	float ClearLineWidth = 560.0f;

	/** 진입 이름 양옆 선 길이 (px) */
	UPROPERTY(EditDefaultsOnly, Category = "Banner|Layout", meta = (ClampMin = "0.0"))
	float TitleLineWidth = 64.0f;

	/** 게임 UI 하늘색 (선·윗줄) */
	UPROPERTY(EditDefaultsOnly, Category = "Banner|Color")
	FLinearColor AccentColor;

	UPROPERTY(EditDefaultsOnly, Category = "Banner|Color")
	FLinearColor TextColor;

	UPROPERTY(EditDefaultsOnly, Category = "Banner|Color")
	FLinearColor SubTextColor;

	UPROPERTY(EditDefaultsOnly, Category = "Banner|Color")
	FLinearColor WallColor;

	/** 워프 섬광 색 (하늘빛 흰색) */
	UPROPERTY(EditDefaultsOnly, Category = "Banner|Color")
	FLinearColor FlashColor;

	/** 흐려지는 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "Banner", meta = (ClampMin = "0.0"))
	float FadeSeconds = 0.4f;

private:
	UPROPERTY()
	TObjectPtr<UFont> Font;

	UPROPERTY()
	TObjectPtr<UWidget> ClearBand;

	UPROPERTY()
	TObjectPtr<UTextBlock> ClearLabelText;

	UPROPERTY()
	TObjectPtr<UTextBlock> ClearText;

	UPROPERTY()
	TObjectPtr<UWidget> TitleBox;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleLabelText;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleNameText;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleWallText;

	UPROPERTY()
	TObjectPtr<UImage> FlashImage;

	/** 지금 보이는 쪽 */
	UPROPERTY()
	TObjectPtr<UWidget> ActiveElement;

	float FadeStartTime = 0.0f;
	FTimerHandle HoldTimerHandle;
	FTimerHandle FadeTimerHandle;

	float FlashStartTime = 0.0f;
	float FlashPeak = 0.0f;
	float FlashIn = 0.0f;
	float FlashOut = 0.0f;
	FTimerHandle FlashTimerHandle;
};
