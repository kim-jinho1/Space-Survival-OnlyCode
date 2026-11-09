#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "USRingPulsePoolComponent.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * 월드 아무 곳에 "퍼지는 고리"를 한 번 그리는 바닥 판 풀. 폭발탄처럼 자주 터지는 효과의 범위 표시용.
 * 그림은 재질이 정하므로 무기 발사·명중 섬광(발광 원판)에도 같은 풀을 쓴다 (AUSWeapon::MuzzleFlashPool / ImpactFlashPool).
 * 고리 모양은 EMP 펄스와 같은 머티리얼(M_USEmpRing 계열)이 "펄스 시각"(Custom Primitive Data)과 현재 시간으로 계산하므로
 * C++은 판을 옮기고 시각만 적는다 — Tick·타이머·파티클 없음. 판은 처음 쓸 때 PoolSize개를 만들어 돌려 쓴다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class US_API UUSRingPulsePoolComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UUSRingPulsePoolComponent();

	/** Location에 반경 Radius짜리 고리를 한 번 퍼뜨린다. 판이 모자라면 가장 오래된 것을 다시 쓴다 */
	void PlayPulse(const FVector& Location, float Radius);

	/** 재질이 지정됐는가. 선택적으로 쓰는 풀(무기 섬광)은 이걸 먼저 보고 경고 없이 건너뛴다 */
	FORCEINLINE bool HasMaterial() const { return RingMaterial != nullptr; }

	/** 판 수 기본값. 생성자에서만 부른다 (판은 처음 PlayPulse 때 만들어지므로 그 뒤 바꿔도 무시됨) */
	FORCEINLINE void SetPoolSize(int32 InPoolSize) { PoolSize = FMath::Max(1, InPoolSize); }

protected:
	/** 고리를 그릴 판 메시 (기본 엔진 Plane, 한 변 100uu) */
	UPROPERTY(EditAnywhere, Category = "RingPulse")
	TObjectPtr<UStaticMesh> PlaneMesh;

	/** 고리 머티리얼. 비어 있으면 Warning 후 아무것도 그리지 않는다 */
	UPROPERTY(EditAnywhere, Category = "RingPulse")
	TObjectPtr<UMaterialInterface> RingMaterial;

	/** 동시에 보일 수 있는 고리 수. 고리 수명(머티리얼 0.6초) 동안 이보다 많이 터지면 오래된 고리가 끊긴다 */
	UPROPERTY(EditAnywhere, Category = "RingPulse", meta = (ClampMin = "1"))
	int32 PoolSize = 8;

	/** 펄스 시각을 적을 Custom Primitive Data 인덱스. 머티리얼 파라미터의 Primitive Data Index와 같아야 한다 */
	UPROPERTY(EditAnywhere, Category = "RingPulse")
	int32 PulseTimeDataIndex = 0;

private:
	/** 판 PoolSize개를 만들어 등록한다 (처음 PlayPulse 때 한 번) */
	void CreateRings();

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> Rings;

	/** 다음에 쓸 판 인덱스 (돌려 쓰기) */
	int32 NextRingIndex = 0;

	/** 머티리얼 미지정 경고를 한 번만 */
	bool bWarnedMissingMaterial = false;
};
