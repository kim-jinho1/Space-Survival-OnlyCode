#include "USExperienceComponent.h"
#include "US.h"

UUSExperienceComponent::UUSExperienceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUSExperienceComponent::LevelUp()
{
	CurrentLevel++;
	TargetExperiencePoints *= LevelExpGrowth;

	OnLevelUp.Broadcast();
	OnLevelChanged.Broadcast(CurrentLevel);
}

void UUSExperienceComponent::ScaleExpGain(float Factor)
{
	if (Factor <= 0.0f)
	{
		UE_LOG(LogUS, Warning, TEXT("[USExperience] ScaleExpGain ignored non-positive factor %f"), Factor);
		return;
	}

	ExpGainMultiplier *= Factor;
}

void UUSExperienceComponent::AddPoints(const float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	CurrentExperiencePoints += Amount * ExpGainMultiplier;

	// 한 번의 호출에서 허용할 최대 레벨업 횟수.
	// LevelExpGrowth 설정이 잘못되어 목표치가 늘지 않으면 루프가 끝나지 않으므로 상한을 둔다.
	constexpr int32 MaxLevelUpsPerCall = 100;
	int32 SafetyCounter = 0;

	// 목표 도달 시 초과분을 이월하며 반복 레벨업.
	// 목표치가 0 이하이면 레벨업 판정 자체가 불가능하므로 루프에 들어가지 않는다.
	while (TargetExperiencePoints > 0.0f && CurrentExperiencePoints >= TargetExperiencePoints)
	{
		CurrentExperiencePoints -= TargetExperiencePoints;
		LevelUp();

		if (++SafetyCounter >= MaxLevelUpsPerCall)
		{
			UE_LOG(LogUS, Warning,
				TEXT("[USExperience] 레벨업이 %d회를 넘어 중단했습니다. LevelExpGrowth(%.3f)와 TargetExperiencePoints(%.3f)를 확인하세요."),
				MaxLevelUpsPerCall, LevelExpGrowth, TargetExperiencePoints);

			CurrentExperiencePoints = 0.0f;
			break;
		}
	}

	// 레벨업이 없었더라도 경험치는 올랐으므로 항상 알린다.
	OnExperiencePointsChanged.Broadcast(CurrentExperiencePoints, TargetExperiencePoints);
}
