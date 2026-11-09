#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "USMusicSetData.generated.h"

class USoundBase;

/** 음악 한 겹: 같은 역할(타악·패드·베이스…)의 조각 여러 개 중 하나를 마디 박자에 맞춰 튼다 */
USTRUCT(BlueprintType)
struct FUSMusicLayer
{
	GENERATED_BODY()

	/** 고를 조각들 (한 번에 하나, 직전과 다른 것을 고른다) */
	UPROPERTY(EditAnywhere, Category = "Music")
	TArray<TObjectPtr<USoundBase>> Sounds;

	/** 몇 마디마다 새 조각을 시작할지 */
	UPROPERTY(EditAnywhere, Category = "Music", meta = (ClampMin = "1"))
	int32 EveryBars = 1;

	/** 이 마디부터 들어온다 (처음엔 패드만 → 타악 → 리드처럼 겹을 쌓아 시작) */
	UPROPERTY(EditAnywhere, Category = "Music", meta = (ClampMin = "0"))
	int32 StartBar = 0;

	/** 차례가 와도 이 확률로만 튼다 (쉬는 마디를 만들어 반복감을 줄인다) */
	UPROPERTY(EditAnywhere, Category = "Music", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Chance = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Music", meta = (ClampMin = "0.0"))
	float Volume = 1.0f;
};

/**
 * 맵 배경음악 = 짧은 음악 조각(Lyra 스템)을 마디 시계에 맞춰 겹겹이 이어 붙이는 규칙.
 * 완성곡이 없고 조각은 3~13초라 하나를 반복하면 금방 질린다 → 겹마다 무작위로 골라 매번 조금씩 다르게 들리게 한다.
 * 조각 길이는 모두 박자(약 0.68초)의 배수 + 잔향이라, 마디 시작에만 틀면 서로 박자가 맞는다.
 */
UCLASS(BlueprintType)
class US_API UUSMusicSetData : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 한 마디 길이(초). Lyra 전투 스템의 타악 한 조각 = 1마디 = 2.7초 */
	UPROPERTY(EditAnywhere, Category = "Music", meta = (ClampMin = "0.5"))
	float BarSeconds = 2.7f;

	UPROPERTY(EditAnywhere, Category = "Music")
	TArray<FUSMusicLayer> Layers;
};
