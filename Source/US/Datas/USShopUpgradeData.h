#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Progression/USProgressionTypes.h"
#include "USShopUpgradeData.generated.h"

class UTexture2D;

/** 보급선 반복 강화가 무엇을 키우는가 */
UENUM(BlueprintType)
enum class EUSShopUpgradeTarget : uint8
{
	/** 부하 전원의 스탯 (Effect 종류) */
	Squad,
	/** 대장의 골드 획득 배율 */
	GoldGain,
	/** 대장의 스탯 (Effect 종류) */
	Leader,
	/** 이번 판 출격 시간 (Value = 더할 초, 곱하지 않는다). 스테이지 판(1:30 고정)부터 상점에서 뺐다 */
	RunTime,
	/** 대장의 보상 획득 반경 (Value 0.15 = ×1.15, Vampire Survivors 자석) */
	MagnetRadius
};

/**
 * 보급선에서 반복해서 살 수 있는 강화 하나. 애셋 하나 = 상품 한 줄이라, 새 상품은 애셋을 만들어 상점 목록에 넣기만 하면 된다.
 * 효과는 살 때마다 곱해진다 (Value 0.1 = ×1.1, 출격 시간만 Value초를 더한다). 가격 = BaseCost × CostGrowth^(이번 런 구매 횟수).
 */
UCLASS(BlueprintType)
class US_API UUSShopUpgradeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	FORCEINLINE EUSShopUpgradeTarget GetTarget() const { return Target; }
	FORCEINLINE EUSEffectType GetEffect() const { return Effect; }
	FORCEINLINE float GetValue() const { return Value; }
	FORCEINLINE int32 GetBaseCost() const { return BaseCost; }
	FORCEINLINE float GetCostGrowth() const { return CostGrowth; }
	FORCEINLINE const FText& GetTitleFormat() const { return TitleFormat; }
	FORCEINLINE const FText& GetEffectText() const { return EffectText; }
	FORCEINLINE UTexture2D* GetIcon() const { return Icon; }

protected:
	/** 무엇을 키우는가 */
	UPROPERTY(EditDefaultsOnly, Category = "Upgrade")
	EUSShopUpgradeTarget Target = EUSShopUpgradeTarget::Squad;

	/** 키울 스탯 (Target이 Squad·Leader일 때만) */
	UPROPERTY(EditDefaultsOnly, Category = "Upgrade",
		meta = (EditCondition = "Target == EUSShopUpgradeTarget::Squad || Target == EUSShopUpgradeTarget::Leader"))
	EUSEffectType Effect = EUSEffectType::AttackDamage;

	/** 한 번 살 때 늘어나는 비율 (0.1 = +10%, 곱해서 쌓임). 출격 시간은 더할 초 */
	UPROPERTY(EditDefaultsOnly, Category = "Upgrade", meta = (ClampMin = "0.0"))
	float Value = 0.1f;

	/** 첫 구매 가격 (골드) */
	UPROPERTY(EditDefaultsOnly, Category = "Price", meta = (ClampMin = "0"))
	int32 BaseCost = 50;

	/** 살 때마다 가격이 곱해지는 비율 (기획서: 1.15) */
	UPROPERTY(EditDefaultsOnly, Category = "Price", meta = (ClampMin = "1.0"))
	float CostGrowth = 1.15f;

	/** 상점 카드 이름 (무엇을). {0} = 지금까지 산 횟수 (예: "아군 화력") */
	UPROPERTY(EditDefaultsOnly, Category = "Text")
	FText TitleFormat;

	/** 상점 카드 효과 (얼마나). 이름 아래 크게 보인다 (예: "+10%", "+15초") */
	UPROPERTY(EditDefaultsOnly, Category = "Text")
	FText EffectText;

	/** 상점 줄 아이콘 (흰색 그림, 위젯이 색을 입힌다) */
	UPROPERTY(EditDefaultsOnly, Category = "Text")
	TObjectPtr<UTexture2D> Icon;
};
