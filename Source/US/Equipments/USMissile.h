#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "USMissile.generated.h"

class UParticleSystemComponent;
class USceneComponent;
class UStaticMeshComponent;

/** 미사일이 목표 지점에 도착함 (도착 위치) */
DECLARE_DELEGATE_OneParam(FDelegateOnMissileArrived, const FVector&);

/**
 * 미사일 일제 사격의 미사일 한 발 (연출 + 도착 알림). 피해는 직접 주지 않고, 도착하면 OnArrived로 알려
 * 발사한 쪽(UUSMissileBarrageComponent)이 폭발·피해를 처리한다.
 * 비행은 2차 베지어 곡선: 출발점 → (중간 지점 + 위로 ArcHeight) → 목표 지점. 위로 솟았다가 꺾여 내리꽂히는 모양이 나온다.
 * 충돌 없이 시간으로만 날아가므로 물리·오버랩 비용이 없다. 도착 후에는 꼬리 연기가 사라질 때까지 숨긴 채 남았다가 스스로 사라진다.
 */
UCLASS()
class US_API AUSMissile : public AActor
{
	GENERATED_BODY()

public:
	AUSMissile();

	virtual void Tick(float DeltaSeconds) override;

	/** Start에서 Target으로 발사한다. 스폰 직후 한 번 부른다 */
	void Launch(const FVector& Start, const FVector& Target);

	/** 도착 알림 (한 번만) */
	FDelegateOnMissileArrived OnArrived;

protected:
	/** 도착: 알리고, 몸을 숨기고, 꼬리 연기가 사라질 시간 뒤에 파괴 */
	void Arrive();

	/** 비행 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "Missile", meta = (ClampMin = "0.05"))
	float FlightTime = 0.5f;

	/** 곡선 꼭대기 높이 (출발·목표 중간 지점에서 위로, cm) */
	UPROPERTY(EditDefaultsOnly, Category = "Missile", meta = (ClampMin = "0.0"))
	float ArcHeight = 350.0f;

	/** 도착 뒤 꼬리 연기가 사라질 때까지 남아 있는 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "Missile", meta = (ClampMin = "0.0"))
	float TrailLingerTime = 0.6f;

	/** 루트. 진행 방향(+X)으로 회전한다 — 메시는 그 아래에서 모델 방향에 맞게 따로 돌려 둔다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> Root;

	/** 미사일 몸체 (표시 전용, 충돌 없음). 메시·머티리얼·상대 회전은 BP에서 (엔진 Cone은 끝이 +Z라 Pitch -90) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** 꼬리 연기 (Cascade). 템플릿은 BP에서 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UParticleSystemComponent> Trail;

private:
	FVector StartLocation = FVector::ZeroVector;
	FVector ControlLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;

	float Elapsed = 0.0f;

	/** 날아가는 중인가 (Launch 전·도착 후 false) */
	bool bFlying = false;
};
