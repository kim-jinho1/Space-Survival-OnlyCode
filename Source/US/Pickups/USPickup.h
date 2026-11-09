// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "USPickup.generated.h"

/** 픽업(젬) 종류 */
UENUM(BlueprintType)
enum class EUSPickupType : uint8
{
	Experience,
	Gold,
};

/**
 * 적 사망 시 드롭되는 젬(픽업) 액터.
 * 플레이어 획득반경(MagnetRadius) 안에 들어오면 플레이어 쪽으로 끌려가고,
 * 접촉 시 경험치/골드를 지급한 뒤 사라진다.
 */
UCLASS()
class US_API AUSPickup : public AActor
{
	GENERATED_BODY()

public:
	AUSPickup();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	/** 스폰 직후 종류/수량을 설정 */
	void InitPickup(EUSPickupType InType, float InAmount);

	/** 자동 흡수 시작 (AutoCollectDelay 타이머, 스테이지 클리어 때 컨트롤러가 바로) */
	void StartAutoCollect();

protected:
	/** 획득 트리거(플레이어 오버랩) */
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	/** 플레이어에게 보상을 지급하고 자신을 제거 */
	void Collect(class AUSPlayerCharacter* Collector);

protected:
	/** 획득 트리거 구체 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Pickup)
	TObjectPtr<class USphereComponent> CollisionSphere;

	/** 표시용 메시 (비주얼은 에디터에서 지정) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Pickup)
	TObjectPtr<class UStaticMeshComponent> Mesh;

	/** 픽업 종류 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pickup)
	EUSPickupType PickupType = EUSPickupType::Experience;

	/** 지급량 (경험치=실수, 골드=반올림 정수) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pickup)
	float Amount = 10.0f;

	/** 자석 흡수 시작 속도(cm/s). 끌리는 동안 MagnetAcceleration으로 빨라진다 */
	UPROPERTY(EditAnywhere, Category = Pickup)
	float MagnetSpeed = 1200.0f;

	/** 끌리는 동안 붙는 가속도(cm/s²) */
	UPROPERTY(EditAnywhere, Category = Pickup, meta = (ClampMin = "0.0"))
	float MagnetAcceleration = 4000.0f;

	/** 가속으로 오를 수 있는 최고 속도(cm/s) */
	UPROPERTY(EditAnywhere, Category = Pickup, meta = (ClampMin = "0.0"))
	float MaxMagnetSpeed = 3000.0f;

	/** 플레이어 현재 속도에 이 배율을 곱한 값보다 느려지지 않는다 (달아나는 대장도 따라잡게) */
	UPROPERTY(EditAnywhere, Category = Pickup, meta = (ClampMin = "0.0"))
	float PlayerSpeedFactor = 1.5f;

	/** 떨어질 때 무작위 방향으로 튀어 나가는 시간(초). 0이면 제자리에 떨어진다 */
	UPROPERTY(EditAnywhere, Category = "Pickup|Scatter", meta = (ClampMin = "0.0"))
	float ScatterDuration = 0.25f;

	/** 튀어 나가는 거리 최소·최대(cm) */
	UPROPERTY(EditAnywhere, Category = "Pickup|Scatter", meta = (ClampMin = "0.0"))
	float ScatterDistanceMin = 40.0f;

	UPROPERTY(EditAnywhere, Category = "Pickup|Scatter", meta = (ClampMin = "0.0"))
	float ScatterDistanceMax = 90.0f;

	/** 떨어진 뒤 이 시간(초)이 지나면 거리와 상관없이 플레이어에게 날아온다. 0 이하면 자동 흡수 없음 */
	UPROPERTY(EditAnywhere, Category = Pickup)
	float AutoCollectDelay = 3.0f;

	/** 플레이어 정보를 못 읽을 때 사용할 기본 획득반경 */
	UPROPERTY(EditAnywhere, Category = Pickup)
	float FallbackMagnetRadius = 200.0f;

	/** 메시 제자리 회전 속도 (도/초, 월드 Z축 기준). 0이면 회전 안 함 */
	UPROPERTY(EditAnywhere, Category = "Pickup")
	float SpinSpeed = 90.0f;

	/** 획득음 */
	UPROPERTY(EditAnywhere, Category = Pickup)
	TObjectPtr<class USoundBase> PickupSound;

protected:
	/** 자석에 끌리는 중 (한번 끌리면 유지) */
	bool bAttracted = false;

	/** 지금 끌리는 속도 (가속 누적) */
	float CurrentMagnetSpeed = 0.0f;

	/** 흩뿌리기 시작·끝 위치와 경과 시간. 경과가 ScatterDuration 이상이면 끝난 것 */
	FVector ScatterStart = FVector::ZeroVector;
	FVector ScatterEnd = FVector::ZeroVector;
	float ScatterElapsed = 0.0f;

	/** 이미 획득되었는지 (중복 지급 방지) */
	bool bCollected = false;

	/** 자동 흡수 타이머 */
	FTimerHandle AutoCollectTimerHandle;

	/** 캐시된 플레이어 폰 */
	UPROPERTY()
	TObjectPtr<class APawn> CachedPlayerPawn;
};
