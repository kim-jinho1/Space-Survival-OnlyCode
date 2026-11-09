#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Progression/USProgressionTypes.h"
#include "USUpgradeOptionData.generated.h"

/**
 * (사용 중단, D5-c) 예전 레벨업 스탯 카드 한 줄. 레벨업 카드가 능력 카드(FUSLevelUpCard)로 바뀌어 아무도 읽지 않는다.
 * DT_UpgradeData가 이 구조체를 행 타입으로 쓰므로, 에디터에서 DT_UpgradeData를 지운 뒤에 이 파일을 지운다 (TODO.md).
 */
USTRUCT(BlueprintType)
struct FUSUpgradeOptionData : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Upgrade)
	EUSEffectType Type = EUSEffectType::HealHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Upgrade)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Upgrade)
	FText Description;

	/** 적용값. 공격력·공격 속도·최대 체력은 곱셈 증가율(0.15 = ×1.15), 이동 속도는 배율 가산치, 회복은 회복량 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Upgrade)
	float Value = 0.0f;
};
