#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "USAudioVolumeSubsystem.generated.h"

/**
 * 맵이 시작될 때마다 저장된 음량을 적용한다.
 * 타이틀·격납고·게임 맵 컨트롤러가 각자 부르지 않게, 게임 월드마다 자동으로 생기는 월드 서브시스템 한 곳에 둔다.
 */
UCLASS()
class US_API UUSAudioVolumeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 에디터 미리보기 월드 등에는 만들지 않는다 (게임·PIE 월드만) */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
};
