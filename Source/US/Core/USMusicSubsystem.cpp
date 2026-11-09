#include "Core/USMusicSubsystem.h"
#include "US.h"
#include "Core/USAudioSettings.h"
#include "Datas/USMusicSetData.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundClass.h"

bool UUSMusicSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UUSMusicSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// PIE 월드 패키지 이름에는 UEDPIE_0_ 접두가 붙어 설정의 맵 경로와 비교하려면 떼야 한다
	const FString WorldPackage = UWorld::RemovePIEPrefix(InWorld.GetOutermost()->GetName());

	for (const FUSMapMusic& Entry : GetDefault<UUSAudioSettings>()->MapMusic)
	{
		if (Entry.Map.ToSoftObjectPath().GetLongPackageName() == WorldPackage)
		{
			UUSMusicSetData* LoadedSet = Entry.Music.LoadSynchronous();
			if (LoadedSet == nullptr)
			{
				UE_LOG(LogUS, Warning, TEXT("[USMusicSubsystem] Music set for %s failed to load (%s)."), *WorldPackage, *Entry.Music.ToString());
			}
			UE_LOG(LogUS, Log, TEXT("[USMusicSubsystem] %s -> %s"), *WorldPackage, *GetNameSafe(LoadedSet));
			PlayMusicSet(LoadedSet);
			return;
		}
	}

	UE_LOG(LogUS, Log, TEXT("[USMusicSubsystem] No music for %s (%d entries)."), *WorldPackage, GetDefault<UUSAudioSettings>()->MapMusic.Num());
}

void UUSMusicSubsystem::Deinitialize()
{
	PlayMusicSet(nullptr);

	Super::Deinitialize();
}

void UUSMusicSubsystem::PlayMusicSet(UUSMusicSetData* InMusicSet)
{
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}

	MusicSet = InMusicSet;
	if (MusicSet == nullptr || MusicSet->Layers.IsEmpty())
	{
		return;
	}

	BarIndex = 0;
	LastSoundIndices.Init(INDEX_NONE, MusicSet->Layers.Num());
	NextBarTime = FPlatformTime::Seconds();

	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UUSMusicSubsystem::HandleTicker));
}

bool UUSMusicSubsystem::HandleTicker(float DeltaTime)
{
	if (MusicSet == nullptr)
	{
		return false;
	}

	// 프레임이 크게 밀렸을 때(로딩 등) 밀린 마디를 한꺼번에 틀지 않고 지금부터 다시 맞춘다
	const double Now = FPlatformTime::Seconds();
	if (Now - NextBarTime > MusicSet->BarSeconds)
	{
		NextBarTime = Now;
	}

	if (Now >= NextBarTime)
	{
		PlayBar();
		++BarIndex;
		NextBarTime += MusicSet->BarSeconds;
	}

	return true;
}

void UUSMusicSubsystem::PlayBar()
{
	for (int32 LayerIndex = 0; LayerIndex < MusicSet->Layers.Num(); ++LayerIndex)
	{
		const FUSMusicLayer& Layer = MusicSet->Layers[LayerIndex];
		const int32 SoundCount = Layer.Sounds.Num();
		if (SoundCount == 0 || BarIndex < Layer.StartBar || (BarIndex - Layer.StartBar) % Layer.EveryBars != 0)
		{
			continue;
		}
		if (FMath::FRand() > Layer.Chance)
		{
			continue;
		}

		// 직전 조각을 빼고 고른다 (조각이 하나뿐이면 그것)
		int32 SoundIndex = FMath::RandRange(0, SoundCount - 1);
		if (SoundCount > 1 && SoundIndex == LastSoundIndices[LayerIndex])
		{
			SoundIndex = (SoundIndex + 1 + FMath::RandRange(0, SoundCount - 2)) % SoundCount;
		}
		LastSoundIndices[LayerIndex] = SoundIndex;

		PlaySound(Layer.Sounds[SoundIndex], Layer.Volume);
	}
}

void UUSMusicSubsystem::PlaySound(USoundBase* Sound, float Volume)
{
	if (Sound == nullptr)
	{
		return;
	}

	UAudioComponent* AudioComponent = UGameplayStatics::CreateSound2D(GetWorld(), Sound, Volume);
	if (AudioComponent == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[USMusicSubsystem] Failed to create audio for %s."), *GetNameSafe(Sound));
		return;
	}

	// 일시정지 중에도 이어지게 UI 소리로, 배경음 슬라이더를 따르게 Music 클래스로
	AudioComponent->bIsUISound = true;
	AudioComponent->SoundClassOverride = GetDefault<UUSAudioSettings>()->MusicSoundClass.LoadSynchronous();
	AudioComponent->Play();
	UE_LOG(LogUS, Verbose, TEXT("[USMusicSubsystem] Bar %d: %s (playing %d)"), BarIndex, *GetNameSafe(Sound), AudioComponent->IsPlaying() ? 1 : 0);
}
