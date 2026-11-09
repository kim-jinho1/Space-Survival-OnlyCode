#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USDamageVignetteWidget.generated.h"

class UImage;

/**
 * 대장 피격 때 화면 가장자리를 잠깐 붉게 물들인다 (Brotato·Halls of Torment식 피격 테두리).
 * 탑뷰에서는 기체가 작아 기체 번쩍임만으로는 맞은 걸 놓치기 쉬워 화면 전체로 알린다.
 * 이미지 이름은 VignetteImage여야 한다. 번쩍이는 동안만 Tick이 일하고, 아니면 바로 빠져나온다.
 */
UCLASS()
class US_API UUSDamageVignetteWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 테두리를 최대 세기로 켜고 FadeDuration 동안 줄인다. 이어 맞으면 처음부터 다시 */
	void PlayFlash();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 화면 전체를 덮는 테두리 이미지 (가운데가 투명한 붉은 비네트 텍스처) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> VignetteImage;

	/** 맞은 순간 불투명도 */
	UPROPERTY(EditDefaultsOnly, Category = "Vignette", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PeakOpacity = 0.85f;

	/** 사라지는 데 걸리는 시간(실제 초) */
	UPROPERTY(EditDefaultsOnly, Category = "Vignette", meta = (ClampMin = "0.05"))
	float FadeDuration = 0.35f;

private:
	void SetFlashOpacity(float Opacity) const;

	/** 켜진 뒤 지난 시간. 음수 = 꺼짐 */
	float Elapsed = -1.0f;
};
