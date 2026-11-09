#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/WorldSubsystem.h"
#include "USMusicSubsystem.generated.h"

class UUSMusicSetData;
class USoundBase;

/**
 * 맵 배경음악: 맵이 시작되면 US Audio 설정(MapMusic)에서 이 맵의 음악 규칙을 찾아 튼다.
 * 마디 시계는 월드 타이머가 아니라 실제 시간(FTSTicker)으로 돈다 — 일시정지·레벨업 카드·히트스톱(시간 배율 0.05) 중에도
 * 음악이 끊기거나 늘어지지 않게. 같은 이유로 조각은 UI 소리(bIsUISound)로 틀어 일시정지 때 멈추지 않는다.
 * 음량은 Music 사운드 클래스로 보내 설정 창 배경음 슬라이더를 따른다.
 */
UCLASS()
class US_API UUSMusicSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 게임·PIE 월드만 */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** 다른 음악 규칙으로 바꾼다 (nullptr = 끔). 이미 울리는 조각은 잔향까지 그대로 둔다 */
	void PlayMusicSet(UUSMusicSetData* InMusicSet);

private:
	/** 매 프레임 실제 시간을 보고 다음 마디 시각이 지났으면 그 마디를 튼다 (프레임 오차가 쌓이지 않게 절대 시각 기준) */
	bool HandleTicker(float DeltaTime);

	/** 한 마디: 차례가 된 겹마다 조각 하나 */
	void PlayBar();

	void PlaySound(USoundBase* Sound, float Volume);

	UPROPERTY()
	TObjectPtr<UUSMusicSetData> MusicSet;

	FTSTicker::FDelegateHandle TickerHandle;

	/** 다음 마디 시작 실제 시각 (FPlatformTime::Seconds) */
	double NextBarTime = 0.0;

	int32 BarIndex = 0;

	/** 겹마다 직전에 튼 조각 (같은 조각 연속 방지) */
	TArray<int32> LastSoundIndices;
};
