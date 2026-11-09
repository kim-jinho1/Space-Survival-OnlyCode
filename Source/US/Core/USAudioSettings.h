#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "USAudioSettings.generated.h"

class USoundClass;
class USoundMix;
class UUSMusicSetData;
class UWorld;

/** 맵 하나와 그 맵에서 틀 배경음악 규칙 */
USTRUCT()
struct FUSMapMusic
{
	GENERATED_BODY()

	UPROPERTY(Config, EditAnywhere, Category = "Music")
	TSoftObjectPtr<UWorld> Map;

	/** 쿠킹에 들어가게 /Game/Audio/Music 아래에 둔다 (DefaultGame.ini DirectoriesToAlwaysCook) */
	UPROPERTY(Config, EditAnywhere, Category = "Music")
	TSoftObjectPtr<UUSMusicSetData> Music;
};

/**
 * 음량 설정이 쓰는 애셋 경로 (프로젝트 설정 → Game → US Audio, 값은 DefaultGame.ini).
 * 사운드 믹스 하나에 사운드 클래스별 음량을 덮어써서 전체·배경음·효과음을 따로 조절한다.
 * 애셋 경로를 코드에 박지 않고, 플레이어마다 다른 값(GameUserSettings)과도 섞지 않으려고 따로 둔다.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "US Audio"))
class US_API UUSAudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 음량 덮어쓰기를 담는 사운드 믹스 (맵마다 한 번 활성화) */
	UPROPERTY(Config, EditAnywhere, Category = "Volume")
	TSoftObjectPtr<USoundMix> VolumeSoundMix;

	/** 전체 음량 = 모든 소리의 부모 클래스 */
	UPROPERTY(Config, EditAnywhere, Category = "Volume")
	TSoftObjectPtr<USoundClass> MasterSoundClass;

	/** 배경음 클래스 */
	UPROPERTY(Config, EditAnywhere, Category = "Volume")
	TSoftObjectPtr<USoundClass> MusicSoundClass;

	/** 효과음 클래스 (클래스가 비어 있는 소리는 프로젝트 기본 사운드 클래스 = 이 클래스로 들어오게 DefaultEngine.ini에서 맞춘다) */
	UPROPERTY(Config, EditAnywhere, Category = "Volume")
	TSoftObjectPtr<USoundClass> SfxSoundClass;

	/** 맵별 배경음악 (UUSMusicSubsystem이 맵 시작 때 찾는다). 없는 맵은 음악 없음 */
	UPROPERTY(Config, EditAnywhere, Category = "Music")
	TArray<FUSMapMusic> MapMusic;
};
