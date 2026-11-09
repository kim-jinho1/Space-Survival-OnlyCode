#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USExperienceComponent.generated.h"

/** 경험치 변경 알림 (CurrentExperiencePoints, TargetExperiencePoints) */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDelegateOnExperiencePointsChanged, float, float);

/** 레벨 변경 알림 (CurrentLevel) */
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateOnLevelChanged, int);

/** 레벨업 알림 */
DECLARE_MULTICAST_DELEGATE(FDelegateOnLevelUp);

enum class EUSEffectType : uint8;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class US_API UUSExperienceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUSExperienceComponent();

public:
	FORCEINLINE float GetCurrentExperiencePoints() const { return CurrentExperiencePoints; }
	FORCEINLINE float GetTargetExperiencePoints() const { return TargetExperiencePoints; }

	FORCEINLINE int32 GetCurrentLevel() const { return CurrentLevel; }

public:
	/** 레벨업 */
	void LevelUp();

	/** 경험치 추가 (경험치 획득 배율을 곱한다) */
	void AddPoints(float Amount);

	/** 경험치 획득 배율에 Factor를 곱한다 (영구 업그레이드, 1.1 = +10%) */
	void ScaleExpGain(float Factor);
	
public:	
	/** 경험치가 변경될 때 호출 */
	FDelegateOnExperiencePointsChanged OnExperiencePointsChanged;
	
	/** 레벨업이 될때 호출 */
	FDelegateOnLevelUp OnLevelUp;

	/** 레벨이 변경될 때 호출 (CurrentLevel) */
	FDelegateOnLevelChanged OnLevelChanged;
	
protected:
	/** 목표 경험치 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Attribute)
	float TargetExperiencePoints = 100.0f;
	
	/** 현재 경험치 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Attribute)
	float CurrentExperiencePoints = 0.0f;
	
	/** 레벨업 시 목표 경험치 증가율 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Attribute, meta = (ClampMin = 1.0))
	float LevelExpGrowth = 1.2f;

	/** 현재 레벨 (1부터 시작) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Attribute)
	int32 CurrentLevel = 1;

	/** 경험치 획득 배율 (런타임 상태, 영구 업그레이드로 곱해진다) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Attribute)
	float ExpGainMultiplier = 1.0f;
	
};
