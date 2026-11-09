#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USHoverVisualComponent.generated.h"

class USceneComponent;

/**
 * 드론 외형 연출 컴포넌트.
 * 보이는 메시(VisualTarget)의 상대 트랜스폼만 바꿔 공중에 떠서 둥실거리고, 이동 방향으로 기울고, 선회할 때 옆으로 눕게(뱅킹) 한다.
 * 이동·충돌은 소유 액터(캡슐 + CharacterMovement)가 그대로 맡고, 이 컴포넌트는 보이는 것만 담당한다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class US_API UUSHoverVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUSHoverVisualComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 연출할 메시를 지정한다. 지정 시점의 상대 트랜스폼(BP에서 잡은 위치·회전·스케일)을 기준으로 삼는다 */
	void SetVisualTarget(USceneComponent* InVisualTarget);

protected:
	/** 기준 위치에서 추가로 띄우는 높이 */
	UPROPERTY(EditDefaultsOnly, Category = "Hover")
	float HoverHeight = 40.0f;

	/** 둥실거리는 위아래 폭 */
	UPROPERTY(EditDefaultsOnly, Category = "Hover")
	float BobAmplitude = 6.0f;

	/** 둥실거리는 주기(초당 왕복 횟수) */
	UPROPERTY(EditDefaultsOnly, Category = "Hover")
	float BobFrequency = 1.2f;

	/** 옆으로 미끄러질 때 최대 롤 각도(도). 옆 속도가 TiltReferenceSpeed일 때 이만큼 기운다 */
	UPROPERTY(EditDefaultsOnly, Category = "Hover")
	float MaxTiltAngle = 18.0f;

	/** 앞뒤로 움직일 때 최대 피치 각도(도). 앞으로 가면 코를 숙인다. 우주선처럼 숙이지 않으려면 0 */
	UPROPERTY(EditDefaultsOnly, Category = "Hover", meta = (ClampMin = "0.0"))
	float MaxPitchAngle = 18.0f;

	/** 선회할 때 최대 롤 각도(도). 회전 속도가 BankReferenceYawRate일 때 이만큼 눕는다. 0이면 뱅킹 없음 */
	UPROPERTY(EditDefaultsOnly, Category = "Hover", meta = (ClampMin = "0.0"))
	float MaxBankAngle = 0.0f;

	/** 최대 뱅킹에 도달하는 회전 속도(도/초) */
	UPROPERTY(EditDefaultsOnly, Category = "Hover", meta = (ClampMin = "1.0"))
	float BankReferenceYawRate = 180.0f;

	/** 최대 기울기에 도달하는 이동 속도 */
	UPROPERTY(EditDefaultsOnly, Category = "Hover")
	float TiltReferenceSpeed = 400.0f;

	/** 기울기 보간 속도. 클수록 방향 전환에 즉각 반응한다 */
	UPROPERTY(EditDefaultsOnly, Category = "Hover")
	float TiltInterpSpeed = 8.0f;

	/** 이 시간(초) 안에 렌더되지 않았으면 연출 계산을 건너뛴다 */
	UPROPERTY(EditDefaultsOnly, Category = "Hover")
	float RenderSkipTime = 0.2f;

private:
	TObjectPtr<USceneComponent> VisualTarget;

	/** SetVisualTarget 시점의 상대 트랜스폼. 연출은 여기에 더해진다 */
	FTransform BaseRelativeTransform = FTransform::Identity;

	/** 둥실 위상(라디안, 0~2π). 유닛마다 랜덤 시작값을 줘 무리가 똑같이 출렁이지 않게 한다 */
	float BobPhase = 0.0f;

	/** 현재 기울기 (X = pitch, Y = roll) */
	FVector2D CurrentTilt = FVector2D::ZeroVector;

	/** 지난 틱의 액터 yaw. 회전 속도(뱅킹)를 구하는 데 쓴다 */
	float PreviousYaw = 0.0f;
};
