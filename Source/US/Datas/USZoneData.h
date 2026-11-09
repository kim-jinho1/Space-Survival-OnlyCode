#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "USZoneData.generated.h"

class UTexture2D;
class UUSEnemyTypeData;
class UUSStageData;
class UWorld;

/**
 * 출격할 구역 하나 (격납고 출격 지도의 섬 하나). 애셋 하나 = 구역 하나.
 * 지도는 IslandImage를 MapPosition에 띄우고, 섬을 고르면 설명·출몰하는 적·목표를 정보 상자에 보여 준다.
 * 데모는 1구역만 열고 나머지는 bLockedInDemo로 잠가 "정식판"으로 보여 준다 (기획서 "구역(맵) 구조").
 */
UCLASS(BlueprintType)
class US_API UUSZoneData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	FORCEINLINE const FText& GetDisplayName() const { return DisplayName; }
	FORCEINLINE const FText& GetDescription() const { return Description; }
	FORCEINLINE const TSoftObjectPtr<UWorld>& GetMap() const { return Map; }
	FORCEINLINE bool IsLockedInDemo() const { return bLockedInDemo; }
	FORCEINLINE UTexture2D* GetIslandImage() const { return IslandImage; }
	FORCEINLINE const FVector2D& GetMapPosition() const { return MapPosition; }
	FORCEINLINE const TArray<TObjectPtr<UUSEnemyTypeData>>& GetEnemies() const { return Enemies; }
	FORCEINLINE const FText& GetObjectiveText() const { return ObjectiveText; }
	FORCEINLINE const UUSEnemyTypeData* GetBoss() const { return Boss; }
	FORCEINLINE const TArray<TObjectPtr<UUSStageData>>& GetStages() const { return Stages; }
	FORCEINLINE bool HasStages() const { return !Stages.IsEmpty(); }
	FORCEINLINE UTexture2D* GetStageMapImage() const { return StageMapImage; }
	FORCEINLINE const FVector2D& GetStageMapSize() const { return StageMapSize; }
	FORCEINLINE const FIntPoint& GetRegionIdMapSize() const { return RegionIdMapSize; }

	/** 지역 번호 지도 칸 (X, Y)의 스테이지 번호(Stages 인덱스). 밖이거나 바다면 INDEX_NONE */
	int32 GetRegionIdAt(int32 X, int32 Y) const
	{
		if (X < 0 || Y < 0 || X >= RegionIdMapSize.X || Y >= RegionIdMapSize.Y || RegionIdMap.Num() != RegionIdMapSize.X * RegionIdMapSize.Y)
		{
			return INDEX_NONE;
		}
		const int32 Index = RegionIdMap[Y * RegionIdMapSize.X + X];
		return Stages.IsValidIndex(Index) ? Index : INDEX_NONE;
	}

	/** 스테이지 번호(1부터)의 적 체력 배율 = StageHealthGrowth ^ (번호 - 1) */
	float GetStageHealthMultiplier(int32 StageNumber) const
	{
		return FMath::Pow(StageHealthGrowth, static_cast<float>(FMath::Max(0, StageNumber - 1)));
	}

	/** 스테이지 번호의 물량 배율 (웨이브당 수·동시 생존 상한) = 1 + StageAmountGrowth × (번호 - 1) */
	float GetStageAmountMultiplier(int32 StageNumber) const
	{
		return 1.0f + StageAmountGrowth * static_cast<float>(FMath::Max(0, StageNumber - 1));
	}

	/** 지역 지도 한 칸(RegionIdMap)의 스테이지 (지도 영역 비율 UV). 바다·바깥이면 nullptr */
	const UUSStageData* FindStageAt(const FVector2D& UV) const
	{
		if (RegionIdMapSize.X <= 0 || RegionIdMapSize.Y <= 0 || RegionIdMap.Num() != RegionIdMapSize.X * RegionIdMapSize.Y
			|| UV.X < 0.0f || UV.Y < 0.0f || UV.X >= 1.0f || UV.Y >= 1.0f)
		{
			return nullptr;
		}
		const int32 X = FMath::FloorToInt(UV.X * RegionIdMapSize.X);
		const int32 Y = FMath::FloorToInt(UV.Y * RegionIdMapSize.Y);
		const int32 Index = RegionIdMap[Y * RegionIdMapSize.X + X];
		return Stages.IsValidIndex(Index) ? Stages[Index].Get() : nullptr;
	}

protected:
	/** 지도에 보일 이름 */
	UPROPERTY(EditDefaultsOnly, Category = "Zone")
	FText DisplayName;

	/** 정보 상자 설명 */
	UPROPERTY(EditDefaultsOnly, Category = "Zone", meta = (MultiLine = true))
	FText Description;

	/** 출격하면 여는 맵. 소프트 참조라 격납고가 전투 맵을 미리 메모리에 올리지 않는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Zone")
	TSoftObjectPtr<UWorld> Map;

	/** 데모에서 잠긴 구역 (고를 수 없고 "정식판"으로 표시) */
	UPROPERTY(EditDefaultsOnly, Category = "Zone")
	bool bLockedInDemo = false;

	/** 출격 지도의 섬 그림 */
	UPROPERTY(EditDefaultsOnly, Category = "Zone|Map")
	TObjectPtr<UTexture2D> IslandImage;

	/** 지도 위 섬 가운데 위치. 지도 영역 왼쪽 위 (0, 0) ~ 오른쪽 아래 (1, 1) 비율이라 해상도가 바뀌어도 그대로다 */
	UPROPERTY(EditDefaultsOnly, Category = "Zone|Map")
	FVector2D MapPosition = FVector2D(0.5f, 0.5f);

	/** 출몰하는 적 (정보 상자에 그림·이름으로). 보여 주기용이라 스포너 설정과는 따로 적는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Zone|Map")
	TArray<TObjectPtr<UUSEnemyTypeData>> Enemies;

	/** 목표 한 줄 ("5:00 버티기") */
	UPROPERTY(EditDefaultsOnly, Category = "Zone|Map")
	FText ObjectiveText;

	/** 이 구역 보스 (정보 상자에 "보스" 줄로). 비우면 줄을 숨긴다. 보여 주기용이라 스포너의 BossEnemyType과 따로 적는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Zone|Map")
	TObjectPtr<UUSEnemyTypeData> Boss;

	/** 이 행성의 스테이지 (지역 지도 순서 = RegionIdMap 번호). 비우면 예전처럼 스테이지 없이 바로 편대로 */
	UPROPERTY(EditDefaultsOnly, Category = "Zone|Stage")
	TArray<TObjectPtr<UUSStageData>> Stages;

	/** 스테이지마다 적 체력에 곱해지는 배율 (밸런스 값 1/3, Docs/BALANCE.md) */
	UPROPERTY(EditDefaultsOnly, Category = "Zone|Stage", meta = (ClampMin = "1.0"))
	float StageHealthGrowth = 1.22f;

	/** 스테이지마다 늘어나는 물량 비율 (밸런스 값 2/3, 0.1 = 스테이지당 +10%) */
	UPROPERTY(EditDefaultsOnly, Category = "Zone|Stage", meta = (ClampMin = "0.0"))
	float StageAmountGrowth = 0.1f;

	/** 지역 지도 바탕 (바다·해안선). 지역 그림은 스테이지마다 그 위에 얹는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Zone|Stage")
	TObjectPtr<UTexture2D> StageMapImage;

	/** 지역 지도 원본 크기(px). 위젯이 이 비율을 지킨다 */
	UPROPERTY(EditDefaultsOnly, Category = "Zone|Stage")
	FVector2D StageMapSize = FVector2D(1100.0f, 620.0f);

	/** 클릭 판정용 지역 번호 지도 (가로 × 세로, 칸 = Stages 번호, 255 = 바다). Tools/gen_stage_map.py가 만든다 */
	UPROPERTY(EditDefaultsOnly, Category = "Zone|Stage")
	TArray<uint8> RegionIdMap;

	UPROPERTY(EditDefaultsOnly, Category = "Zone|Stage")
	FIntPoint RegionIdMapSize = FIntPoint::ZeroValue;
};
