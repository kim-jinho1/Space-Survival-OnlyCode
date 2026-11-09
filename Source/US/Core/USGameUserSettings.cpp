#include "Core/USGameUserSettings.h"
#include "US.h"
#include "Core/USAudioSettings.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

UUSGameUserSettings* UUSGameUserSettings::Get()
{
	return GEngine ? Cast<UUSGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

void UUSGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();

	MasterVolume = DefaultMasterVolume;
	MusicVolume = DefaultMusicVolume;
	SfxVolume = DefaultSfxVolume;
	bScreenShakeEnabled = true;
	bDamageNumbersEnabled = true;
	FrameRateLimit = DefaultFrameRateLimit;
}

void UUSGameUserSettings::SetMasterVolume(float Volume)
{
	MasterVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
}

void UUSGameUserSettings::SetMusicVolume(float Volume)
{
	MusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
}

void UUSGameUserSettings::SetSfxVolume(float Volume)
{
	SfxVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
}

int32 UUSGameUserSettings::GetRepresentativeQualityLevel() const
{
	// 설정 창 단계는 낮음~최고 4개라 시네마틱(4)은 최고(3)로 접는다
	constexpr int32 MaxMenuLevel = 3;

	const int32 OverallLevel = GetOverallScalabilityLevel();
	if (OverallLevel >= 0)
	{
		return FMath::Min(OverallLevel, MaxMenuLevel);
	}

	const int32 GroupLevels[] = {
		GetViewDistanceQuality(), GetAntiAliasingQuality(), GetShadowQuality(), GetGlobalIlluminationQuality(),
		GetReflectionQuality(), GetPostProcessingQuality(), GetTextureQuality(), GetVisualEffectQuality(),
		GetFoliageQuality(), GetShadingQuality(), GetLandscapeQuality() };

	int32 Counts[MaxMenuLevel + 1] = {};
	for (const int32 GroupLevel : GroupLevels)
	{
		++Counts[FMath::Clamp(GroupLevel, 0, MaxMenuLevel)];
	}

	// 동률이면 높은 단계를 고른다 (엔진 기본값이 최고라 처음 켠 상태와 가깝다)
	int32 BestLevel = MaxMenuLevel;
	for (int32 Level = MaxMenuLevel; Level >= 0; --Level)
	{
		if (Counts[Level] > Counts[BestLevel])
		{
			BestLevel = Level;
		}
	}
	return BestLevel;
}

void UUSGameUserSettings::ApplyFirstLaunchQuality()
{
	// 벤치마크 결과는 GameUserSettings.ini에 저장되고, 한 번도 안 돌렸으면 엔진 기본값 -1
	if (GetLastGPUBenchmarkResult() >= 0.0f)
	{
		return;
	}

	RunHardwareBenchmark();

	// 벤치마크는 렌더 해상도도 GPU 점수로 정해 강한 GPU면 100%를 고른다(RTX 2060 2560×1080에서 GPU 14.9 → 23.2 ms)
	ScalabilityQuality.ResolutionQuality = ProjectDefaultResolutionQuality;

	ApplyHardwareBenchmarkResults();
	UE_LOG(LogUS, Log, TEXT("[USGameUserSettings] First launch benchmark: CPU %.0f, GPU %.0f -> quality %d"),
		GetLastCPUBenchmarkResult(), GetLastGPUBenchmarkResult(), GetRepresentativeQualityLevel());
}

void UUSGameUserSettings::ApplyDefaultFrameRateLimit()
{
	if (FrameRateLimitRevision >= CurrentFrameRateLimitRevision)
	{
		return;
	}

	SetFrameRateLimit(DefaultFrameRateLimit);
	FrameRateLimitRevision = CurrentFrameRateLimitRevision;
	ApplyNonResolutionSettings();
	SaveSettings();
	UE_LOG(LogUS, Log, TEXT("[USGameUserSettings] Frame rate limit set to %.0f"), DefaultFrameRateLimit);
}

void UUSGameUserSettings::SetMenuQualityLevel(int32 QualityLevel)
{
	SetOverallScalabilityLevel(QualityLevel);

	// 벤치마크와 같은 함정 — 최고를 고르면 렌더 해상도 100%가 저장돼 GPU 시간이 크게 늘었다
	ScalabilityQuality.ResolutionQuality = ProjectDefaultResolutionQuality;
}

void UUSGameUserSettings::ActivateVolumeMix(const UObject* WorldContextObject) const
{
	USoundMix* Mix = GetDefault<UUSAudioSettings>()->VolumeSoundMix.LoadSynchronous();
	if (Mix == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[USGameUserSettings] VolumeSoundMix is not set (Project Settings > US Audio). Volume settings have no effect."));
		return;
	}

	// 믹스 활성화는 참조 카운트라 맵마다 Push만 하면 쌓인다. 맵이 바뀌어도 오디오 장치는 그대로일 수 있으므로 Pop 후 Push로 1개를 유지한다
	UGameplayStatics::PopSoundMixModifier(WorldContextObject, Mix);
	UGameplayStatics::PushSoundMixModifier(WorldContextObject, Mix);
	ApplyAudioVolumes(WorldContextObject);
}

void UUSGameUserSettings::ApplyAudioVolumes(const UObject* WorldContextObject) const
{
	const UUSAudioSettings* AudioSettings = GetDefault<UUSAudioSettings>();
	USoundMix* Mix = AudioSettings->VolumeSoundMix.LoadSynchronous();
	if (Mix == nullptr)
	{
		return;
	}

	// 덮어쓰기는 부모에서 자식으로 곱해 내려가므로 전체(부모) × 배경음/효과음(자식)이 된다
	ApplyClassVolume(WorldContextObject, Mix, AudioSettings->MasterSoundClass, MasterVolume);
	ApplyClassVolume(WorldContextObject, Mix, AudioSettings->MusicSoundClass, MusicVolume);
	ApplyClassVolume(WorldContextObject, Mix, AudioSettings->SfxSoundClass, SfxVolume);
}

void UUSGameUserSettings::ApplyClassVolume(const UObject* WorldContextObject, USoundMix* Mix,
	const TSoftObjectPtr<USoundClass>& SoundClass, float Volume) const
{
	USoundClass* LoadedClass = SoundClass.LoadSynchronous();
	if (LoadedClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[USGameUserSettings] A sound class is not set (Project Settings > US Audio). That volume slider has no effect."));
		return;
	}

	// 슬라이더를 끄는 동안 바로 들리게 페이드 없이 (기본값 1초)
	constexpr float NoFade = 0.0f;
	UGameplayStatics::SetSoundMixClassOverride(WorldContextObject, Mix, LoadedClass, Volume, 1.0f, NoFade, true);
}
