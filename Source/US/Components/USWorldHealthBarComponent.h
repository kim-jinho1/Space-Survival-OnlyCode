#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "USWorldHealthBarComponent.generated.h"

/**
 * 머리 위 체력 바. 판 메시 + 머티리얼로 그리고 남은 비율은 Custom Primitive Data로 넘긴다.
 * 적마다 위젯(UMG)을 붙이지 않아 수십 마리여도 머티리얼 하나를 공유하며 싸게 그려진다.
 * 맞으면 보이고 VisibleDuration 동안 더 안 맞으면 숨는다 (타이머, Tick 없음).
 * 쿼터뷰 고정 카메라라 절대 회전으로 두고 BP에서 카메라 쪽으로 기울인 값을 그대로 쓴다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class US_API UUSWorldHealthBarComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UUSWorldHealthBarComponent();

	/** 체력 비율(0~1)을 반영하고 바를 잠시 보인다. 0 이하면 바로 숨긴다 */
	void ShowHealthRatio(float Ratio);

	/** 바를 숨긴다 */
	void HideBar();

protected:
	/** 체력 비율을 적을 Custom Primitive Data 인덱스. 머티리얼 파라미터의 Primitive Data Index와 같아야 한다 */
	UPROPERTY(EditDefaultsOnly, Category = "HealthBar")
	int32 RatioDataIndex = 0;

	/** 마지막으로 맞은 뒤 바가 보이는 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "HealthBar", meta = (ClampMin = "0.1"))
	float VisibleDuration = 3.0f;

private:
	/** 숨김 타이머. 맞을 때마다 다시 건다 */
	FTimerHandle HideTimerHandle;
};
