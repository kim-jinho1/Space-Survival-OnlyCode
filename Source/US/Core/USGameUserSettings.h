#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "USGameUserSettings.generated.h"

class USoundClass;
class USoundMix;

/**
 * 플레이어 설정 (Saved/Config/<플랫폼>/GameUserSettings.ini).
 * 해상도·창 모드·그래픽 품질·수직 동기화는 엔진 UGameUserSettings 기능을 그대로 쓰고, 음량 3개와 화면 흔들림·데미지 숫자 켜기만 더한다.
 * DefaultEngine.ini [/Script/Engine.Engine] GameUserSettingsClassName으로 등록해야 GEngine이 이 클래스로 만든다.
 */
UCLASS(Config = GameUserSettings, ConfigDoNotCheckDefaults)
class US_API UUSGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	/** GEngine이 만든 설정 객체 (등록이 빠졌으면 nullptr) */
	static UUSGameUserSettings* Get();

	virtual void SetToDefaults() override;

	float GetMasterVolume() const { return MasterVolume; }
	float GetMusicVolume() const { return MusicVolume; }
	float GetSfxVolume() const { return SfxVolume; }

	/** 음량 값만 바꾼다 (0~1). 소리에 반영은 ApplyAudioVolumes, 파일 저장은 SaveSettings */
	void SetMasterVolume(float Volume);
	void SetMusicVolume(float Volume);
	void SetSfxVolume(float Volume);

	bool IsScreenShakeEnabled() const { return bScreenShakeEnabled; }
	bool IsDamageNumbersEnabled() const { return bDamageNumbersEnabled; }

	/** 값만 바꾼다. 다음 흔들림·숫자부터 바로 반영되고(매번 읽는다), 파일 저장은 SaveSettings */
	void SetScreenShakeEnabled(bool bEnabled) { bScreenShakeEnabled = bEnabled; }
	void SetDamageNumbersEnabled(bool bEnabled) { bDamageNumbersEnabled = bEnabled; }

	/**
	 * 설정 창에 보여 줄 그래픽 품질 단계 (0 = 낮음 ~ 3 = 최고).
	 * 항목이 모두 같으면 그 단계, 섞여 있으면(GetOverallScalabilityLevel이 -1) 항목별 단계 중 가장 많은 단계. 시네마틱(4)은 최고로 친다
	 */
	int32 GetRepresentativeQualityLevel() const;

	/**
	 * 처음 실행이면(벤치마크 기록 없음) 하드웨어 벤치마크로 품질 단계를 골라 적용·저장한다. 이미 돌렸으면 아무것도 안 한다.
	 * 렌더 해상도는 벤치마크 값을 버리고 프로젝트 기본(업스케일)을 쓴다
	 */
	void ApplyFirstLaunchQuality();

	/**
	 * 프레임 제한을 기본값(60)으로 한 번만 넣고 저장한다. 이미 넣었으면 아무것도 안 한다.
	 * 엔진 기본은 제한 없음(수직 동기화도 꺼짐)이라 노트북에서 GPU가 쉬지 않고 돌아 발열이 컸다
	 */
	void ApplyDefaultFrameRateLimit();

	/**
	 * 설정 창에서 고른 품질 단계(0 = 낮음 ~ 3 = 최고)를 값으로만 넣는다. 적용·저장은 ApplySettings.
	 * SetOverallScalabilityLevel은 렌더 해상도도 단계에 맞춰(최고 = 100%) 바꾸므로 프로젝트 기본(업스케일)으로 되돌린다
	 */
	void SetMenuQualityLevel(int32 QualityLevel);

	/** 음량 믹스를 활성화하고 현재 음량을 적용한다. 맵이 시작될 때 한 번 (UUSAudioVolumeSubsystem) */
	void ActivateVolumeMix(const UObject* WorldContextObject) const;

	/** 활성화된 음량 믹스에 현재 음량을 다시 덮어쓴다 (설정 창 슬라이더) */
	void ApplyAudioVolumes(const UObject* WorldContextObject) const;

protected:
	/** 기본 음량. 멤버 초기값과 SetToDefaults가 같은 값을 쓴다 */
	static constexpr float DefaultMasterVolume = 1.0f;
	static constexpr float DefaultMusicVolume = 0.7f;
	static constexpr float DefaultSfxVolume = 0.8f;

	/** 렌더 해상도 0 = 프로젝트 기본 화면 비율 — 낮은 해상도로 그리고 TSR이 키운다 */
	static constexpr float ProjectDefaultResolutionQuality = 0.0f;

	/** 기본 프레임 제한 (fps) */
	static constexpr float DefaultFrameRateLimit = 60.0f;

	/** ApplyDefaultFrameRateLimit가 올리는 번호. 기본 제한을 바꾸면 올려서 기존 ini에도 다시 넣는다 */
	static constexpr int32 CurrentFrameRateLimitRevision = 1;

	/** 클래스 하나의 음량 덮어쓰기 (자식 클래스까지) */
	void ApplyClassVolume(const UObject* WorldContextObject, USoundMix* Mix, const TSoftObjectPtr<USoundClass>& SoundClass, float Volume) const;

	/** 전체 음량 (0~1) */
	UPROPERTY(Config)
	float MasterVolume = DefaultMasterVolume;

	/** 배경음 음량 (0~1) */
	UPROPERTY(Config)
	float MusicVolume = DefaultMusicVolume;

	/** 효과음 음량 (0~1) */
	UPROPERTY(Config)
	float SfxVolume = DefaultSfxVolume;

	/** 처치·피격 화면 흔들림 (AUSPlayerController::PlayHitShake) */
	UPROPERTY(Config)
	bool bScreenShakeEnabled = true;

	/** 적 위 데미지 숫자 (AUSPlayerController::HandleEnemyDamaged) */
	UPROPERTY(Config)
	bool bDamageNumbersEnabled = true;

	/** 기본 프레임 제한을 넣은 번호 (0 = 아직). SetToDefaults에서 건드리지 않는다 — 생성자 값이 키 없는 옛 ini에 남아 건너뛰게 된다 */
	UPROPERTY(Config)
	int32 FrameRateLimitRevision = 0;
};
