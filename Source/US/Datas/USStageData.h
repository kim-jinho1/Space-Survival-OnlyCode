#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Spawning/USEnemySpawner.h"
#include "USStageData.generated.h"

class UTexture2D;
class UUSEnemyTypeData;
class UMaterialInterface;

/**
 * 행성(구역) 안의 스테이지 하나 = 지역 지도의 땅 한 칸 (Into the Breach 섬 지역 고르기, 사용자 결정 2026-10-26).
 * 한 판은 ClearTimeSeconds(1:30)를 버티면 클리어, 보스 스테이지는 그때 보스가 나오고 격파하면 클리어.
 * 깬 지역과 맞닿은 지역이 열리고, 보스 스테이지는 나머지를 다 깨야 열린다 (UUSMetaProgressionSubsystem::IsStageOpen).
 * 적 체력·물량 배율은 스테이지 번호로 구역 데이터(UUSZoneData)의 공식이 계산한다 — 스테이지마다 맞출 값은 벽 덤(HealthBonus) 하나뿐.
 */
UCLASS(BlueprintType)
class US_API UUSStageData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	FORCEINLINE FName GetStageId() const { return StageId; }
	FORCEINLINE int32 GetStageNumber() const { return StageNumber; }
	FORCEINLINE const FText& GetDisplayName() const { return DisplayName; }
	FORCEINLINE bool IsBossStage() const { return bBossStage; }
	FORCEINLINE const TArray<TObjectPtr<UUSStageData>>& GetNeighbors() const { return Neighbors; }
	FORCEINLINE const TArray<FUSEnemySpawnEntry>& GetSpawnTable() const { return SpawnTable; }
	FORCEINLINE UTexture2D* GetRegionImage() const { return RegionImage; }
	FORCEINLINE const FVector2D& GetRegionMin() const { return RegionMin; }
	FORCEINLINE const FVector2D& GetRegionMax() const { return RegionMax; }
	FORCEINLINE const FVector2D& GetLabelPosition() const { return LabelPosition; }
	FORCEINLINE const FLinearColor& GetRegionColor() const { return RegionColor; }
	FORCEINLINE float GetHealthBonus() const { return HealthBonus; }
	FORCEINLINE UMaterialInterface* GetBackdropMaterial() const { return BackdropMaterial; }
	FORCEINLINE const FVector& GetPlanetLocation() const { return PlanetLocation; }
	FORCEINLINE float GetPlanetRadius() const { return PlanetRadius; }

	/** 벽 스테이지 (체력 덤이 있어 격납고 강화를 사 와야 넘는 곳, 지도·진입 자막에 "위험") */
	FORCEINLINE bool IsWallStage() const { return HealthBonus > 1.0f; }

	/** 이 스테이지에 나오는 적 종류 (스폰 테이블에서 중복 없이, 지도 정보 카드용) */
	TArray<const UUSEnemyTypeData*> GetEnemyTypes() const
	{
		TArray<const UUSEnemyTypeData*> Types;
		for (const FUSEnemySpawnEntry& Entry : SpawnTable)
		{
			if (Entry.EnemyType)
			{
				Types.AddUnique(Entry.EnemyType);
			}
		}
		return Types;
	}

protected:
	/** 저장 키 (깬 스테이지 목록). 애셋 이름을 바꿔도 기록이 남게 따로 둔다 */
	UPROPERTY(EditDefaultsOnly, Category = "Stage")
	FName StageId;

	/** 스테이지 번호 (1부터). 적 체력·물량 공식과 "다음 스테이지" 순서가 이 번호를 쓴다 */
	UPROPERTY(EditDefaultsOnly, Category = "Stage", meta = (ClampMin = "1"))
	int32 StageNumber = 1;

	/** 지역 이름 ("빙하 평원") */
	UPROPERTY(EditDefaultsOnly, Category = "Stage")
	FText DisplayName;

	/** 보스 스테이지 (클리어 시각에 스포너 보스가 나온다, 나머지를 다 깨야 열린다) */
	UPROPERTY(EditDefaultsOnly, Category = "Stage")
	bool bBossStage = false;

	/** 맞닿은 지역 (한쪽에만 적어도 된다 — 열림 판정이 양쪽을 다 본다) */
	UPROPERTY(EditDefaultsOnly, Category = "Stage")
	TArray<TObjectPtr<UUSStageData>> Neighbors;

	/**
	 * 벽 스테이지 체력 덤: 번호 공식 배율 위에 더 곱한다 (1 = 보통). 판마다 처음부터 시작하므로(2026-10-27) 성장은 격납고 트리뿐 —
	 * 곡선만으로는 "여기서 막힌다"가 안 보여 4·7번에 계단을 둔다 (Nodebuster 보스 벽)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Stage", meta = (ClampMin = "1.0"))
	float HealthBonus = 1.0f;

	/** 이 스테이지의 스폰 테이블. 비우면 맵 스포너의 테이블 그대로 */
	UPROPERTY(EditDefaultsOnly, Category = "Stage|Spawn")
	TArray<FUSEnemySpawnEntry> SpawnTable;

	/** 전투 맵 먼 별판에 씌울 배경 그림 재질 (성운·행성 …, AUSStageBackdrop). 맵이 하나라 지역 느낌은 이것으로 낸다 */
	UPROPERTY(EditDefaultsOnly, Category = "Stage|Backdrop")
	TObjectPtr<UMaterialInterface> BackdropMaterial;

	/**
	 * 구역 행성(3D 구)의 월드 위치. 경기장 아래 깊은 곳(Z 음수)에 띄운다. 같은 구역이면 같은 행성이고 스테이지마다 위치·크기만 다르다 —
	 * 번호가 오를수록 가깝고 크게 (그림 행성은 비스듬한 판 위에서 찌그러지고 조명을 안 받아 버림, 사용자 2026-10-28)
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Stage|Backdrop")
	FVector PlanetLocation = FVector(0.0f, 0.0f, -12000.0f);

	/** 행성 반지름 (uu). 0이면 행성을 숨긴다 */
	UPROPERTY(EditDefaultsOnly, Category = "Stage|Backdrop", meta = (ClampMin = "0.0"))
	float PlanetRadius = 0.0f;

	/** 지역 그림 (흰 지형 + 굵은 외곽선, RegionColor를 곱한다). Tools/gen_stage_map.py가 만든다 */
	UPROPERTY(EditDefaultsOnly, Category = "Stage|Map")
	TObjectPtr<UTexture2D> RegionImage;

	/** 지역 그림이 놓일 지도 영역 비율 (왼쪽 위 0,0 ~ 오른쪽 아래 1,1) */
	UPROPERTY(EditDefaultsOnly, Category = "Stage|Map")
	FVector2D RegionMin = FVector2D::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Stage|Map")
	FVector2D RegionMax = FVector2D(1.0f, 1.0f);

	/** 번호 글자 자리 (지도 영역 비율) */
	UPROPERTY(EditDefaultsOnly, Category = "Stage|Map")
	FVector2D LabelPosition = FVector2D(0.5f, 0.5f);

	/** 지형 색 (얼음·바위·사막 …, 보스는 용암) */
	UPROPERTY(EditDefaultsOnly, Category = "Stage|Map")
	FLinearColor RegionColor = FLinearColor(0.4f, 0.6f, 0.65f, 1.0f);
};
