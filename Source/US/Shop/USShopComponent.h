#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USShopComponent.generated.h"

class UUSAllyTypeData;
class UUSCurrencyComponent;
class UUSShopUpgradeData;
class UUSSquadComponent;
class USoundBase;
class UTexture2D;

/** 보급선에서 출격 시간을 샀다 (더할 초). 런 시계를 가진 컨트롤러가 받는다 */
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateOnRunTimeExtended, float);

/** 보급선 상품 종류 */
UENUM()
enum class EUSShopOfferKind : uint8
{
	/** 살아 있는 부하 전원 회복 */
	Repair,
	/** 격추된 부하 한 대 부활 */
	Revive,
	/** 반복 강화 (편대·대장 스탯, 골드 획득, 출격 시간) */
	Upgrade,
	/** 부하 한 대 합류 */
	Recruit
};

/** 상점 목록의 한 줄. 보급선을 열 때마다 분대 상태로 새로 만든다 */
USTRUCT()
struct FUSShopOffer
{
	GENERATED_BODY()

	/** 상품 종류 */
	EUSShopOfferKind Kind = EUSShopOfferKind::Repair;

	/** 부활할 슬롯 (부활이 아니면 INDEX_NONE) */
	int32 SlotIndex = INDEX_NONE;

	/** 반복 강화 목록(Upgrades)의 인덱스 (강화가 아니면 INDEX_NONE) */
	int32 UpgradeIndex = INDEX_NONE;

	/**
	 * 합류할 부하 종류 (합류가 아니면 nullptr).
	 * 원시 포인터인 이유: 목록은 상점이 열린 동안만 쓰고, 가리키는 DataAsset은 분대 HireableTypes가 붙잡고 있어 GC되지 않는다
	 */
	const UUSAllyTypeData* AllyType = nullptr;

	/** 가격 (골드) */
	int32 Cost = 0;

	/** 표시 이름 (무엇을: "출격 시간", "방패 전투기") */
	FText Title;

	/** 효과 (얼마나: "+15초", "부활"). 카드에서 이름 아래 크게 보인다 */
	FText Effect;

	/** 이름 아래 한 줄 ("구매 2회"·"격추된 기체 부활") */
	FText Detail;

	/** 줄 아이콘 (없으면 위젯이 숨긴다). 원시 포인터인 이유는 AllyType과 같다: 가리키는 텍스처는 데이터 애셋이 붙잡고 있다 */
	UTexture2D* Icon = nullptr;
};

/**
 * 보급선 상점 로직. 분대 상태로 상품 목록(수리 + 격추된 부하마다 부활 + 반복 강화 + 부하 합류)을 만들고 구매를 처리한다.
 * 상점 위젯은 이 목록을 그리기만 하고, 컨트롤러는 위젯을 열고 닫기만 한다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class US_API UUSShopComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUSShopComponent();

	/**
	 * 분대 상태로 상품 목록을 새로 만든다 (상점을 열 때, 구매 후).
	 * bCanExtendRunTime이 false면 출격 시간 상품을 뺀다 (클리어 시각까지 이미 채웠거나 계속 가기 중)
	 */
	void RefreshOffers(const UUSSquadComponent* Squad, bool bCanExtendRunTime);

	/** Index 상품을 산다. 골드 부족·잘못된 인덱스·효과 없음이면 false */
	bool TryPurchase(int32 Index, UUSSquadComponent* Squad, UUSCurrencyComponent* Currency);

	FORCEINLINE const TArray<FUSShopOffer>& GetOffers() const { return Offers; }

	/** 출격 시간 상품을 샀다 */
	FDelegateOnRunTimeExtended OnRunTimeExtended;

protected:
	/** 다음 수리 가격 = RepairBaseCost × RepairCostGrowth^수리 횟수 */
	int32 GetRepairCost() const;

	/** Upgrades[Index]의 다음 가격 = BaseCost × CostGrowth^구매 횟수 */
	int32 GetUpgradeCost(int32 Index) const;

	/** 다음 합류 가격 = RecruitBaseCost × RecruitCostGrowth^합류 횟수 (종류와 상관없이 공통) */
	int32 GetRecruitCost() const;

	/** 반복 강화 한 줄을 산다 */
	bool TryPurchaseUpgrade(const FUSShopOffer& Offer, UUSSquadComponent* Squad, UUSCurrencyComponent* Currency);

	/** 부하 합류 한 줄을 산다 */
	bool TryPurchaseRecruit(const FUSShopOffer& Offer, UUSSquadComponent* Squad, UUSCurrencyComponent* Currency);

	/** 첫 수리 가격 */
	UPROPERTY(EditDefaultsOnly, Category = "Shop|Repair", meta = (ClampMin = "0"))
	int32 RepairBaseCost = 20;

	/** 수리할 때마다 가격이 곱해지는 비율 */
	UPROPERTY(EditDefaultsOnly, Category = "Shop|Repair", meta = (ClampMin = "1.0"))
	float RepairCostGrowth = 1.25f;

	/** 수리 한 번에 회복하는 양 (최대 체력 대비) */
	UPROPERTY(EditDefaultsOnly, Category = "Shop|Repair", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RepairFraction = 0.5f;

	/** 수리 성공음 (일시정지 중이라 UI 소리로 재생) */
	UPROPERTY(EditDefaultsOnly, Category = "Shop|Repair")
	TObjectPtr<USoundBase> RepairSound;

	/** 수리 상품 아이콘 */
	UPROPERTY(EditDefaultsOnly, Category = "Shop|Repair")
	TObjectPtr<UTexture2D> RepairIcon;

	/** 상품 이름 아래 한 줄: 수리 / 부활 / 합류 / 반복 강화({0} = 지금까지 산 횟수) */
	UPROPERTY(EditDefaultsOnly, Category = "Shop|Text")
	FText RepairDetail;

	UPROPERTY(EditDefaultsOnly, Category = "Shop|Text")
	FText ReviveDetail;

	UPROPERTY(EditDefaultsOnly, Category = "Shop|Text")
	FText RecruitDetail;

	UPROPERTY(EditDefaultsOnly, Category = "Shop|Text")
	FText UpgradeDetailFormat;

	/** 수리 상품 이름. {0} = 회복량(%) */
	UPROPERTY(EditDefaultsOnly, Category = "Shop|Text")
	FText RepairTitleFormat;

	/** 수리·부활·합류 상품 효과 (이름 아래 크게). 수리는 {0} = 회복량(%) */
	UPROPERTY(EditDefaultsOnly, Category = "Shop|Text")
	FText RepairEffectFormat;

	UPROPERTY(EditDefaultsOnly, Category = "Shop|Text")
	FText ReviveEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Shop|Text")
	FText RecruitEffect;

	/** 부활 상품 이름. {0} = 슬롯 번호, {1} = 부하 종류 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "Shop|Text")
	FText ReviveTitleFormat;

	/** 반복 강화 상품. 분대가 가득 차도 항상 판다 (기획서) */
	UPROPERTY(EditDefaultsOnly, Category = "Shop|Upgrade")
	TArray<TObjectPtr<UUSShopUpgradeData>> Upgrades;

	/** 반복 강화 구매음 (일시정지 중이라 UI 소리로 재생) */
	UPROPERTY(EditDefaultsOnly, Category = "Shop|Upgrade")
	TObjectPtr<USoundBase> UpgradeSound;

	/**
	 * 부하 합류를 파는가. D6에서는 편성을 격납고가 맡아 껐지만, 판 안에서 분대가 안 늘어 "부하 늘리는 재미"가 없었다
	 * (사용자 플레이 2026-10-21: 편성 3칸뿐) → 다시 켬. 분대 상한(UUSSquadComponent::MaxMembers 8)까지
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Shop|Recruit")
	bool bOfferRecruits = true;

	/** 첫 합류 가격 */
	UPROPERTY(EditDefaultsOnly, Category = "Shop|Recruit", meta = (ClampMin = "0"))
	int32 RecruitBaseCost = 60;

	/** 합류할 때마다 가격이 곱해지는 비율 (세 종류 공통) */
	UPROPERTY(EditDefaultsOnly, Category = "Shop|Recruit", meta = (ClampMin = "1.0"))
	float RecruitCostGrowth = 1.35f;

	/** 합류 상품 이름. {0} = 부하 종류 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "Shop|Text")
	FText RecruitTitleFormat;

private:
	/** 지금 보여 주는 상품 목록 */
	TArray<FUSShopOffer> Offers;

	/** 이번 런에서 수리한 횟수 (가격 계산) */
	int32 RepairCount = 0;

	/** Upgrades와 같은 순서로, 이번 런에서 각 강화를 산 횟수 (가격·Lv 표시) */
	TArray<int32> UpgradeCounts;

	/** 이번 런에서 합류시킨 횟수 (가격 계산) */
	int32 RecruitCount = 0;
};
