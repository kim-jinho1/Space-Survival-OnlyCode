#include "Core/USAudioVolumeSubsystem.h"
#include "US.h"
#include "Core/USGameUserSettings.h"
#include "Engine/World.h"

bool UUSAudioVolumeSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UUSAudioVolumeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UUSGameUserSettings* Settings = UUSGameUserSettings::Get();
	if (Settings == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[USAudioVolumeSubsystem] GameUserSettings is not UUSGameUserSettings. Check GameUserSettingsClassName in DefaultEngine.ini."));
		return;
	}

	Settings->ActivateVolumeMix(&InWorld);
}
