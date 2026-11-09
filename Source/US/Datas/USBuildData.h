#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "USBuildData.generated.h"

class UTexture2D;

/** 빌드 완성 보너스 종류 */
UENUM(BlueprintType)
enum class EUSBuildBonusType : uint8
{
	/** 대장·부하 전원 공격력. BonusValue = 곱셈 증가율 (0.2 = ×1.2) */
	AttackDamage,
	/** 대장·부하 전원 최대 체력. BonusValue = 곱셈 증가율 (0.25 = ×1.25) */
	MaxHealth,
	/** 감속 미사일·EMP 펄스의 감속 시간. BonusValue = 더할 초 */
	SlowDuration
};

/**
 * 레벨업 능력 카드의 빌드 하나 (포격·제압·철벽). 능력 애셋(UUSAbilityData::Build)이 이 애셋을 가리킨다.
 * 카드 위 색 띠와 화면 위 빌드 진행 줄에 이름·색을 쓰고, 이 빌드의 능력을 모두 배우면(각 1단계 이상) 완성 보너스를 한 번 준다.
 * 근거: Hades 은총 카드(신마다 색·아이콘), Brotato 태그 가중치(가진 태그의 아이템이 더 잘 나옴).
 */
UCLASS(BlueprintType)
class US_API UUSBuildData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	FORCEINLINE const FText& GetDisplayName() const { return DisplayName; }
	FORCEINLINE const FText& GetBandText() const { return BandText; }
	FORCEINLINE const FLinearColor& GetColor() const { return Color; }
	FORCEINLINE const FLinearColor& GetBandColor() const { return BandColor; }
	FORCEINLINE UTexture2D* GetIcon() const { return Icon; }
	FORCEINLINE EUSBuildBonusType GetBonusType() const { return BonusType; }
	FORCEINLINE float GetBonusValue() const { return BonusValue; }
	FORCEINLINE const FText& GetBonusText() const { return BonusText; }

protected:
	/** 진행 줄에 쓸 짧은 이름 ("포격") */
	UPROPERTY(EditDefaultsOnly, Category = "Build")
	FText DisplayName;

	/** 카드 색 띠 글자 ("포격 빌드") */
	UPROPERTY(EditDefaultsOnly, Category = "Build")
	FText BandText;

	/** 밝은 색: 진행 줄 글자·단계 점 */
	UPROPERTY(EditDefaultsOnly, Category = "Build")
	FLinearColor Color = FLinearColor::White;

	/** 어두운 색: 카드 위 띠 바탕 */
	UPROPERTY(EditDefaultsOnly, Category = "Build")
	FLinearColor BandColor = FLinearColor::Gray;

	/** 띠 아이콘 (없으면 숨김) */
	UPROPERTY(EditDefaultsOnly, Category = "Build")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditDefaultsOnly, Category = "Bonus")
	EUSBuildBonusType BonusType = EUSBuildBonusType::AttackDamage;

	/** 보너스 수치 (뜻은 EUSBuildBonusType 주석) */
	UPROPERTY(EditDefaultsOnly, Category = "Bonus", meta = (ClampMin = "0.0"))
	float BonusValue = 0.2f;

	/** 완성 뒤 진행 줄에 쓸 보너스 설명 ("공격력 +20%") */
	UPROPERTY(EditDefaultsOnly, Category = "Bonus")
	FText BonusText;
};
