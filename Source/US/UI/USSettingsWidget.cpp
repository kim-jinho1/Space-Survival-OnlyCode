#include "UI/USSettingsWidget.h"
#include "US.h"
#include "Core/USGameUserSettings.h"
#include "Progression/USMetaProgressionSubsystem.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Components/WidgetSwitcher.h"
#include "Components/TextBlock.h"
#include "Containers/Ticker.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	/** 창 모드 칸 순서 = 이 배열 순서 (WindowModeLabels와 맞춘다) */
	const EWindowMode::Type SettingsWindowModes[] = { EWindowMode::WindowedFullscreen, EWindowMode::Fullscreen, EWindowMode::Windowed };
}

UUSSettingsWidget::UUSSettingsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	WindowModeLabels = {
		NSLOCTEXT("USSettingsWidget", "WindowedFullscreen", "전체 화면(창)"),
		NSLOCTEXT("USSettingsWidget", "Fullscreen", "전체 화면"),
		NSLOCTEXT("USSettingsWidget", "Windowed", "창 모드") };
	QualityLabels = {
		NSLOCTEXT("USSettingsWidget", "QualityLow", "낮음"),
		NSLOCTEXT("USSettingsWidget", "QualityMedium", "중간"),
		NSLOCTEXT("USSettingsWidget", "QualityHigh", "높음"),
		NSLOCTEXT("USSettingsWidget", "QualityEpic", "최고") };
	ResolutionFormat = NSLOCTEXT("USSettingsWidget", "ResolutionFormat", "{0} × {1}");
	VolumeFormat = NSLOCTEXT("USSettingsWidget", "VolumeFormat", "{0}%");
	ToggleOnLabel = NSLOCTEXT("USSettingsWidget", "ToggleOn", "켜기");
	ToggleOffLabel = NSLOCTEXT("USSettingsWidget", "ToggleOff", "끄기");
	ResetProgressLabel = NSLOCTEXT("USSettingsWidget", "ResetProgress", "진행 초기화");
	ResetProgressConfirmLabel = NSLOCTEXT("USSettingsWidget", "ResetProgressConfirm", "한 번 더 누르면 초기화됩니다");
	ResetProgressDoneLabel = NSLOCTEXT("USSettingsWidget", "ResetProgressDone", "초기화되었습니다");
	VideoConfirmFormat = NSLOCTEXT("USSettingsWidget", "VideoConfirmFormat", "이 설정을 유지할까요? {0}초 후 이전 설정으로 돌아갑니다");
}

void UUSSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 배경은 클릭을 막아야 하므로 숨기지 않고 투명하게만 한다
	if (Backdrop && !bDimBackground)
	{
		Backdrop->SetRenderOpacity(0.0f);
	}

	if (VolumeTabButton)
	{
		VolumeTabButton->OnClicked.AddUniqueDynamic(this, &UUSSettingsWidget::HandleVolumeTabClicked);
	}
	if (DisplayTabButton)
	{
		DisplayTabButton->OnClicked.AddUniqueDynamic(this, &UUSSettingsWidget::HandleDisplayTabClicked);
	}
	if (GameplayTabButton)
	{
		GameplayTabButton->OnClicked.AddUniqueDynamic(this, &UUSSettingsWidget::HandleGameplayTabClicked);
	}

	// BindWidget이라 WBP에 없으면 컴파일 단계에서 막히지만, 다시 열릴 때 중복 바인딩되지 않게 AddUnique
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UUSSettingsWidget::HandleMasterVolumeChanged);
	}
	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UUSSettingsWidget::HandleMusicVolumeChanged);
	}
	if (SfxVolumeSlider)
	{
		SfxVolumeSlider->OnValueChanged.AddUniqueDynamic(this, &UUSSettingsWidget::HandleSfxVolumeChanged);
	}
	if (WindowModeCombo)
	{
		WindowModeCombo->OnSelectionChanged.AddUniqueDynamic(this, &UUSSettingsWidget::HandleWindowModeChanged);
	}
	if (ScreenShakeCombo)
	{
		ScreenShakeCombo->OnSelectionChanged.AddUniqueDynamic(this, &UUSSettingsWidget::HandleScreenShakeChanged);
	}
	if (DamageNumbersCombo)
	{
		DamageNumbersCombo->OnSelectionChanged.AddUniqueDynamic(this, &UUSSettingsWidget::HandleDamageNumbersChanged);
	}
	if (ResetProgressButton)
	{
		ResetProgressButton->OnClicked.AddUniqueDynamic(this, &UUSSettingsWidget::HandleResetProgressClicked);
	}
	if (ResetProgressRow)
	{
		ResetProgressRow->SetVisibility(bShowResetProgress ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (ApplyButton)
	{
		ApplyButton->OnClicked.AddUniqueDynamic(this, &UUSSettingsWidget::HandleApplyClicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UUSSettingsWidget::HandleCloseClicked);
	}
	if (VideoKeepButton)
	{
		VideoKeepButton->OnClicked.AddUniqueDynamic(this, &UUSSettingsWidget::HandleVideoKeepClicked);
	}
	if (VideoRevertButton)
	{
		VideoRevertButton->OnClicked.AddUniqueDynamic(this, &UUSSettingsWidget::HandleVideoRevertClicked);
	}
	if (VideoConfirmRow)
	{
		VideoConfirmRow->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 부모 WBP 안에 들어 있어 처음엔 숨겨 둔다 (Open으로 연다)
	SetVisibility(ESlateVisibility::Collapsed);
}

void UUSSettingsWidget::NativeDestruct()
{
	// 확인 중에 맵을 떠나면(위젯이 사라지면) 바꾼 화면을 그대로 두지 않고 되돌린다
	if (IsVideoConfirmPending())
	{
		RevertVideoMode();
	}

	Super::NativeDestruct();
}

void UUSSettingsWidget::Open()
{
	UUSGameUserSettings* Settings = UUSGameUserSettings::Get();
	if (Settings == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] GameUserSettings is not UUSGameUserSettings. Check GameUserSettingsClassName in DefaultEngine.ini."), *GetName());
		return;
	}

	RefreshFromSettings(*Settings);
	ResetProgressPrompt();
	SelectTab(0);
	SetVisibility(ESlateVisibility::Visible);
}

void UUSSettingsWidget::SelectTab(int32 Index)
{
	if (TabSwitcher)
	{
		TabSwitcher->SetActiveWidgetIndex(Index);
	}

	// 밑줄은 Collapsed가 아니라 Hidden — 자리를 지켜 탭 글자가 위아래로 움직이지 않게
	UTextBlock* const Labels[] = { VolumeTabLabel, DisplayTabLabel, GameplayTabLabel };
	UWidget* const Underlines[] = { VolumeTabUnderline, DisplayTabUnderline, GameplayTabUnderline };
	for (int32 Tab = 0; Tab < UE_ARRAY_COUNT(Labels); ++Tab)
	{
		const bool bSelected = Tab == Index;
		if (UTextBlock* Label = Labels[Tab])
		{
			Label->SetColorAndOpacity(bSelected ? SelectedTabLabelColor : IdleTabLabelColor);
		}
		if (Underlines[Tab])
		{
			Underlines[Tab]->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		}
	}
}

void UUSSettingsWidget::HandleVolumeTabClicked()
{
	SelectTab(0);
}

void UUSSettingsWidget::HandleDisplayTabClicked()
{
	SelectTab(1);
}

void UUSSettingsWidget::HandleGameplayTabClicked()
{
	SelectTab(2);
}

void UUSSettingsWidget::Close()
{
	// 유지를 누르지 않고 닫으면 확인하지 않은 것으로 보고 되돌린다
	if (IsVideoConfirmPending())
	{
		RevertVideoMode();
	}

	// 음량은 슬라이더를 움직일 때 이미 반영됐고 여기서 파일에만 쓴다. 적용 안 한 화면 설정은 설정 객체에 넣지 않았으므로 저장되지 않는다
	if (UUSGameUserSettings* Settings = UUSGameUserSettings::Get())
	{
		Settings->SaveSettings();
	}

	ResetProgressPrompt();
	SetVisibility(ESlateVisibility::Collapsed);
	OnClosed.ExecuteIfBound();
}

bool UUSSettingsWidget::IsOpen() const
{
	return GetVisibility() != ESlateVisibility::Collapsed && GetVisibility() != ESlateVisibility::Hidden;
}

void UUSSettingsWidget::RefreshFromSettings(const UUSGameUserSettings& Settings)
{
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->SetValue(Settings.GetMasterVolume());
	}
	if (MusicVolumeSlider)
	{
		MusicVolumeSlider->SetValue(Settings.GetMusicVolume());
	}
	if (SfxVolumeSlider)
	{
		SfxVolumeSlider->SetValue(Settings.GetSfxVolume());
	}
	SetVolumeText(MasterVolumeText, Settings.GetMasterVolume());
	SetVolumeText(MusicVolumeText, Settings.GetMusicVolume());
	SetVolumeText(SfxVolumeText, Settings.GetSfxVolume());

	if (WindowModeCombo)
	{
		WindowModeCombo->ClearOptions();
		int32 SelectedIndex = 0;
		for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(SettingsWindowModes)); ++Index)
		{
			WindowModeCombo->AddOption(WindowModeLabels.IsValidIndex(Index) ? WindowModeLabels[Index].ToString() : FString::FromInt(Index));
			if (SettingsWindowModes[Index] == Settings.GetFullscreenMode())
			{
				SelectedIndex = Index;
			}
		}
		WindowModeCombo->SetSelectedIndex(SelectedIndex);
	}

	RefreshResolutionOptions(Settings.GetScreenResolution());

	if (QualityCombo)
	{
		QualityCombo->ClearOptions();
		for (const FText& Label : QualityLabels)
		{
			QualityCombo->AddOption(Label.ToString());
		}

		// 항목마다 단계가 다르면(엔진 기본값에 시네마틱이 섞이는 등) 전체 단계가 -1이라 빈 칸이 되므로, 가장 많은 단계를 보여 준다
		const int32 Level = FMath::Min(Settings.GetRepresentativeQualityLevel(), QualityLabels.Num() - 1);
		if (QualityLabels.IsValidIndex(Level))
		{
			QualityCombo->SetSelectedIndex(Level);
		}
		else
		{
			UE_LOG(LogUS, Warning, TEXT("[%s] QualityLabels is empty. Graphics quality combo shows nothing."), *GetName());
			QualityCombo->ClearSelection();
		}
	}

	// 칸을 다시 채우는 동안 바뀜 알림이 와도 같은 값이라 무해하다
	FillToggleCombo(ScreenShakeCombo, Settings.IsScreenShakeEnabled());
	FillToggleCombo(DamageNumbersCombo, Settings.IsDamageNumbersEnabled());
	FillToggleCombo(VSyncCombo, Settings.IsVSyncEnabled());

	RefreshResolutionEnabled();
}

void UUSSettingsWidget::FillToggleCombo(UComboBoxString* Combo, bool bEnabled) const
{
	if (Combo == nullptr)
	{
		return;
	}

	Combo->ClearOptions();
	Combo->AddOption(ToggleOnLabel.ToString());
	Combo->AddOption(ToggleOffLabel.ToString());
	Combo->SetSelectedIndex(bEnabled ? 0 : 1);
}

bool UUSSettingsWidget::IsToggleOn(const UComboBoxString* Combo, bool bFallback)
{
	return Combo ? Combo->GetSelectedIndex() == 0 : bFallback;
}

void UUSSettingsWidget::RefreshResolutionOptions(const FIntPoint& CurrentResolution)
{
	if (ResolutionCombo == nullptr)
	{
		return;
	}

	Resolutions.Reset();
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(Resolutions);

	// 창 모드로 드래그해 바꾼 크기처럼 목록에 없는 현재 해상도도 고를 수 있게 넣는다
	Resolutions.AddUnique(CurrentResolution);
	Resolutions.Sort([](const FIntPoint& A, const FIntPoint& B)
	{
		return A.X != B.X ? A.X < B.X : A.Y < B.Y;
	});

	ResolutionCombo->ClearOptions();
	for (const FIntPoint& Resolution : Resolutions)
	{
		ResolutionCombo->AddOption(FText::Format(ResolutionFormat, FText::AsNumber(Resolution.X, &FNumberFormattingOptions::DefaultNoGrouping()),
			FText::AsNumber(Resolution.Y, &FNumberFormattingOptions::DefaultNoGrouping())).ToString());
	}
	ResolutionCombo->SetSelectedIndex(Resolutions.IndexOfByKey(CurrentResolution));
}

void UUSSettingsWidget::RefreshResolutionEnabled()
{
	if (ResolutionCombo == nullptr || WindowModeCombo == nullptr)
	{
		return;
	}

	const int32 ModeIndex = WindowModeCombo->GetSelectedIndex();
	const bool bWindowedFullscreen = ModeIndex >= 0 && ModeIndex < static_cast<int32>(UE_ARRAY_COUNT(SettingsWindowModes))
		&& SettingsWindowModes[ModeIndex] == EWindowMode::WindowedFullscreen;
	ResolutionCombo->SetIsEnabled(!bWindowedFullscreen);
}

void UUSSettingsWidget::SetVolumeText(UTextBlock* Text, float Volume) const
{
	if (Text)
	{
		Text->SetText(FText::Format(VolumeFormat, FText::AsNumber(FMath::RoundToInt(Volume * 100.0f))));
	}
}

void UUSSettingsWidget::HandleMasterVolumeChanged(float Value)
{
	if (UUSGameUserSettings* Settings = UUSGameUserSettings::Get())
	{
		Settings->SetMasterVolume(Value);
		Settings->ApplyAudioVolumes(this);
	}
	SetVolumeText(MasterVolumeText, Value);
}

void UUSSettingsWidget::HandleMusicVolumeChanged(float Value)
{
	if (UUSGameUserSettings* Settings = UUSGameUserSettings::Get())
	{
		Settings->SetMusicVolume(Value);
		Settings->ApplyAudioVolumes(this);
	}
	SetVolumeText(MusicVolumeText, Value);
}

void UUSSettingsWidget::HandleSfxVolumeChanged(float Value)
{
	if (UUSGameUserSettings* Settings = UUSGameUserSettings::Get())
	{
		Settings->SetSfxVolume(Value);
		Settings->ApplyAudioVolumes(this);
	}
	SetVolumeText(SfxVolumeText, Value);
}

void UUSSettingsWidget::HandleWindowModeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	RefreshResolutionEnabled();
}

void UUSSettingsWidget::HandleScreenShakeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (UUSGameUserSettings* Settings = UUSGameUserSettings::Get())
	{
		Settings->SetScreenShakeEnabled(IsToggleOn(ScreenShakeCombo, true));
	}
}

void UUSSettingsWidget::HandleDamageNumbersChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (UUSGameUserSettings* Settings = UUSGameUserSettings::Get())
	{
		Settings->SetDamageNumbersEnabled(IsToggleOn(DamageNumbersCombo, true));
	}
}

void UUSSettingsWidget::HandleResetProgressClicked()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// 실제 시간으로 잰다 — 일시정지 중에 열려도 잴 수 있게
	const double Now = FPlatformTime::Seconds();
	if (ResetArmedRealTime <= 0.0 || Now - ResetArmedRealTime > ResetConfirmSeconds)
	{
		ResetArmedRealTime = Now;
		if (ResetProgressText)
		{
			ResetProgressText->SetText(ResetProgressConfirmLabel);
		}
		// 초기화 줄은 타이틀에만 보이고 타이틀은 멈추지 않으므로 월드 타이머로 글자를 되돌린다
		World->GetTimerManager().SetTimer(ResetPromptTimerHandle, this, &UUSSettingsWidget::ResetProgressPrompt, ResetConfirmSeconds, false);
		return;
	}

	World->GetTimerManager().ClearTimer(ResetPromptTimerHandle);
	ResetArmedRealTime = 0.0;

	UUSMetaProgressionSubsystem* Meta = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr;
	if (Meta == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] No meta progression subsystem. Progress was not reset."), *GetName());
		return;
	}

	Meta->ResetProgress();
	UE_LOG(LogUS, Log, TEXT("[%s] Progress reset from settings."), *GetName());
	if (ResetProgressText)
	{
		ResetProgressText->SetText(ResetProgressDoneLabel);
	}
}

void UUSSettingsWidget::ResetProgressPrompt()
{
	ResetArmedRealTime = 0.0;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ResetPromptTimerHandle);
	}
	if (ResetProgressText)
	{
		ResetProgressText->SetText(ResetProgressLabel);
	}
}

void UUSSettingsWidget::HandleApplyClicked()
{
	UUSGameUserSettings* Settings = UUSGameUserSettings::Get();
	if (Settings == nullptr)
	{
		return;
	}

	// 지금 화면을 "마지막 확정"으로 기록해 둔다 — 되돌릴 곳. 확인 대기 중에 다시 적용하면 이전 확정 값을 그대로 둔다
	if (!IsVideoConfirmPending())
	{
		Settings->ConfirmVideoMode();
	}

	const int32 ModeIndex = WindowModeCombo ? WindowModeCombo->GetSelectedIndex() : INDEX_NONE;
	if (ModeIndex >= 0 && ModeIndex < static_cast<int32>(UE_ARRAY_COUNT(SettingsWindowModes)))
	{
		const EWindowMode::Type Mode = SettingsWindowModes[ModeIndex];
		Settings->SetFullscreenMode(Mode);

		// 전체 화면(창)은 모니터 크기 창이라 고른 해상도 대신 바탕화면 해상도를 넣는다 (다른 값이면 창 모드로 돌아올 때 이상한 크기가 남는다)
		const int32 ResolutionIndex = ResolutionCombo ? ResolutionCombo->GetSelectedIndex() : INDEX_NONE;
		if (Mode == EWindowMode::WindowedFullscreen)
		{
			Settings->SetScreenResolution(Settings->GetDesktopResolution());
		}
		else if (Resolutions.IsValidIndex(ResolutionIndex))
		{
			Settings->SetScreenResolution(Resolutions[ResolutionIndex]);
		}
	}

	const int32 QualityIndex = QualityCombo ? QualityCombo->GetSelectedIndex() : INDEX_NONE;
	if (QualityLabels.IsValidIndex(QualityIndex))
	{
		Settings->SetMenuQualityLevel(QualityIndex);
	}

	Settings->SetVSyncEnabled(IsToggleOn(VSyncCombo, Settings->IsVSyncEnabled()));

	// ApplySettings = 해상도·창 모드 변경 요청 + 스케일러빌리티 적용 + 파일 저장
	Settings->ApplySettings(false);

	// 모니터가 거절한 해상도는 엔진이 바꾼 값으로 다시 보여 준다
	RefreshFromSettings(*Settings);

	// 창 모드·해상도가 확정 값과 다를 때만 묻는다 (품질·수직 동기화만 바꿨으면 바로 확정)
	// 전체 화면(창)은 모니터 모드를 안 바꿔 화면이 꺼질 일이 없으니 해상도 차이는 묻지 않는다 (저장된 값이 바탕화면과 달라 매번 뜨던 문제)
	const EWindowMode::Type AppliedMode = Settings->GetFullscreenMode();
	const bool bVideoModeChanged = AppliedMode != Settings->GetLastConfirmedFullscreenMode()
		|| (AppliedMode != EWindowMode::WindowedFullscreen && Settings->GetScreenResolution() != Settings->GetLastConfirmedScreenResolution());
	if (bVideoModeChanged)
	{
		StartVideoConfirm();
	}
	else
	{
		StopVideoConfirm();
	}
}

void UUSSettingsWidget::HandleCloseClicked()
{
	Close();
}

void UUSSettingsWidget::HandleVideoKeepClicked()
{
	if (UUSGameUserSettings* Settings = UUSGameUserSettings::Get())
	{
		Settings->ConfirmVideoMode();
		Settings->SaveSettings();
	}
	StopVideoConfirm();
}

void UUSSettingsWidget::HandleVideoRevertClicked()
{
	RevertVideoMode();
}

void UUSSettingsWidget::StartVideoConfirm()
{
	VideoConfirmDeadline = FPlatformTime::Seconds() + VideoConfirmSeconds;
	if (!VideoConfirmTickerHandle.IsValid())
	{
		VideoConfirmTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UUSSettingsWidget::TickVideoConfirm), 0.2f);
	}
	if (VideoConfirmRow)
	{
		VideoConfirmRow->SetVisibility(ESlateVisibility::Visible);
	}
	UpdateVideoConfirmText();
}

void UUSSettingsWidget::StopVideoConfirm()
{
	if (VideoConfirmTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(VideoConfirmTickerHandle);
		VideoConfirmTickerHandle.Reset();
	}
	if (VideoConfirmRow)
	{
		VideoConfirmRow->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UUSSettingsWidget::RevertVideoMode()
{
	StopVideoConfirm();

	UUSGameUserSettings* Settings = UUSGameUserSettings::Get();
	if (Settings == nullptr)
	{
		return;
	}

	// RevertVideoMode는 값만 되돌린다 → 다시 적용해야 화면이 바뀌고 파일에도 확정 값이 써진다
	Settings->RevertVideoMode();
	Settings->ApplySettings(false);
	UE_LOG(LogUS, Log, TEXT("[%s] Video mode reverted to %dx%d (mode %d)."), *GetName(),
		Settings->GetScreenResolution().X, Settings->GetScreenResolution().Y, static_cast<int32>(Settings->GetFullscreenMode()));

	RefreshFromSettings(*Settings);
}

bool UUSSettingsWidget::TickVideoConfirm(float DeltaTime)
{
	if (FPlatformTime::Seconds() >= VideoConfirmDeadline)
	{
		// RevertVideoMode가 핸들을 비우고 티커를 지운다. false도 돌려 이번 호출 뒤 확실히 멈추게 한다
		RevertVideoMode();
		return false;
	}

	UpdateVideoConfirmText();
	return true;
}

void UUSSettingsWidget::UpdateVideoConfirmText() const
{
	if (VideoConfirmText)
	{
		const int32 SecondsLeft = FMath::Max(0, FMath::CeilToInt(VideoConfirmDeadline - FPlatformTime::Seconds()));
		VideoConfirmText->SetText(FText::Format(VideoConfirmFormat, FText::AsNumber(SecondsLeft)));
	}
}
