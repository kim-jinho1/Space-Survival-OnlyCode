#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/USKeyMenu.h"
#include "USTitleWidget.generated.h"

class UButton;
class UTextBlock;
class UUSCreditsWidget;
class UUSSettingsWidget;

/** 타이틀 화면에서 버튼을 누름 */
DECLARE_DELEGATE(FDelegateOnStartRequested);
DECLARE_DELEGATE(FDelegateOnQuitRequested);

/**
 * 타이틀 화면: 게임 제목(WBP에 배치한 글자)과 "시작"·"설정"·"크레딧"·"종료" 버튼.
 * 버튼 클릭은 델리게이트로 타이틀 컨트롤러에 알린다. 설정은 이 WBP 안에 숨겨 둔 WBP_USSettings를 직접 연다 (일시정지 창과 같은 위젯).
 * 크레딧도 같은 방식으로 숨겨 둔 WBP_USCredits를 연다.
 * 메뉴는 "지금 고른 항목" 하나를 들고 있다(FUSKeyMenu, 일시정지 창과 같다): ↑/↓(W/S, 게임패드 십자키)로 옮기고 Enter/Space(게임패드 A)로 실행, 마우스를 올려도 옮긴다.
 * 고른 항목만 채운 스타일(WBP의 StartButton 모양)로, 나머지는 글자만(WBP의 SettingsButton 모양)으로 칠한다.
 * 키 입력은 이 위젯이 받는다(버튼은 WBP에서 포커스를 꺼 둔다 — 버튼이 포커스를 가져가면 Enter가 화면에 보이는 고른 항목과 다른 버튼을 누를 수 있다).
 * 오른쪽 아래 버전 글자는 프로젝트 설정 ProjectVersion(DefaultGame.ini)에서 채운다 — 빌드마다 손으로 고치지 않게.
 * 위젯 이름: StartButton, SettingsButton, CreditsButton, QuitButton, SettingsPanel, CreditsPanel, (선택) VersionText.
 */
UCLASS()
class US_API UUSTitleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** "시작" 클릭 */
	FDelegateOnStartRequested OnStartRequested;

	/** "종료" 클릭 */
	FDelegateOnQuitRequested OnQuitRequested;

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** 고른 항목을 실행한다 (클릭과 같은 처리) */
	void ActivateSelected();

	/** 설정·크레딧 창이 열려 있는가 (열려 있으면 메뉴 키를 무시한다) */
	bool IsPanelOpen() const;

	/** 설정·크레딧 창이 닫힘 → 타이틀에 키보드 포커스를 돌려준다 */
	void HandlePanelClosed();

	UFUNCTION()
	void HandleStartHovered();

	UFUNCTION()
	void HandleSettingsHovered();

	UFUNCTION()
	void HandleCreditsHovered();

	UFUNCTION()
	void HandleQuitHovered();

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleCreditsClicked();

	UFUNCTION()
	void HandleQuitClicked();

	/** 고른 항목 글자색 (채운 바탕 위라 어둡게) */
	UPROPERTY(EditDefaultsOnly, Category = "Title|Menu")
	FSlateColor SelectedLabelColor = FLinearColor(0.0f, 0.02f, 0.03f, 1.0f);

	/** 나머지 항목 글자색 */
	UPROPERTY(EditDefaultsOnly, Category = "Title|Menu")
	FSlateColor IdleLabelColor = FLinearColor(0.7f, 0.78f, 0.88f, 1.0f);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> StartButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CreditsButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> QuitButton;

	/** 설정 창 (WBP_USSettings를 자식으로 넣은 것) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUSSettingsWidget> SettingsPanel;

	/** 크레딧 창 (WBP_USCredits를 자식으로 넣은 것) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUSCreditsWidget> CreditsPanel;

	/** 버전 글자. {0} = ProjectVersion */
	UPROPERTY(EditDefaultsOnly, Category = "Title|Text")
	FText VersionFormat = NSLOCTEXT("USTitleWidget", "VersionFormat", "데모 v{0}");

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VersionText;

private:
	/** 메뉴 고른 항목 (위에서부터 시작·설정·크레딧·종료) */
	FUSKeyMenu Menu;
};
