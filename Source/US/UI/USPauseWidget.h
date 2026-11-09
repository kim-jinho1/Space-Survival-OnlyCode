#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/USKeyMenu.h"
#include "Containers/Ticker.h"
#include "USPauseWidget.generated.h"

class UButton;
class UTextBlock;
class UUSSettingsWidget;
class UUSEnemyTypeData;

/** 일시정지 창에서 버튼을 누름 */
DECLARE_DELEGATE(FDelegateOnResumeRequested);
DECLARE_DELEGATE(FDelegateOnAbandonRequested);
DECLARE_DELEGATE(FDelegateOnQuitRequested);

/** 일시정지 창 오른쪽 "이번 출격" 칸에 보일 값 (창을 열 때 컨트롤러가 채운다. 게임이 멈춰 있어 열려 있는 동안 바뀌지 않는다) */
struct FUSPauseSnapshot
{
	/** bContinuing이 false면 남은 시간, true면 계속 가기로 더 버틴 시간 */
	float ClockSeconds = 0.0f;
	bool bContinuing = false;
	int32 Level = 0;
	int32 KillCount = 0;
	int32 Gold = 0;
	/** 살아 있는 아군 / 최대 아군 */
	int32 AllyCount = 0;
	int32 MaxAllies = 0;
	/** 이번 판에 만난 적 종류, 처음 나온 순서 (R-41 5) */
	TArray<const UUSEnemyTypeData*> MetEnemies;
};

/**
 * 일시정지 창 (Esc / P): 왼쪽 메뉴 "계속", "설정", "출격 포기"(그때까지 번 스크랩으로 결산), "게임 종료"(스크랩 기록 후 종료, 두 번 눌러야 함) + 오른쪽 "이번 출격" 현황 칸.
 * 설정을 고르면 현황 칸을 숨기고 같은 자리에 이 WBP 안에 숨겨 둔 WBP_USSettings를 연다 (타이틀과 같은 위젯).
 * 메뉴는 타이틀과 같이 ↑/↓·Enter로 고른다(FUSKeyMenu) — 컨트롤러가 창을 띄운 뒤 키보드 포커스를 준다.
 * 버튼 클릭은 델리게이트로 컨트롤러에 알린다.
 * 위젯 이름: ResumeButton, SettingsButton, AbandonButton, (선택) QuitButton·QuitLabel, SettingsPanel, (선택) StatsPanel, ClockLabelText, ClockText, LevelText, KillText, GoldText, AllyText, MetEnemiesText.
 * MetEnemiesText: 이번 판에 만난 적 "이름 — 특징" 한 줄씩 (R-41 5, 사용자 결정 A: 적 설명은 싸우는 중이 아니라 멈췄을 때 읽는다. Hades 일시정지 도감처럼).
 */
UCLASS()
class US_API UUSPauseWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSPauseWidget(const FObjectInitializer& ObjectInitializer);

	/** Esc / P를 다시 누름: 설정 창이 열려 있으면 그것만 닫고, 아니면 계속 */
	void HandleBackRequested();

	/** "이번 출격" 칸을 채운다 */
	void ShowSnapshot(const FUSPauseSnapshot& Snapshot);

	/** "계속" */
	FDelegateOnResumeRequested OnResumeRequested;

	/** "출격 포기" */
	FDelegateOnAbandonRequested OnAbandonRequested;

	/** "게임 종료" (두 번째 누름) */
	FDelegateOnQuitRequested OnQuitRequested;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** 설정 창이 닫힘 → 현황 칸을 다시 보이고 메뉴에 키보드 포커스를 돌려준다 */
	void HandleSettingsClosed();

	UFUNCTION()
	void HandleResumeClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleAbandonClicked();

	/** 처음 누르면 확인 글자로 바꾸고, QuitConfirmSeconds 안에 다시 누르면 종료를 알린다 (설정 창 "진행 초기화"와 같은 방식) */
	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleQuitHovered();

	/** 확인 시간이 지남 → 글자를 되돌린다. 게임이 멈춰 있어 월드 타이머 대신 실제 시간 티커를 쓴다 */
	bool HandleQuitConfirmExpired(float DeltaTime);

	UFUNCTION()
	void HandleResumeHovered();

	UFUNCTION()
	void HandleSettingsHovered();

	UFUNCTION()
	void HandleAbandonHovered();

	/** 고른 항목 글자색 (채운 바탕 위라 어둡게) */
	UPROPERTY(EditDefaultsOnly, Category = "Pause|Menu")
	FSlateColor SelectedLabelColor = FLinearColor(0.0f, 0.02f, 0.03f, 1.0f);

	/** 나머지 항목 글자색 */
	UPROPERTY(EditDefaultsOnly, Category = "Pause|Menu")
	FSlateColor IdleLabelColor = FLinearColor(0.7f, 0.78f, 0.88f, 1.0f);

	/** 시계 줄 이름: 남은 시간 / 계속 가기 중 */
	UPROPERTY(EditDefaultsOnly, Category = "Pause|Text")
	FText RemainingLabel;

	UPROPERTY(EditDefaultsOnly, Category = "Pause|Text")
	FText ContinueLabel;

	/** "게임 종료" 버튼 글자 / 한 번 누른 뒤 글자 */
	UPROPERTY(EditDefaultsOnly, Category = "Pause|Text")
	FText QuitLabelText;

	UPROPERTY(EditDefaultsOnly, Category = "Pause|Text")
	FText QuitConfirmLabelText;

	/** 두 번째 누름을 기다리는 시간(실제 초) */
	UPROPERTY(EditDefaultsOnly, Category = "Pause|Menu", meta = (ClampMin = "0.5"))
	float QuitConfirmSeconds = 3.0f;

	/** 시계 값. {0} = 분, {1} = 초(두 자리). 계속 가기 중엔 ContinueClockFormat */
	UPROPERTY(EditDefaultsOnly, Category = "Pause|Text")
	FText ClockFormat;

	UPROPERTY(EditDefaultsOnly, Category = "Pause|Text")
	FText ContinueClockFormat;

	/** 아군 줄. {0} = 살아 있는 수, {1} = 최대 */
	UPROPERTY(EditDefaultsOnly, Category = "Pause|Text")
	FText AllyFormat;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ResumeButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> AbandonButton;

	/** 고르지 않았을 때는 WBP에 칠해 둔 빨간 테두리 모양을 그대로 쓴다 (FUSKeyMenu OwnIdleStyleButtons) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuitLabel;

	/** 설정 창 (WBP_USSettings를 자식으로 넣은 것) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUSSettingsWidget> SettingsPanel;

	/** "이번 출격" 칸 (설정이 열려 있는 동안 숨긴다) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> StatsPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ClockLabelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ClockText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> KillText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GoldText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AllyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MetEnemiesText;

	/** 만난 적 한 줄. {0} = 이름, {1} = 특징 (특징이 비면 이름만) */
	UPROPERTY(EditDefaultsOnly, Category = "Pause|Text")
	FText MetEnemyLineFormat = NSLOCTEXT("USPauseWidget", "MetEnemyLineFormat", "{0} — {1}");

	/** 아직 만난 적이 없을 때 */
	UPROPERTY(EditDefaultsOnly, Category = "Pause|Text")
	FText NoMetEnemiesText = NSLOCTEXT("USPauseWidget", "NoMetEnemiesText", "아직 만난 적이 없습니다");

private:
	/** 메뉴 고른 항목 (위에서부터 계속·설정·출격 포기·게임 종료) */
	FUSKeyMenu Menu;

	/** "게임 종료"를 한 번 눌러 두 번째를 기다리는 중 */
	bool bQuitArmed = false;
	FTSTicker::FDelegateHandle QuitConfirmTickerHandle;
};
