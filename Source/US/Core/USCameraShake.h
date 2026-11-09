#pragma once

#include "CoreMinimal.h"
#include "Shakes/DefaultCameraShakeBase.h"
#include "USCameraShake.generated.h"

/**
 * 짧은 타격 흔들림 (Perlin 노이즈). 처치·피격 모두 이 클래스를 쓰고 세기는 재생할 때 배율로 나눈다.
 * 쿼터뷰는 카메라가 멀어 위치 흔들림은 거의 안 보이므로 회전(Pitch/Yaw) 위주로 흔든다.
 */
UCLASS()
class US_API UUSCameraShake : public UDefaultCameraShakeBase
{
	GENERATED_BODY()

public:
	UUSCameraShake(const FObjectInitializer& ObjectInitializer);
};
