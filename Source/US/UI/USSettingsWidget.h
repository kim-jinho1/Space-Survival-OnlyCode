#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Containers/Ticker.h"
#include "Types/SlateEnums.h"
#include "USSettingsWidget.generated.h"

class UButton;
class UComboBoxString;
class USlider;
class UTextBlock;
class UUSGameUserSettings;
class UWidget;
class UWidgetSwitcher;

/** 설정 창을 닫음 */
DECLARE_DELEGATE(FDelegateOnSettingsClosed);

/**
 * 설정 창 (타이틀·일시정지 메뉴 공용). 부모 WBP 안에 숨긴 채 넣어 두고 Open으로 연다.
 * 음량 슬라이더는 움직이는 즉시 들리고, 창 모드·해상도·그래픽 품질은 "적용"을 눌러야 바뀐다 (해상도는 화면이 깜빡이므로 한 번에).
 * "닫기"는 음량을 파일에 저장하고, 적용하지 않은 화면 설정은 버린다 (다음에 열면 실제 값으로 다시 채운다).
 * 루트는 화면 전체를 덮는 배경(Backdrop)이라 뒤의 메뉴 버튼이 눌리지 않는다. 배경을 어둡게 칠할지는 넣은 곳마다 다르다(bDimBackground):
 * 타이틀은 어둡게, 일시정지는 이미 일시정지 창이 어둡게 덮고 있고 설정을 오른쪽 칸에만 열어 투명하게 (어두운 칸이 겹치면 경계에 세로 이음매가 보인다).
 * 화면 흔들림·데미지 숫자는 음량처럼 고르는 즉시 반영되고, 수직 동기화는 화면 설정이라 "적용"에 묶는다 (출시 전 체크 R-20).
 * "진행 초기화"는 두 번 눌러야 지운다 — 첫 클릭은 글자만 "한 번 더 누르면 초기화"로 바꾸고 ResetConfirmSeconds 안에 다시 눌러야 실행 (R-19).
 * 창 모드·해상도를 바꿔 적용하면 "유지할까요?" 줄이 VideoConfirmSeconds 동안 뜨고, "유지"를 안 누르면 전 화면으로 되돌린다 (R-21) —
 * 모니터가 못 받는 해상도라 화면이 안 보여도 기다리면 돌아온다. 엔진의 ConfirmVideoMode/RevertVideoMode(마지막 확정 값)를 쓴다.
 * 위젯 이름: MasterVolumeSlider, MusicVolumeSlider, SfxVolumeSlider, WindowModeCombo, ResolutionCombo, QualityCombo,
 * ApplyButton, CloseButton. 선택: MasterVolumeText, MusicVolumeText, SfxVolumeText (퍼센트 표시), Backdrop,
 * ScreenShakeCombo, DamageNumbersCombo, VSyncCombo (켜기/끄기 칸), ResetProgressRow (초기화 줄 전체), ResetProgressButton, ResetProgressText,
 * VideoConfirmRow (유지 확인 줄 전체), VideoConfirmText, VideoKeepButton, VideoRevertButton.
 */
UCLASS()
class US_API UUSSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSSettingsWidget(const FObjectInitializer& ObjectInitializer);

	/** 현재 설정으로 칸을 채우고 보인다 */
	void Open();

	/** 음량을 저장하고 숨긴다 */
	void Close();

	/** 열려 있는가 (Esc가 설정 창을 먼저 닫게) */
	bool IsOpen() const;

	/** 닫힘 */
	FDelegateOnSettingsClosed OnClosed;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 탭 Index(0 음량 · 1 화면 · 2 게임)를 보이고 탭 버튼을 다시 칠한다 */
	void SelectTab(int32 Index);

	UFUNCTION()
	void HandleVolumeTabClicked();

	UFUNCTION()
	void HandleDisplayTabClicked();

	UFUNCTION()
	void HandleGameplayTabClicked();

	UFUNCTION()
	void HandleMasterVolumeChanged(float Value);

	UFUNCTION()
	void HandleMusicVolumeChanged(float Value);

	UFUNCTION()
	void HandleSfxVolumeChanged(float Value);

	UFUNCTION()
	void HandleWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleScreenShakeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleDamageNumbersChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void HandleResetProgressClicked();

	UFUNCTION()
	void HandleApplyClicked();

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleVideoKeepClicked();

	UFUNCTION()
	void HandleVideoRevertClicked();

	/** 유지 확인 줄을 띄우고 실제 시간으로 센다 */
	void StartVideoConfirm();

	/** 확인 줄을 숨기고 세기를 멈춘다 (값은 건드리지 않는다) */
	void StopVideoConfirm();

	/** 마지막으로 확정한 창 모드·해상도로 되돌려 적용한다 */
	void RevertVideoMode();

	/** 남은 초를 글자에 쓰고, 다 되면 되돌린다. FTSTicker라 일시정지 중에도 돈다 */
	bool TickVideoConfirm(float DeltaTime);

	/** 확인을 기다리는 중인가 */
	bool IsVideoConfirmPending() const { return VideoConfirmTickerHandle.IsValid(); }

	/** 남은 초 글자 갱신 */
	void UpdateVideoConfirmText() const;

	/** 설정 객체의 실제 값으로 모든 칸을 채운다 */
	void RefreshFromSettings(const UUSGameUserSettings& Settings);

	/** 해상도 목록을 만든다 (모니터가 지원하는 전체 화면 해상도 + 현재 해상도) */
	void RefreshResolutionOptions(const FIntPoint& CurrentResolution);

	/** 전체 화면(창)은 바탕화면 해상도를 쓰므로 해상도 칸을 잠근다 */
	void RefreshResolutionEnabled();

	/** 퍼센트 글자 갱신 (선택 위젯이라 없으면 넘어간다) */
	void SetVolumeText(UTextBlock* Text, float Volume) const;

	/** 켜기/끄기 칸을 채우고 값을 고른다 (0 = 켜기, 1 = 끄기) */
	void FillToggleCombo(UComboBoxString* Combo, bool bEnabled) const;

	/** 켜기를 골랐는가 (칸이 없으면 bFallback) */
	static bool IsToggleOn(const UComboBoxString* Combo, bool bFallback);

	/** 초기화 버튼 글자를 처음 상태로 (확인 대기 해제) */
	UFUNCTION()
	void ResetProgressPrompt();

	/** 창 모드 이름. cpp의 SettingsWindowModes 순서 (전체 화면(창), 전체 화면, 창) */
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Text")
	TArray<FText> WindowModeLabels;

	/** 그래픽 품질 이름. 0 = 낮음 ~ 3 = 최고 (엔진 스케일러빌리티 단계) */
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Text")
	TArray<FText> QualityLabels;

	/** 해상도 칸. {0} = 가로, {1} = 세로 */
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Text")
	FText ResolutionFormat;

	/** 음량 퍼센트. {0} = 0~100 */
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Text")
	FText VolumeFormat;

	/** 켜기/끄기 칸 글자 */
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Text")
	FText ToggleOnLabel;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Text")
	FText ToggleOffLabel;

	/** 초기화 버튼 글자: 평소 / 확인 대기 / 지운 뒤 */
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Text")
	FText ResetProgressLabel;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Text")
	FText ResetProgressConfirmLabel;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Text")
	FText ResetProgressDoneLabel;

	/** 화면 유지 확인 글자. {0} = 남은 초 */
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Text")
	FText VideoConfirmFormat;

	/** 화면을 바꾼 뒤 이 시간(초) 안에 "유지"를 안 누르면 되돌린다 */
	UPROPERTY(EditDefaultsOnly, Category = "Settings", meta = (ClampMin = "3.0"))
	float VideoConfirmSeconds = 15.0f;

	/** 첫 클릭 뒤 이 시간(초) 안에 다시 눌러야 지운다 */
	UPROPERTY(EditDefaultsOnly, Category = "Settings", meta = (ClampMin = "0.5"))
	float ResetConfirmSeconds = 4.0f;

	/** 진행 초기화 줄을 보이는가. 타이틀만 켠다 — 판 도중(일시정지)에 지우면 그 판 결과가 빈 세이브에 기록된다 */
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bShowResetProgress = false;

	/** 뒤를 어둡게 덮는가. 부모 WBP에 넣은 인스턴스마다 정한다 (일시정지 창 안에서는 끈다) */
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bDimBackground = true;

	/** 화면 전체를 덮는 배경 (클릭 막기 + 어둡게) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> Backdrop;

	/**
	 * 탭: 1.5배 글자로 한 칸에 다 넣으면 1080 화면보다 길어져서(위·아래 잘림) 음량·화면·게임 세 탭으로 나눴다.
	 * 탭은 바탕 없는 글자 + 밑줄(사용자 결정 A안). 고른 탭만 글자색을 바꾸고 밑줄을 보인다
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidgetSwitcher> TabSwitcher;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> VolumeTabButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DisplayTabButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> GameplayTabButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VolumeTabLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DisplayTabLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GameplayTabLabel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> VolumeTabUnderline;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DisplayTabUnderline;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> GameplayTabUnderline;

	/** 고른 탭 글자색 (밑줄과 같은 청록) */
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Tabs")
	FSlateColor SelectedTabLabelColor = FLinearColor(0.25f, 0.85f, 0.95f, 1.0f);

	/** 나머지 탭 글자색 */
	UPROPERTY(EditDefaultsOnly, Category = "Settings|Tabs")
	FSlateColor IdleTabLabelColor = FLinearColor(0.62f, 0.71f, 0.78f, 1.0f);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> MasterVolumeSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> MusicVolumeSlider;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> SfxVolumeSlider;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MasterVolumeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MusicVolumeText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SfxVolumeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> WindowModeCombo;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> ResolutionCombo;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> QualityCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> ScreenShakeCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> DamageNumbersCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UComboBoxString> VSyncCombo;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ResetProgressRow;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResetProgressButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResetProgressText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> VideoConfirmRow;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VideoConfirmText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> VideoKeepButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> VideoRevertButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ApplyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

private:
	/** 해상도 칸의 각 줄이 가리키는 해상도 (칸 순서와 같다) */
	TArray<FIntPoint> Resolutions;

	/** 초기화 첫 클릭 시각 (실제 시간, 0 = 대기 아님) */
	double ResetArmedRealTime = 0.0;

	/** 확인 대기 글자를 되돌리는 타이머 */
	FTimerHandle ResetPromptTimerHandle;

	/** 화면 유지 확인 시계 (실제 시간, 유효 = 확인 대기 중) */
	FTSTicker::FDelegateHandle VideoConfirmTickerHandle;

	/** 이 시각(FPlatformTime::Seconds)이 지나면 되돌린다 */
	double VideoConfirmDeadline = 0.0;
};
