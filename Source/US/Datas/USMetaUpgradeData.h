#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "USMetaUpgradeData.generated.h"

class UUSAllyTypeData;

/** 영구 업그레이드 한 칸이 무엇을 키우는가 */
UENUM(BlueprintType)
enum class EUSMetaEffect : uint8
{
	/** 대장 공격력 (단계마다 ×(1 + Value)) */
	LeaderDamage,
	/** 대장 최대 체력 (단계마다 ×(1 + Value)) */
	LeaderHealth,
	/** 부하 전원 공격력 */
	SquadDamage,
	/** 부하 전원 최대 체력 */
	SquadHealth,
	/** 골드 획득량 */
	GoldGain,
	/** 경험치 획득량 */
	ExpGain,
	/** 결산 스크랩 획득량 */
	ScrapGain,
	/** 출격 편성 칸 (단계마다 한 칸, 격납고에서 기종을 골라 둔다) */
	StartingAlly,
	/** AllyType을 새 부하 후보에 넣는다 (1단계짜리) */
	UnlockAllyType,
	/** 출격 시간 (단계마다 +Value초, 더해서 쌓임). 스테이지 판(1:30 고정)부터 트리에서 뺐다 */
	RunDuration,
	/** 출격하자마자 고르는 레벨업 카드 (단계마다 +Value장, 더해서 쌓임). 출격 시간 칸 자리 (스테이지 판 개편 2026-10-26) */
	StartingCard,
	/**
	 * 레벨업 카드 다시 뽑기 횟수 (판마다, 단계마다 +Value회, 더해서 쌓임). Vampire Survivors "Reroll".
	 * 연쇄 폭발 확률 칸 자리 (2026-11-02, 레벨업 카드 폭발탄과 구분이 안 돼 폭발 칸 둘을 뺐다)
	 */
	CardReroll,
	/**
	 * 대장 이동 속도 (단계마다 +Value, 0.05 = +5%, 더해서 쌓임 — 이속 카드·상점과 같은 ApplyEffect 경로). 추진 불꽃도 커진다.
	 * 폭발 범위 칸 자리 (2026-11-02)
	 */
	LeaderMoveSpeed
};

/**
 * 영구 업그레이드 트리의 한 칸. 애셋 하나 = 칸 하나라, 새 칸은 애셋을 만들어 트리 목록(UUSMetaTreeData)에 넣기만 하면 된다.
 * 가격 = BaseCost × CostGrowth^(지금 단계). 저장은 애셋 이름이 아니라 NodeId로 하므로 애셋 이름을 바꿔도 저장된 단계가 남는다.
 */
UCLASS(BlueprintType)
class US_API UUSMetaUpgradeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** CurrentLevel 단계에서 다음 단계로 올리는 가격 */
	int32 GetCostAtLevel(int32 CurrentLevel) const;

	/** Level 단계까지 찍었을 때 쌓인 효과 글자 ("+15%", "+20초", "+80"). 시작 부하·해금처럼 수치가 없는 칸은 빈 글자 */
	FText GetEffectText(int32 Level) const;

	FORCEINLINE FName GetNodeId() const { return NodeId; }
	FORCEINLINE const FText& GetDisplayName() const { return DisplayName; }
	FORCEINLINE const FText& GetDescription() const { return Description; }
	FORCEINLINE EUSMetaEffect GetEffect() const { return Effect; }
	FORCEINLINE float GetValuePerLevel() const { return ValuePerLevel; }
	FORCEINLINE int32 GetMaxLevel() const { return MaxLevel; }
	FORCEINLINE const UUSMetaUpgradeData* GetPrerequisite() const { return Prerequisite; }
	FORCEINLINE const UUSAllyTypeData* GetAllyType() const { return AllyType; }
	FORCEINLINE const UUSAllyTypeData* GetOwnerAllyType() const { return OwnerAllyType; }
	FORCEINLINE const FText& GetPartName() const { return PartName; }
	FORCEINLINE FVector2D GetBlueprintAnchor() const { return BlueprintAnchor; }

protected:
	/** 저장 키. 한 번 정하면 바꾸지 않는다 (바꾸면 저장된 단계를 잃는다) */
	UPROPERTY(EditDefaultsOnly, Category = "Node")
	FName NodeId;

	/** 트리에 보일 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "Node")
	FText DisplayName;

	/** 효과 설명 */
	UPROPERTY(EditDefaultsOnly, Category = "Node", meta = (MultiLine = true))
	FText Description;

	/** 무엇을 키우는가 */
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	EUSMetaEffect Effect = EUSMetaEffect::LeaderDamage;

	/** 단계마다 늘어나는 비율 (0.15 = 단계마다 ×1.15, 곱해서 쌓임). 출격 시간·시작 카드·다시 뽑기·이속은 더해서 쌓임, 시작 부하·해금은 쓰지 않는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Effect", meta = (ClampMin = "0.0"))
	float ValuePerLevel = 0.1f;

	/** 시작 부하로 넣을 종류, 또는 해금할 종류 (StartingAlly·UnlockAllyType일 때만) */
	UPROPERTY(EditDefaultsOnly, Category = "Effect",
		meta = (EditCondition = "Effect == EUSMetaEffect::StartingAlly || Effect == EUSMetaEffect::UnlockAllyType"))
	TObjectPtr<UUSAllyTypeData> AllyType;

	/** 최대 단계 */
	UPROPERTY(EditDefaultsOnly, Category = "Effect", meta = (ClampMin = "1"))
	int32 MaxLevel = 5;

	/** 첫 단계 가격 (스크랩) */
	UPROPERTY(EditDefaultsOnly, Category = "Price", meta = (ClampMin = "0"))
	int32 BaseCost = 100;

	/** 단계를 올릴 때마다 가격이 곱해지는 비율 */
	UPROPERTY(EditDefaultsOnly, Category = "Price", meta = (ClampMin = "1.0"))
	float CostGrowth = 1.5f;

	/** 이 칸을 열려면 1단계 이상 찍어야 하는 칸. 비우면 처음부터 열림 */
	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	TObjectPtr<UUSMetaUpgradeData> Prerequisite;

	/** 격납고에서 이 칸을 보여 줄 기체. 비우면 대장기. 해금 칸은 해금할 기종 자신 */
	UPROPERTY(EditDefaultsOnly, Category = "Hangar")
	TObjectPtr<UUSAllyTypeData> OwnerAllyType;

	/** 설계도 화면에서 이 강화가 고치는 부위 이름 (예: "선체", "엔진"). 카드 이름 아래 작게 보인다 */
	UPROPERTY(EditDefaultsOnly, Category = "Hangar")
	FText PartName;

	/** 설계도 화면에서 카드 선이 닿을 도면 위 위치 (왼쪽 위 0,0 ~ 오른쪽 아래 1,1). X가 0.5보다 작으면 카드가 도면 왼쪽에 붙는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Hangar", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	FVector2D BlueprintAnchor = FVector2D(0.5f, 0.5f);
};
