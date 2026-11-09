#include "Components/USUpgradeVisualComponent.h"
#include "US.h"
#include "Agents/USDroneCharacter.h"
#include "Datas/USAbilityData.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

UUSUpgradeVisualComponent::UUSUpgradeVisualComponent()
{
	// 단계가 바뀔 때만 일한다
	PrimaryComponentTick.bCanEverTick = false;
}

void UUSUpgradeVisualComponent::SetVisualTargets(USceneComponent* InScaleTarget, UStaticMeshComponent* InVisualMesh)
{
	ScaleTarget = InScaleTarget;
	VisualMesh = InVisualMesh;
	BaseScale = ScaleTarget ? ScaleTarget->GetRelativeScale3D() : FVector::OneVector;
	BaseMesh = VisualMesh ? VisualMesh->GetStaticMesh() : nullptr;
	BaseMeshTransform = VisualMesh ? VisualMesh->GetRelativeTransform() : FTransform::Identity;
}

void UUSUpgradeVisualComponent::SetAbilityLevel(const UUSAbilityData* Ability, int32 Level)
{
	if (Ability == nullptr)
	{
		return;
	}

	if (ScaleTarget == nullptr || VisualMesh == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: UpgradeVisual has no targets; call SetVisualTargets in PostInitializeComponents"),
			*GetNameSafe(GetOwner()));
		return;
	}

	AbilityLevels.Add(Ability, Level);
	ApplyTier();
}

int32 UUSUpgradeVisualComponent::GetTier() const
{
	int32 Tier = 0;
	for (const TPair<TObjectPtr<const UUSAbilityData>, int32>& Pair : AbilityLevels)
	{
		Tier += Pair.Value;
	}
	return FMath::Min(Tier, MaxTier);
}

void UUSUpgradeVisualComponent::ApplyTier()
{
	const int32 Tier = GetTier();

	// 진화: 모델이 실제로 바뀔 때만 교체한다 (같은 단계 재적용·같은 구간 안의 등급 변화는 그대로)
	const int32 TierMeshIndex = FindTierMeshIndex(Tier);
	if (TierMeshIndex != CurrentTierMeshIndex)
	{
		AUSDroneCharacter* Drone = Cast<AUSDroneCharacter>(GetOwner());
		if (Drone == nullptr)
		{
			UE_LOG(LogUS, Warning, TEXT("%s: UpgradeVisual owner is not a drone; cannot swap mesh"), *GetNameSafe(GetOwner()));
		}
		else if (TierMeshIndex == INDEX_NONE)
		{
			Drone->SetVisualMesh(BaseMesh, BaseMeshTransform, {});
		}
		else
		{
			const FUSTierMesh& TierMesh = TierMeshes[TierMeshIndex];
			Drone->SetVisualMesh(TierMesh.Mesh, TierMesh.Transform, TierMesh.SlotMaterials);
		}
		CurrentTierMeshIndex = TierMeshIndex;
	}

	// 크기: 추진 불꽃·포구도 ScaleTarget(VisualRoot) 아래라 같이 커진다. 둥실 연출은 위치·회전만 바꾸므로 크기와 겹치지 않는다
	ScaleTarget->SetRelativeScale3D(BaseScale * (1.0f + ScalePerTier * Tier));

	// 발광: 메시를 바꿔도 Custom Primitive Data는 컴포넌트에 남지만, 순서를 고정해 늘 마지막에 적는다
	VisualMesh->SetCustomPrimitiveDataFloat(TierCustomDataIndex, static_cast<float>(Tier) / static_cast<float>(MaxTier));
}

int32 UUSUpgradeVisualComponent::FindTierMeshIndex(int32 Tier) const
{
	int32 BestIndex = INDEX_NONE;
	int32 BestMinTier = 0;
	for (int32 Index = 0; Index < TierMeshes.Num(); ++Index)
	{
		const FUSTierMesh& TierMesh = TierMeshes[Index];
		if (TierMesh.Mesh == nullptr)
		{
			UE_LOG(LogUS, Warning, TEXT("%s: TierMeshes[%d] has no mesh"), *GetNameSafe(GetOwner()), Index);
			continue;
		}

		if (Tier >= TierMesh.MinTier && TierMesh.MinTier > BestMinTier)
		{
			BestIndex = Index;
			BestMinTier = TierMesh.MinTier;
		}
	}
	return BestIndex;
}
