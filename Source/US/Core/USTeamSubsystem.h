#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "USTeamSubsystem.generated.h"

struct FUSScreenBounds;

/**
 * 월드 단위 아군/적 등록부.
 * 드론(대장+부하)과 적이 BeginPlay/EndPlay/사망 시 스스로 등록·해제하고,
 * 타겟이 필요한 쪽은 여기서 "가장 가까운 상대"를 조회한다.
 * GetAllActorsOfClass / GetPlayerPawn을 매 프레임 부르는 대신 쓰는 캐시.
 */
UCLASS()
class US_API UUSTeamSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterAlly(AActor* Actor);
	void UnregisterAlly(AActor* Actor);
	void RegisterEnemy(AActor* Actor);
	void UnregisterEnemy(AActor* Actor);

	/** From에서 가장 가까운 적. MaxDistance가 0 이하면 거리 제한 없음. Screen을 주면 화면 안의 적만. 없으면 nullptr */
	AActor* FindNearestEnemy(const FVector& From, float MaxDistance = 0.0f, const FUSScreenBounds* Screen = nullptr) const;

	/**
	 * Point를 외형 범위(GetPickBounds, 평면 XY) 안에 두거나 범위에서 Padding 안에 있는 적. 여러 마리면 범위에 가장 가까운 적,
	 * 같으면(둘 다 범위 안) 중심이 가까운 적. 없으면 nullptr. 클릭 표적 지정용
	 */
	AActor* FindEnemyAtPoint(const FVector& Point, float Padding) const;

	/** From에서 가장 가까운 아군(대장 포함). MaxDistance가 0 이하면 거리 제한 없음. 없으면 nullptr */
	AActor* FindNearestAlly(const FVector& From, float MaxDistance = 0.0f) const;

	/** 도발 대상 등록. 반경 안의 적은 가장 가까운 아군 대신 이 액터를 노린다 */
	void RegisterTaunter(AActor* Actor, float Radius);
	void UnregisterTaunter(AActor* Actor);

	/** From을 도발 반경 안에 두는 도발 대상 중 가장 가까운 것. 없으면 nullptr */
	AActor* FindTaunterFor(const FVector& From) const;

	FORCEINLINE const TArray<TObjectPtr<AActor>>& GetAllies() const { return Allies; }
	FORCEINLINE const TArray<TObjectPtr<AActor>>& GetEnemies() const { return Enemies; }

private:
	/** 목록에서 From에 가장 가까운 유효 액터 */
	static AActor* FindNearest(const TArray<TObjectPtr<AActor>>& Actors, const FVector& From, float MaxDistance,
		const FUSScreenBounds* Screen = nullptr);

	UPROPERTY()
	TArray<TObjectPtr<AActor>> Allies;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> Enemies;

	/** 도발 대상 → 도발 반경 */
	UPROPERTY()
	TMap<TObjectPtr<AActor>, float> Taunters;
};
