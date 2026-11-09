#pragma once

#include "CoreMinimal.h"
#include "USLevelUpCard.generated.h"

class UTexture2D;
class UUSAbilityData;
class UUSBuildData;

/** 레벨업 카드 종류 */
UENUM(BlueprintType)
enum class EUSLevelUpCardKind : uint8
{
	/** 능력을 배우거나 다음 단계로 */
	Ability,
	/** 보급: 골드 (능력 후보가 모자랄 때 채움) */
	SupplyGold,
	/** 보급: 대장·부하 수리 (능력 후보가 모자랄 때 채움) */
	SupplyRepair
};

/**
 * 레벨업 카드 한 장. UUSUpgradeComponent::GenerateChoices가 만들고, 위젯은 글자·아이콘만 보여 주고, 고른 카드를 그대로 돌려준다.
 */
USTRUCT(BlueprintType)
struct FUSLevelUpCard
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "LevelUp")
	EUSLevelUpCardKind Kind = EUSLevelUpCardKind::Ability;

	/** Kind가 Ability일 때의 능력 (const 포인터라 BP에는 노출하지 않는다) */
	UPROPERTY()
	TObjectPtr<const UUSAbilityData> Ability;

	/** 이 카드를 고르면 될 단계 (1 = 새로 배움) */
	UPROPERTY(BlueprintReadOnly, Category = "LevelUp")
	int32 NextLevel = 0;

	/** 한 줄 제목 ("기관포기 · 확산 사격 1단계"). 칩·이름 칸이 없는 카드 WBP와 로그가 쓴다 */
	UPROPERTY(BlueprintReadOnly, Category = "LevelUp")
	FText Title;

	/** 카드 위 칩: 어느 기체의 능력인가 ("기관포기"·"대장"·"보급") */
	UPROPERTY(BlueprintReadOnly, Category = "LevelUp")
	FText Tag;

	/** 칩 아래 큰 이름 ("확산 사격"·"골드") */
	UPROPERTY(BlueprintReadOnly, Category = "LevelUp")
	FText Name;

	/** 단계 점 개수 (0이면 점 줄을 숨긴다 — 보급 카드) */
	UPROPERTY(BlueprintReadOnly, Category = "LevelUp")
	int32 MaxLevel = 0;

	/** 효과 한 줄 */
	UPROPERTY(BlueprintReadOnly, Category = "LevelUp")
	FText Description;

	/** 효과 아래 회색 보충 설명 (비면 위젯이 숨긴다) */
	UPROPERTY(BlueprintReadOnly, Category = "LevelUp")
	FText Detail;

	/** 카드 아이콘 (없으면 위젯이 숨긴다) */
	UPROPERTY(BlueprintReadOnly, Category = "LevelUp")
	TObjectPtr<UTexture2D> Icon;

	/** 카드 위 색 띠 글자 ("포격 빌드"·"보급") */
	UPROPERTY(BlueprintReadOnly, Category = "LevelUp|Build")
	FText BandText;

	/** 색 띠 바탕 */
	UPROPERTY(BlueprintReadOnly, Category = "LevelUp|Build")
	FLinearColor BandColor = FLinearColor::Gray;

	/** 빌드 밝은 색 (단계 점) */
	UPROPERTY(BlueprintReadOnly, Category = "LevelUp|Build")
	FLinearColor AccentColor = FLinearColor(0.37f, 0.88f, 0.95f, 1.0f);

	/** 색 띠 아이콘 (없으면 위젯이 숨긴다) */
	UPROPERTY(BlueprintReadOnly, Category = "LevelUp|Build")
	TObjectPtr<UTexture2D> BandIcon;
};

/** 빌드 하나의 이번 판 진행 (화면 위 진행 줄) */
USTRUCT(BlueprintType)
struct FUSBuildProgress
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<const UUSBuildData> Build;

	/** 배운 능력 수 (1단계 이상) */
	UPROPERTY(BlueprintReadOnly, Category = "LevelUp|Build")
	int32 Learned = 0;

	/** 이 빌드의 능력 수 */
	UPROPERTY(BlueprintReadOnly, Category = "LevelUp|Build")
	int32 Total = 0;

	UPROPERTY(BlueprintReadOnly, Category = "LevelUp|Build")
	bool bComplete = false;
};
