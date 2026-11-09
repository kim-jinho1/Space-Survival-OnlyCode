#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USThrusterFXComponent.generated.h"

class USceneComponent;
class UParticleSystem;
class UParticleSystemComponent;

/**
 * 드론 추진 불꽃 연출 컴포넌트.
 * 엔진 위치마다 불꽃 이펙트를 한 번 붙여 두고, 이동 속도에 비례해 크기만 바꾼다.
 * 붙일 부모(외형 회전 중심)는 소유 액터가 SetAttachParent로 넘겨 준다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class US_API UUSThrusterFXComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUSThrusterFXComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 불꽃을 붙일 부모를 지정한다. BeginPlay 전에 불러야 한다 */
	void SetAttachParent(USceneComponent* InAttachParent);

	/** 불꽃을 끄고 크기 갱신을 멈춘다 (사망 등). 이미 나온 입자는 자연스럽게 사라진다 */
	void StopThrusters();

	/** 불꽃 크기 배율 (대기·최대 모두에 곱한다). 대장 이속 배율을 그대로 받아 빨라진 만큼 불꽃이 커진다 */
	void SetSizeMultiplier(float InMultiplier) { SizeMultiplier = FMath::Max(0.0f, InMultiplier); }

	/** Paragon 이펙트를 LOD 0으로 고정한다 (카메라 2000~2700에서 LOD 1로 내려가 거의 안 보이는 문제). 적 트레일도 쓴다 */
	static void ForceTopLOD(UParticleSystemComponent* Component);

protected:
	/** 엔진마다 붙일 불꽃 이펙트 (계속 반복되는 이펙트여야 한다) */
	UPROPERTY(EditDefaultsOnly, Category = "Thruster")
	TObjectPtr<UParticleSystem> ThrusterFX;

	/** 엔진 위치 목록 (부모 기준). 위치마다 불꽃 하나 */
	UPROPERTY(EditDefaultsOnly, Category = "Thruster")
	TArray<FVector> EngineOffsets;

	/** 불꽃 회전 보정. 이펙트의 분사 방향이 기체 뒤쪽을 향하도록 맞춘다 */
	UPROPERTY(EditDefaultsOnly, Category = "Thruster")
	FRotator ThrusterRotation = FRotator(0.0f, 180.0f, 0.0f);

	/** 정지했을 때 불꽃 크기 */
	UPROPERTY(EditDefaultsOnly, Category = "Thruster")
	float IdleScale = 0.3f;

	/** ReferenceSpeed 이상으로 움직일 때 불꽃 크기 */
	UPROPERTY(EditDefaultsOnly, Category = "Thruster")
	float MaxScale = 1.0f;

	/** MaxScale에 도달하는 이동 속도 */
	UPROPERTY(EditDefaultsOnly, Category = "Thruster")
	float ReferenceSpeed = 400.0f;

	/** 크기 보간 속도. 클수록 가감속에 즉각 반응한다 */
	UPROPERTY(EditDefaultsOnly, Category = "Thruster")
	float ScaleInterpSpeed = 6.0f;

	/**
	 * 엔진 꼬리 궤적 이펙트 (비우면 궤적 없음). 기체가 지나간 자리에 남는 이펙트라 길이가 속도를 따라간다.
	 * 카메라가 멀어 불꽃만으로는 이동이 안 읽힐 때 움직임을 보여 준다. 크기는 TrailScale로 고정 (속도 비례 안 함)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Thruster|Trail")
	TObjectPtr<UParticleSystem> TrailFX;

	/** 궤적 위치 목록 (부모 기준). 위치마다 궤적 하나 */
	UPROPERTY(EditDefaultsOnly, Category = "Thruster|Trail")
	TArray<FVector> TrailOffsets;

	/** 궤적 크기 */
	UPROPERTY(EditDefaultsOnly, Category = "Thruster|Trail", meta = (ClampMin = "0.0"))
	float TrailScale = 1.0f;

	/** 이 시간(초) 안에 렌더되지 않았으면 크기 계산을 건너뛴다 */
	UPROPERTY(EditDefaultsOnly, Category = "Thruster")
	float RenderSkipTime = 0.2f;

private:
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> AttachParent;

	/** BeginPlay에서 한 번 만든 불꽃들. 매 프레임 크기만 갱신한다 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UParticleSystemComponent>> ThrusterComponents;

	/** BeginPlay에서 한 번 만든 궤적들. 크기는 바꾸지 않고 사망 때만 끈다 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UParticleSystemComponent>> TrailComponents;

	/** 현재 불꽃 크기 */
	float CurrentScale = 0.0f;

	/** SetSizeMultiplier로 받은 배율 */
	float SizeMultiplier = 1.0f;
};
