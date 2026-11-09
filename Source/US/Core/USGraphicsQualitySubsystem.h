#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "USGraphicsQualitySubsystem.generated.h"

/**
 * 처음 실행한 PC에서 하드웨어 벤치마크로 그래픽 품질 단계를 자동으로 고른다.
 * 엔진 기본값은 최고 단계라 약한 GPU에서도 최고로 켜지기 때문. 한 번 고른 뒤에는 설정 창에서 바꾼 값이 계속 우선한다.
 * 에디터(PIE)에서는 만들지 않는다 — 에디터 품질 설정을 덮어쓰지 않게.
 */
UCLASS()
class US_API UUSGraphicsQualitySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
};
