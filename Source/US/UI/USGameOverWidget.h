#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Datas/USScrapRewardData.h"
#include "UI/USRunSummaryWidget.h"
#include "USGameOverWidget.generated.h"

class UButton;
class UTextBlock;

/** 게임오버 창에서 버튼을 누름 */
DECLARE_DELEGATE(FDelegateOnHangarRequested);
DECLARE_DELEGATE(FDelegateOnTitleRequested);
DECLARE_DELEGATE(FDelegateOnRetryRequested);

/**
 * 결산 창 (격추·귀환·출격 포기 뒤): 제목, 판 요약 표(UUSRunSummaryWidget), 누적 출격·클리어 수, "격납고로"·"다시 출격"·"타이틀로".
 * 표는 클리어 창과 같은 위젯을 쓴다. 여기서는 제목·누적 줄·버튼만 맡는다.
 * (최고 기록은 격납고에서 아무 결정에도 쓰이지 않아 여기로 옮겼다 — 방금 판과 나란히 놓여야 의미가 생긴다)
 * 위젯 이름: HeaderText, RunSummary, CareerText, HangarButton, TitleButton, (선택) RetryButton.
 */
UCLASS()
class US_API UUSGameOverWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSGameOverWidget(const FObjectInitializer& ObjectInitializer);

	/** 결과 표시. 합계 스크랩은 0부터 올라간다 */
	void SetupResult(const FUSRunResult& Result, const FUSRunRecords& Records);

	/** 보스가 아닌 스테이지를 깨고 돌아옴: 제목을 "스테이지 N 클리어"로 (SetupResult 뒤에 부른다) */
	void SetStageClearTitle(int32 StageNumber);

	/** "격납고로" 클릭 */
	FDelegateOnHangarRequested OnHangarRequested;

	/** "타이틀로" 클릭 */
	FDelegateOnTitleRequested OnTitleRequested;

	/** "다시 출격" 클릭 (끝난 스테이지를 같은 편대로 다시 연다) */
	FDelegateOnRetryRequested OnRetryRequested;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleHangarClicked();

	UFUNCTION()
	void HandleTitleClicked();

	UFUNCTION()
	void HandleRetryClicked();

	/** 끝난 이유·클리어 여부에 맞는 제목 */
	const FText& GetHeaderTitle(const FUSRunResult& Result) const;

	/** 제목: 격추 */
	UPROPERTY(EditDefaultsOnly, Category = "GameOver|Text")
	FText DestroyedTitle;

	/** 제목: 클리어 후 나가기 */
	UPROPERTY(EditDefaultsOnly, Category = "GameOver|Text")
	FText ReturnedTitle;

	/** 제목: 클리어 후 계속 가다 격추 */
	UPROPERTY(EditDefaultsOnly, Category = "GameOver|Text")
	FText ClearedDestroyedTitle;

	/** 제목: 스테이지 클리어 뒤 귀환. {0} = 스테이지 번호 */
	UPROPERTY(EditDefaultsOnly, Category = "GameOver|Text")
	FText StageClearTitleFormat;

	/** 제목: 일시정지 창에서 출격 포기 */
	UPROPERTY(EditDefaultsOnly, Category = "GameOver|Text")
	FText AbandonedTitle;

	/** 표 맨 아래 줄 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "GameOver|Text")
	FText TotalLabel;

	/** 제목 색: 귀환·클리어 (실패처럼 읽히지 않게 격추와 다른 색) */
	UPROPERTY(EditDefaultsOnly, Category = "GameOver|Color")
	FSlateColor SuccessTitleColor = FLinearColor(0.37f, 0.83f, 0.93f, 1.0f);

	/** 제목 색: 격추·출격 포기 */
	UPROPERTY(EditDefaultsOnly, Category = "GameOver|Color")
	FSlateColor FailTitleColor = FLinearColor(0.95f, 0.47f, 0.42f, 1.0f);

	/** 누적 줄. {0} = 출격 수, {1} = 클리어 수 */
	UPROPERTY(EditDefaultsOnly, Category = "GameOver|Text")
	FText CareerFormat;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> HeaderText;

	/** 판 요약 표 (WBP_USRunSummary) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUSRunSummaryWidget> RunSummary;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CareerText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HangarButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TitleButton;

	/** 선택: 없으면 R 키 재시작만 남는다 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> RetryButton;
};
