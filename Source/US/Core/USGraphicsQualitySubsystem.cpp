#include "Core/USGraphicsQualitySubsystem.h"
#include "US.h"
#include "Core/USGameUserSettings.h"

bool UUSGraphicsQualitySubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return !GIsEditor && Super::ShouldCreateSubsystem(Outer);
}

void UUSGraphicsQualitySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UUSGameUserSettings* Settings = UUSGameUserSettings::Get();
	if (Settings == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[USGraphicsQualitySubsystem] GameUserSettings is not UUSGameUserSettings. Check GameUserSettingsClassName in DefaultEngine.ini."));
		return;
	}

	Settings->ApplyFirstLaunchQuality();
	Settings->ApplyDefaultFrameRateLimit();
}
