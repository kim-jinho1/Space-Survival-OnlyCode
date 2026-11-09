#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "USAbilityData.generated.h"

class UTexture2D;
class UUSAllyTypeData;
class UUSBuildData;

/** 능력 종류. 종류마다 Value·SecondaryValue의 뜻이 다르다 (UUSAbilityData::ApplyTo 참고) */
UENUM(BlueprintType)
enum class EUSAbilityType : uint8
{
	/** 감속 미사일: 주기적으로 가까운 적 여러 곳에 폭발해 감속 (대장). Value = 표적 수, Secondary = 감속 시간(초) */
	MissileBarrage,
	/** 총알이 적을 더 뚫음. Value = 추가로 뚫는 적 수 */
	Pierce,
	/** 한 번에 여러 발을 부채꼴로. Value = 발 수, Secondary = 한 발 피해 배율 */
	Spread,
	/** 받은 피해 일부를 때린 적에게. Value = 되돌리는 비율 (0.3 = 30%) */
	Reflect,
	/** 도발 반경·충각 피해. Value = 도발 반경(cm), Secondary = 충각 피해 배율 */
	Bulwark,
	/** EMP 펄스에 피해. Value = 펄스 한 번 피해 (부하 공격력 배율 적용 전) */
	EmpShock,
	/** EMP 범위·감속 시간. Value = 펄스 반경(cm), Secondary = 감속 지속(초) */
	EmpField,
	/** 표적 지정 (대장): 대장이 맞힌 적이 부하에게 더 아프게 맞는다. Value = 부하 피해 보너스(0.5 = +50%), Secondary = 지속(초) */
	TargetMark,
	/** 폭발탄 (대장): 총알이 적에게 맞으면 확률로 그 자리에서 폭발. Value = 확률(0.1 = 10%), Secondary = 폭발 피해 배율(명중 피해 기준) */
	ExplosiveRounds
};

/** 능력 한 단계의 수치와 카드 설명 */
USTRUCT(BlueprintType)
struct FUSAbilityLevel
{
	GENERATED_BODY()

	/** 주 수치 (뜻은 EUSAbilityType 주석) */
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	float Value = 0.0f;

	/** 보조 수치 (안 쓰는 종류는 무시) */
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	float SecondaryValue = 0.0f;

	/** 이 단계 카드 효과 한 줄 (예: "6초마다 적 2곳에 미사일") */
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	FText Description;

	/** 효과 아래 회색 보충 설명 (예: "한 발 피해 ×0.6"). 비우면 카드에서 숨긴다 */
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	FText Detail;
};

/**
 * 레벨업 능력 카드 하나 (애셋 하나 = 능력 하나). 소유 기종(OwnerType)이 비어 있으면 대장 능력이다.
 * 단계는 Levels 배열 길이만큼(기획서: 3단계). 단계 수치는 "절대값"이라 몇 번을 다시 적용해도 결과가 같다 —
 * 부활·새로 합류한 부하에게 현재 단계를 그대로 다시 적용하면 된다.
 */
UCLASS(BlueprintType)
class US_API UUSAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	FORCEINLINE EUSAbilityType GetAbilityType() const { return AbilityType; }
	FORCEINLINE const UUSAllyTypeData* GetOwnerType() const { return OwnerType; }
	FORCEINLINE const UUSBuildData* GetBuild() const { return Build; }
	FORCEINLINE bool IsLeaderAbility() const { return OwnerType == nullptr; }
	FORCEINLINE const FText& GetDisplayName() const { return DisplayName; }
	FORCEINLINE UTexture2D* GetIcon() const { return Icon; }
	FORCEINLINE int32 GetMaxLevel() const { return Levels.Num(); }

	/** Level(1부터) 단계 데이터. 범위 밖이면 nullptr */
	const FUSAbilityLevel* GetLevelData(int32 Level) const;

	/** Target 기체에 Level 단계를 적용한다 (해당 컴포넌트·무기를 찾아 값을 정한다). 받을 곳이 없으면 Warning 후 false */
	bool ApplyTo(AActor* Target, int32 Level) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	EUSAbilityType AbilityType = EUSAbilityType::Pierce;

	/** 이 능력을 받는 부하 기종. 비우면 대장 능력 */
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	TObjectPtr<UUSAllyTypeData> OwnerType;

	/** 이 능력이 속한 빌드 (카드 색 띠·가중치·완성 보너스). 비우면 빌드 없는 능력 */
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	TObjectPtr<UUSBuildData> Build;

	/** 카드 제목 (단계는 카드가 붙인다) */
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	FText DisplayName;

	/** 카드 아이콘 (없으면 숨김) */
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	TObjectPtr<UTexture2D> Icon;

	/** 단계별 수치. [0] = 1단계 */
	UPROPERTY(EditDefaultsOnly, Category = "Ability")
	TArray<FUSAbilityLevel> Levels;
};
