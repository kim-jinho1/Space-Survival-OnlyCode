#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "USMetaTreeData.generated.h"

class UUSAllyTypeData;
class UUSMetaUpgradeData;

/**
 * 영구 업그레이드 트리 전체 (칸 목록). 판 시작 때 효과를 적용하는 게임 컨트롤러와 트리를 그리는 격납고 컨트롤러가 같은 애셋을 가리킨다.
 */
UCLASS(BlueprintType)
class US_API UUSMetaTreeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	FORCEINLINE const TArray<TObjectPtr<UUSMetaUpgradeData>>& GetNodes() const { return Nodes; }
	FORCEINLINE const UUSAllyTypeData* GetDefaultAllyType() const { return DefaultAllyType; }
	FORCEINLINE int32 GetBaseLoadoutSlots() const { return BaseLoadoutSlots; }

protected:
	/** 트리의 모든 칸 */
	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	TArray<TObjectPtr<UUSMetaUpgradeData>> Nodes;

	/** 편성 칸을 아직 고르지 않았거나 고른 기종이 잠겼을 때 채우는 기종 (기관포기) */
	UPROPERTY(EditDefaultsOnly, Category = "Loadout")
	TObjectPtr<UUSAllyTypeData> DefaultAllyType;

	/** 업그레이드 없이 가진 출격 편성 칸 수 */
	UPROPERTY(EditDefaultsOnly, Category = "Loadout", meta = (ClampMin = "0"))
	int32 BaseLoadoutSlots = 1;
};
