#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "USBullet.generated.h"

class UParticleSystem;
class UParticleSystemComponent;
class USoundBase;
class UStaticMeshComponent;

UCLASS()
class US_API AUSBullet : public AActor
{
	GENERATED_BODY()

public:
	AUSBullet();

	virtual void Tick(float DeltaSeconds) override;

	/** 시작 지점에서 목표 지점으로 날아가도록 발사. 비행 시간 제한도 거리에 맞춰 여기서 정한다 */
	void Launch(const FVector& Start, const FVector& Target);

	/** 목표 도착 시 재생할 명중 이펙트·사운드. 판정은 히트스캔이 이미 끝냈고, 보이는 타이밍만 총알 도착에 맞춘다 */
	void SetImpact(UParticleSystem* InImpactFX, USoundBase* InImpactSound, const FRotator& InImpactRotation);

	/** 풀에서 꺼낼 때: 보이게 하고 Tick·궤적을 켠다 (UUSBulletPoolSubsystem 전용) */
	void ActivateFromPool();

	/** 풀에 넣을 때: 숨기고 Tick·궤적을 끄고 상태를 초기화한다 (UUSBulletPoolSubsystem 전용) */
	void DeactivateToPool();

	FORCEINLINE bool IsInPool() const { return bInPool; }

	/** 비행 속도 (cm/s) */
	UPROPERTY(EditAnywhere, Category = "Bullet")
	float Speed = 7000.0f;

	/** 비행 시간 제한 여유(초). 제한 = 거리 / 속도 + 이 값. 넘기면 목표에 못 닿아도 멈춘다 */
	UPROPERTY(EditAnywhere, Category = "Bullet")
	float FlightTimeMargin = 0.5f;

	/** 멈춘 뒤 남은 궤적 꼬리가 사라지도록 기다리는 시간(초). 그 뒤 풀에 반납 */
	UPROPERTY(EditAnywhere, Category = "Bullet")
	float TrailLingerTime = 0.3f;

protected:
	/** 도착 지점에서 명중 이펙트·사운드 재생 */
	void PlayImpact() const;

	/** 비행을 멈추고 궤적 꼬리가 사라질 때까지 기다린다 (TrailLingerTime이 0 이하면 바로 반납) */
	void BeginLinger();

	/** 풀에 반납한다. 풀이 없으면 파괴 */
	void ReturnToPool();

	UPROPERTY(VisibleAnywhere, Category = "Bullet")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** 비행 궤적 이펙트. 이펙트는 BP의 이 컴포넌트 Template에서 고른다 */
	UPROPERTY(VisibleAnywhere, Category = "Bullet")
	TObjectPtr<UParticleSystemComponent> TrailComponent;

	FVector TargetLocation = FVector::ZeroVector;
	bool bLaunched = false;

private:
	/** 도착 시 재생할 명중 이펙트 (없으면 재생 안 함 — 빗나간 총알) */
	UPROPERTY()
	TObjectPtr<UParticleSystem> ImpactFX;

	/** 도착 시 재생할 명중 사운드 */
	UPROPERTY()
	TObjectPtr<USoundBase> ImpactSound;

	/** 명중 이펙트 방향 (맞은 면의 법선) */
	FRotator ImpactRotation = FRotator::ZeroRotator;

	/** 발사 후 흐른 시간 */
	float FlightTime = 0.0f;

	/** 이번 비행의 시간 제한. Launch에서 거리 / 속도로 계산 */
	float MaxFlightTime = 0.0f;

	/** 멈춘 뒤 꼬리가 사라지길 기다리는 중인지 */
	bool bLingering = false;

	/** 반납까지 남은 대기 시간 */
	float LingerTimeLeft = 0.0f;

	/** 풀에서 쉬는 중인지. 이중 반납 방지 */
	bool bInPool = false;
};
