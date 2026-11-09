#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USUpgradeVisualComponent.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UUSAbilityData;

/** 등급 MinTier부터 바뀔 기체 모습 하나 (진화 모델) */
USTRUCT(BlueprintType)
struct FUSTierMesh
{
	GENERATED_BODY()

	/** 이 등급 이상이면 이 모습 */
	UPROPERTY(EditDefaultsOnly, Category = "Tier", meta = (ClampMin = "1"))
	int32 MinTier = 3;

	/** 진화 모델 */
	UPROPERTY(EditDefaultsOnly, Category = "Tier")
	TObjectPtr<UStaticMesh> Mesh;

	/** VisualRoot 기준 위치·회전·크기 (모델 피벗 보정, 코가 +X를 보게 하는 회전 포함) */
	UPROPERTY(EditDefaultsOnly, Category = "Tier")
	FTransform Transform;

	/**
	 * 이 모델에만 덮어쓸 슬롯 재질 (BP의 SlotMaterials 다음에 입힌다).
	 * 모델마다 포인트 슬롯이 차지하는 면적이 달라서, 넓은 포인트 면은 몸체 금속으로, 작은 면에 편 색을 옮길 때 쓴다.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Tier")
	TMap<FName, TObjectPtr<UMaterialInterface>> SlotMaterials;
};

/**
 * 부하 업그레이드 외형. 능력 단계가 오르면 기체 모습도 바뀌게 한다.
 * 등급 = 배운 능력 단계의 합(최대 MaxTier).
 * - 진화: 등급이 TierMeshes의 MinTier에 닿으면 기체 모델을 바꾼다 (BP에서 기종별로 지정).
 * - 크기: 등급 1마다 ScalePerTier씩 커진다 (VisualRoot 크기라 추진 불꽃·포구도 같이).
 * - 발광: 기체 포인트가 밝아지다 최고 등급에서 금색. 드론마다 머티리얼 사본을 만들지 않고 Custom Primitive Data로 등급 값(0~1)만 넘긴다 (M_USTeamAccent가 읽는다).
 * 단계 수치처럼 "N등급 모습"을 덮어쓰는 방식이라 같은 단계를 다시 받아도 결과가 같다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class US_API UUSUpgradeVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUSUpgradeVisualComponent();

	/**
	 * 크기를 바꿀 외형 회전 중심과 기체 메시를 지정한다. 소유 액터의 PostInitializeComponents에서 부른다.
	 * 이 시점의 BP 값(메시·상대 트랜스폼·크기)을 등급 0 모습으로 기억한다.
	 */
	void SetVisualTargets(USceneComponent* InScaleTarget, UStaticMeshComponent* InVisualMesh);

	/** Ability가 Level 단계가 됐다. 등급을 다시 계산해 외형에 반영한다 */
	void SetAbilityLevel(const UUSAbilityData* Ability, int32 Level);

	/** 현재 등급 (배운 능력 단계의 합, MaxTier로 자름) */
	int32 GetTier() const;

protected:
	/** 진화 모델 목록. 등급이 가장 높은 MinTier 이상인 항목을 쓴다. 비우면 진화 없음 */
	UPROPERTY(EditDefaultsOnly, Category = "Tier")
	TArray<FUSTierMesh> TierMeshes;

	/** 등급 1마다 커지는 비율 (0.05 = 5%) */
	UPROPERTY(EditDefaultsOnly, Category = "Tier", meta = (ClampMin = "0.0"))
	float ScalePerTier = 0.05f;

	/** 최고 등급. 기종마다 능력 2개 × 3단계 = 6. 이 등급에서 발광 값이 1(금색)이 된다 */
	UPROPERTY(EditDefaultsOnly, Category = "Tier", meta = (ClampMin = "1"))
	int32 MaxTier = 6;

	/** 등급 값(0~1)을 적을 Custom Primitive Data 인덱스. M_USTeamAccent의 Tier 파라미터 인덱스와 같아야 한다 */
	UPROPERTY(EditDefaultsOnly, Category = "Tier", meta = (ClampMin = "0"))
	int32 TierCustomDataIndex = 0;

private:
	/** 등급에 맞춰 모델·크기·발광을 다시 적용한다 */
	void ApplyTier();

	/** Tier에 맞는 TierMeshes 번호. 해당 없으면 INDEX_NONE(등급 0 모습) */
	int32 FindTierMeshIndex(int32 Tier) const;

	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> ScaleTarget;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	/** 등급 0 모습 (BP 값) */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> BaseMesh;

	FTransform BaseMeshTransform;

	/** ScaleTarget의 원래(등급 0) 상대 크기 */
	FVector BaseScale = FVector::OneVector;

	/** 능력별 현재 단계 */
	UPROPERTY(Transient)
	TMap<TObjectPtr<const UUSAbilityData>, int32> AbilityLevels;

	/** 지금 쓰는 TierMeshes 번호 (INDEX_NONE = 등급 0 모습). 바뀔 때만 메시를 교체한다 */
	int32 CurrentTierMeshIndex = INDEX_NONE;
};
