#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Datas/USScrapRewardData.h"
#include "UI/USRunSummaryWidget.h"
#include "USClearWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UTexture2D;

/** 클리어 창에서 버튼을 누름 */
DECLARE_DELEGATE(FDelegateOnLeaveRequested);
DECLARE_DELEGATE(FDelegateOnContinueRequested);

/**
 * 클리어 창: 보스를 격파하면 뜬다. "격납고로"(LeaveButton, 기록 후 바로 격납고 — 결산 창 생략, 2026-10-27) / "계속 가기"(적이 점점 강해지는 대신 구간마다 보너스).
 * 결산 창과 같은 판 요약 표(UUSRunSummaryWidget)로 "지금까지" 번 스크랩을 보여 줘, 나갈지 더 갈지를 숫자를 보고 고르게 한다.
 * 컨트롤러가 그 시점 결과를 한 번 넣어 주고(SetupClear), 버튼 클릭은 델리게이트로 알린다. 이 결과는 기록에 저장되지 않는다.
 * 데모 빌드에서는 표 아래에 "데모는 여기까지" 띠를 보여 주고 계속 가기(버튼·힌트)는 숨긴다(출시 전 체크 R-15 사용자 결정 A, R-41 4). 정식판에서는 WBP에서 DemoNoticeText를 빼거나 문구를 비운다.
 * 보스를 격파해 클리어하면(R-36, 목업 A) 표 옆에 격파한 보스 그림·이름 칸을 보인다(ShowDefeatedBoss). 보스 없이 클리어하면 칸은 숨긴 채.
 * 스테이지 판에서는 보스(행성) 스테이지 클리어에만 뜬다. 보통 스테이지는 클리어 연출 뒤 바로 결산으로 간다(2026-10-27, 판마다 처음부터).
 * 위젯 이름: RunSummary, LeaveButton, ContinueButton, (선택) SubtitleText, ContinueHintText, DemoNoticeText, DemoNoticePanel, BossPanel, BossImage, BossNameText.
 */
UCLASS()
class US_API UUSClearWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSClearWidget(const FObjectInitializer& ObjectInitializer);

	/** 지금까지의 결과와 계속 가기 안내(구간 보너스, 구간 길이 초) */
	void SetupClear(const FUSRunResult& Result, const FUSRunRecords& Records, int32 ContinueBonusPerStep, float StepSeconds);

	/** 격파한 보스 칸을 채워 보인다 (SetupClear 뒤에 부른다). Image가 없으면 그림만 숨긴다 */
	void ShowDefeatedBoss(UTexture2D* Image, const FText& BossName);

	/** "격납고로" 클릭 */
	FDelegateOnLeaveRequested OnLeaveRequested;

	/** "계속 가기" 클릭 */
	FDelegateOnContinueRequested OnContinueRequested;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleLeaveClicked();

	UFUNCTION()
	void HandleContinueClicked();

	/** 제목 아래 한 줄 */
	UPROPERTY(EditDefaultsOnly, Category = "Clear|Text")
	FText Subtitle;

	/** 표 맨 아래 줄 이름 (아직 판이 끝나지 않았으므로 "합계"가 아니다) */
	UPROPERTY(EditDefaultsOnly, Category = "Clear|Text")
	FText TotalLabel;

	/** "계속 가기" 버튼 아래 안내. {0} = 구간 길이(초), {1} = 구간 보너스 */
	UPROPERTY(EditDefaultsOnly, Category = "Clear|Text")
	FText ContinueHintFormat;

	/** 데모 끝 안내 (비우면 띠를 숨긴다) */
	UPROPERTY(EditDefaultsOnly, Category = "Clear|Text")
	FText DemoNotice;

	/** 격파 칸 이름 줄. {0} = 보스 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "Clear|Text")
	FText DefeatedBossFormat;

	/** 판 요약 표 (WBP_USRunSummary) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUSRunSummaryWidget> RunSummary;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ContinueHintText;

	/** 데모 끝 띠 글자 (띠 테두리는 WBP에서 이 글자를 감싼 Border) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DemoNoticeText;

	/** 데모 끝 띠 전체 (문구가 비면 같이 숨긴다) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DemoNoticePanel;

	/** 격파한 보스 칸 전체 (보스 없이 클리어하면 숨김) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BossPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> BossImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BossNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> LeaveButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ContinueButton;
};
