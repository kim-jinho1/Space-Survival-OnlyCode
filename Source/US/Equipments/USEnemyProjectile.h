#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "USEnemyProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 * 적(보스)이 쏘는 탄 (출시 전 체크 R-36).
 * 우리 편 무기(AUSBullet)는 명중을 무기가 미리 정하고 탄은 보이기만 하지만, 이 탄은 피할 수 있어야 하므로 스스로 겹침으로 맞는다.
 * 곧게 날다가 우리 편(대장·부하, UUSTeamSubsystem 아군 등록부)에 닿으면 피해를 주고 사라진다. 수명이 끝나도 사라진다.
 * 보스 탄막은 몇 초마다 수십 발이라 풀 없이 스폰·삭제한다.
 */
UCLASS()
class US_API AUSEnemyProjectile : public AActor
{
	GENERATED_BODY()

public:
	AUSEnemyProjectile();

	virtual void Tick(float DeltaSeconds) override;

	/** 스폰 직후: 수평 방향·속도·피해·수명을 넣고 날기 시작한다 */
	void Launch(const FVector& Direction, float InSpeed, float InDamage, float LifeSeconds);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& Sweep);

	/** 맞는 판정 (Pawn 겹침만) */
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<USphereComponent> Collision;

	/** 보이는 탄 (메시·머티리얼은 BP) */
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	FVector Velocity = FVector::ZeroVector;
	float Damage = 0.0f;

	/** 이미 맞혔는가 (한 프레임에 둘과 겹쳐도 한 번만) */
	bool bHit = false;
};
