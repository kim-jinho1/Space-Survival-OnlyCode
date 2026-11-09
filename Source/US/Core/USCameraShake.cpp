#include "Core/USCameraShake.h"
#include "Shakes/PerlinNoiseCameraShakePattern.h"

UUSCameraShake::UUSCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 연달아 처치할 때 흔들림이 겹쳐 쌓이지 않고 새로 시작하게 한다
	bSingleInstance = true;

	// 부모(UDefaultCameraShakeBase)가 루트 패턴을 Perlin 노이즈로 만들어 둔다
	UPerlinNoiseCameraShakePattern* Pattern = Cast<UPerlinNoiseCameraShakePattern>(GetRootShakePattern());
	if (Pattern == nullptr)
	{
		return;
	}

	Pattern->Duration = 0.2f;
	Pattern->BlendInTime = 0.02f;
	Pattern->BlendOutTime = 0.12f;

	Pattern->X.Amplitude = 4.0f;
	Pattern->X.Frequency = 25.0f;
	Pattern->Y.Amplitude = 4.0f;
	Pattern->Y.Frequency = 25.0f;

	Pattern->Pitch.Amplitude = 0.8f;
	Pattern->Pitch.Frequency = 20.0f;
	Pattern->Yaw.Amplitude = 0.8f;
	Pattern->Yaw.Frequency = 20.0f;
}
