#include "UI/USDamageVignetteWidget.h"
#include "US.h"
#include "Components/Image.h"

void UUSDamageVignetteWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 조준·클릭을 가로채지 않게 한다
	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (VignetteImage == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: VignetteImage is missing; damage vignette is disabled"), *GetName());
	}
	SetFlashOpacity(0.0f);
}

void UUSDamageVignetteWidget::PlayFlash()
{
	Elapsed = 0.0f;
	SetFlashOpacity(PeakOpacity);
}

void UUSDamageVignetteWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (Elapsed < 0.0f)
	{
		return;
	}

	// 위젯 Tick은 실제 시간이라 처치 히트스톱(전역 시간 배율)에도 같은 속도로 사라진다
	Elapsed += InDeltaTime;
	const float Alpha = FMath::Clamp(Elapsed / FadeDuration, 0.0f, 1.0f);

	// 처음엔 천천히, 끝에서 빨리 빠지게 (1 - t)^2
	SetFlashOpacity(PeakOpacity * FMath::Square(1.0f - Alpha));
	if (Alpha >= 1.0f)
	{
		Elapsed = -1.0f;
	}
}

void UUSDamageVignetteWidget::SetFlashOpacity(float Opacity) const
{
	if (VignetteImage)
	{
		VignetteImage->SetRenderOpacity(Opacity);
	}
}
