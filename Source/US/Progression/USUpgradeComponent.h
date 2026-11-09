#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Progression/USLevelUpCard.h"
#include "USUpgradeComponent.generated.h"

class APawn;
class UTexture2D;
class UUSAbilityData;
class UUSBuildData;
class UUSSquadComponent;

/**
 * 레벨업 카드 후보를 만들고, 선택된 카드를 적용한다. PlayerController가 소유한다.
 * 카드는 능력 카드만 나온다 (D5-c, 기획서 "능력 카드"): 대장 능력 + 지금 분대에 살아 있는 기종의 능력 중 최대 단계가 아닌 것.
 * 후보가 Count보다 적으면 보급 카드(골드·수리)로 채운다. 능력 단계는 이 판에만 유지된다.
 * 대장 능력은 대장에 바로, 부하 능력은 분대(UUSSquadComponent::SetAbilityLevel)에 넘겨 그 기종 전원과 이후 합류·부활 기체에 적용한다.
 * 빌드: 능력마다 빌드(UUSBuildData)가 있고, 배운 능력이 많은 빌드의 카드가 더 잘 나온다(가중치, Brotato 태그 방식).
 * 한 빌드의 능력을 모두 1단계 이상 배우면 완성 보너스를 한 번 준다. 뽑은 뒤 순서를 섞어 빌드 카드가 한 자리에 몰리지 않게 한다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class US_API UUSUpgradeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUSUpgradeComponent();

	/** 카드 후보를 중복 없이 최대 Count장 만든다. Squad가 없으면 대장 능력만 */
	TArray<FUSLevelUpCard> GenerateChoices(int32 Count, const UUSSquadComponent* Squad) const;

	/** 선택된 카드를 적용한다 */
	void ApplyCard(const FUSLevelUpCard& Card, APawn* Leader, UUSSquadComponent* Squad);

	/** Ability의 이번 판 단계 (안 배웠으면 0) */
	int32 GetAbilityLevel(const UUSAbilityData* Ability) const;

	/** Builds 순서대로 이번 판 빌드 진행 (화면 위 진행 줄) */
	TArray<FUSBuildProgress> GetBuildProgress() const;

protected:
	/** 레벨업 카드로 나올 수 있는 능력 전체 (대장·모든 기종) */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp")
	TArray<TObjectPtr<UUSAbilityData>> Abilities;

	/** 진행 줄에 보일 빌드 (순서 = 화면 왼쪽부터). 능력의 Build가 여기 없어도 가중치·보너스는 동작한다 */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Build")
	TArray<TObjectPtr<UUSBuildData>> Builds;

	/** 카드 뽑기 가중치 = 1 + 그 빌드에서 배운 능력 수 × 이 값 (0 = 가중치 없음) */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Build", meta = (ClampMin = "0.0"))
	float BuildWeightPerLearned = 1.0f;

	/** 빌드 없는 능력·보급 카드의 색 띠 바탕 */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Build")
	FLinearColor NeutralBandColor = FLinearColor(0.05f, 0.07f, 0.12f, 1.0f);

	/** 능력 카드 제목. {0} = 기체 이름(대장·기종), {1} = 능력 이름, {2} = 될 단계 */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Text")
	FText AbilityTitleFormat;

	/** 대장 능력 카드에 쓸 기체 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Text")
	FText LeaderDisplayName;

	/** 보급 골드 카드가 주는 골드 */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Supply", meta = (ClampMin = "0"))
	int32 SupplyGoldAmount = 50;

	/** 보급 수리 카드가 회복하는 최대 체력 비율 (대장·부하 전원) */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Supply", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SupplyRepairFraction = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Supply")
	TObjectPtr<UTexture2D> SupplyGoldIcon;

	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Supply")
	TObjectPtr<UTexture2D> SupplyRepairIcon;

	/** 보급 카드 칩 */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Text")
	FText SupplyTag;

	/** 보급 카드 큰 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Text")
	FText SupplyGoldName;

	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Text")
	FText SupplyRepairName;

	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Text")
	FText SupplyGoldTitle;

	/** {0} = 골드 */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Text")
	FText SupplyGoldDescriptionFormat;

	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Text")
	FText SupplyRepairTitle;

	/** {0} = 회복 퍼센트 */
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp|Text")
	FText SupplyRepairDescriptionFormat;

private:
	/** 보급 카드 한 장 */
	FUSLevelUpCard MakeSupplyCard(EUSLevelUpCardKind Kind) const;

	/** Build에 속한 능력 중 배운 것 수 / 전체 수 */
	int32 CountLearned(const UUSBuildData* Build) const;
	int32 CountTotal(const UUSBuildData* Build) const;

	/** Build를 다 배웠으면 완성 보너스를 한 번 준다 */
	void TryCompleteBuild(const UUSBuildData* Build, APawn* Leader, UUSSquadComponent* Squad);

	/** 완성 보너스를 받은 빌드 */
	UPROPERTY()
	TSet<TObjectPtr<const UUSBuildData>> CompletedBuilds;

	/** 이번 판에 배운 능력의 단계 */
	UPROPERTY()
	TMap<TObjectPtr<const UUSAbilityData>, int32> AbilityLevels;
};
