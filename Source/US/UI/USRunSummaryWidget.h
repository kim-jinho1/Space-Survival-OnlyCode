#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Datas/USScrapRewardData.h"
#include "USRunSummaryWidget.generated.h"

class UTextBlock;
class UWidget;

/** 판 요약 표에 같이 보여 줄 누적 기록. 최고 기록은 이번 판을 기록하기 "전" 값이어야 갱신 여부를 가릴 수 있다 */
struct FUSRunRecords
{
	/** 메타 진행 기록이 있는가 (없으면 최고 열·누적 줄을 숨긴다) */
	bool bValid = false;

	/** 이번 판 전까지의 최장 생존(초) */
	float PreviousBestSeconds = 0.0f;

	/** 이번 판 전까지의 최다 처치 */
	int32 PreviousBestKills = 0;

	/** 이번 판을 포함한 누적 출격·클리어 수 (결산 창만 쓴다) */
	int32 RunCount = 0;
	int32 ClearCount = 0;
};

/**
 * 판 요약 표: 줄 = 생존·처치·클리어·계속 가기·레벨, 열 = 이번 / 최고 / 스크랩, 맨 아래 합계.
 * 결산 창(UUSGameOverWidget)과 클리어 창(UUSClearWidget)이 같이 쓴다 — 표를 한 곳에서 그려야 두 창 모양이 어긋나지 않는다.
 * 최고 열에는 이번 판 "전" 기록을 두고, 이번 판이 넘었으면 "갱신" 표시를 켠다. 합계는 0부터 올라가는 연출을 할 수 있다.
 * 결산·클리어 창이 떠 있는 동안 게임은 일시정지라 월드 타이머가 멈추므로 연출은 NativeTick(실제 시간)으로 한다.
 * 위젯 이름: SurvivalTimeText, BestTimeText, TimeScrapText, NewBestTimeBadge, KillText, BestKillText, KillScrapText, NewBestKillBadge,
 * ClearText, ClearScrapText, ContinueRow, ContinueTimeText, ContinueScrapText, LevelText, TotalLabelText, TotalScrapText, (선택) BestHeader.
 */
UCLASS()
class US_API UUSRunSummaryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSRunSummaryWidget(const FObjectInitializer& ObjectInitializer);

	/** 표를 채운다. TotalLabel = 맨 아래 줄 이름("합계"·"지금까지"), bCountUp이면 합계가 0부터 올라간다 */
	void SetupSummary(const FUSRunResult& Result, const FUSRunRecords& Records, const FText& TotalLabel, bool bCountUp);

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 최고 열·갱신 표시 */
	void SetupRecordColumn(const FUSRunResult& Result, const FUSRunRecords& Records);

	/** 합계 칸에 Value를 쓴다 */
	void SetTotalScrapText(int32 Value) const;

	/** 시간 글자 "m:ss". {0} = 분, {1} = 초(두 자리) */
	UPROPERTY(EditDefaultsOnly, Category = "RunSummary|Text")
	FText TimeFormat;

	/** 스크랩 칸 글자. {0} = 스크랩 */
	UPROPERTY(EditDefaultsOnly, Category = "RunSummary|Text")
	FText ScrapFormat;

	/** 클리어 줄 "이번" 칸 (클리어함 / 못 함) */
	UPROPERTY(EditDefaultsOnly, Category = "RunSummary|Text")
	FText ClearedText;

	UPROPERTY(EditDefaultsOnly, Category = "RunSummary|Text")
	FText NotClearedText;

	/** 스테이지 판의 클리어 줄 "이번" 칸. {0} = 깬 스테이지 수 */
	UPROPERTY(EditDefaultsOnly, Category = "RunSummary|Text")
	FText StagesClearedFormat;

	/** 클리어 줄 "이번" 칸 글자색 (달성 / 못 함). 못 함은 다른 빈 칸("-")과 같은 흐린 색 */
	UPROPERTY(EditDefaultsOnly, Category = "RunSummary|Color")
	FSlateColor ClearedColor = FLinearColor(0.37f, 0.83f, 0.93f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "RunSummary|Color")
	FSlateColor NotClearedColor = FLinearColor(0.5f, 0.58f, 0.66f, 1.0f);

	/** 이전 기록이 없을 때 최고 칸 */
	UPROPERTY(EditDefaultsOnly, Category = "RunSummary|Text")
	FText NoRecordText;

	/** 합계가 0부터 올라가는 시간(초). 0이면 바로 최종값 */
	UPROPERTY(EditDefaultsOnly, Category = "RunSummary", meta = (ClampMin = "0.0"))
	float CountUpDuration = 1.2f;

	/** "최고" 열 머리글 (기록이 없으면 숨긴다) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> BestHeader;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SurvivalTimeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BestTimeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TimeScrapText;

	/** 생존 시간이 최고 기록을 넘었을 때 켜는 "갱신" 표시 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> NewBestTimeBadge;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KillText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BestKillText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KillScrapText;

	/** 처치 수가 최고 기록을 넘었을 때 켜는 "갱신" 표시 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> NewBestKillBadge;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ClearText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ClearScrapText;

	/** 계속 가기 줄 전체 (계속 가기를 안 한 판에는 숨긴다) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> ContinueRow;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ContinueTimeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ContinueScrapText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TotalLabelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TotalScrapText;

private:
	/** Seconds를 TimeFormat 글자로 */
	FText FormatTime(float Seconds) const;

	/** Scrap을 ScrapFormat 글자로 */
	FText FormatScrap(int32 Scrap) const;

	/** 합계 최종값 */
	int32 TargetScrap = 0;

	/** 올리기 시작한 뒤 흐른 시간(초). 음수면 연출 없음 */
	float CountUpElapsed = -1.0f;
};
