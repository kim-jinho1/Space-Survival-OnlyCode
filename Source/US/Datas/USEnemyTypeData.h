#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "USEnemyTypeData.generated.h"

class AUSEnemyCharacter;
class UMaterialInterface;
class UParticleSystem;
class UStaticMesh;
class UTexture2D;

/**
 * 적 종류 하나의 정의. 어떤 BP로 스폰할지와 시작 스탯·외형을 담는다.
 * 스포너가 이 데이터로 적을 지연 스폰하고, 적은 BeginPlay에서 스탯과 외형을 적용한다 (부하의 UUSAllyTypeData와 같은 방식).
 * 행동이 다른 적(기뢰의 자폭 등)은 EnemyClass(C++ 자식 + BP)로, 숫자와 외형은 여기에 둔다.
 */
UCLASS(BlueprintType)
class US_API UUSEnemyTypeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	FORCEINLINE TSubclassOf<AUSEnemyCharacter> GetEnemyClass() const { return EnemyClass; }
	FORCEINLINE const FText& GetDisplayName() const { return DisplayName; }
	FORCEINLINE const FText& GetSummary() const { return Summary; }
	FORCEINLINE UTexture2D* GetIcon() const { return Icon; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetMoveSpeed() const { return MoveSpeed; }
	FORCEINLINE float GetContactDamage() const { return ContactDamage; }
	FORCEINLINE float GetExperienceReward() const { return ExperienceReward; }
	FORCEINLINE int32 GetGoldReward() const { return GoldReward; }
	FORCEINLINE UStaticMesh* GetMesh() const { return Mesh; }
	FORCEINLINE float GetVisualScale() const { return VisualScale; }
	FORCEINLINE const TArray<TObjectPtr<UMaterialInterface>>& GetOverrideMaterials() const { return OverrideMaterials; }
	FORCEINLINE UParticleSystem* GetTrailFX() const { return TrailFX; }
	FORCEINLINE const TArray<FVector>& GetTrailOffsets() const { return TrailOffsets; }
	FORCEINLINE float GetTrailScale() const { return TrailScale; }

protected:
	/** 스폰할 적 BP (행동이 다른 적은 C++ 자식 클래스의 BP) */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	TSubclassOf<AUSEnemyCharacter> EnemyClass;

	/** 화면에 보일 이름 (격납고 출격 지도의 구역 정보) */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Info")
	FText DisplayName;

	/** 한 줄 특징 (처음 만났을 때 소개, R-41 5). 예: "가까이 오면 터진다" */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Info")
	FText Summary;

	/** 흰색 한 가지 색 그림 (출격 지도에서 적 색을 곱해 칠한다) */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Info")
	TObjectPtr<UTexture2D> Icon;

	/** 시작 최대 체력. 스포너가 경과 시간 배율을 곱해 넣는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "1.0"))
	float MaxHealth = 30.0f;

	/** 이동 속도 */
	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0.0"))
	float MoveSpeed = 150.0f;

	/** 접촉 데미지 (기뢰는 폭발 피해를 BP에서 따로 정한다) */
	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0.0"))
	float ContactDamage = 10.0f;

	/** 처치 경험치 */
	UPROPERTY(EditDefaultsOnly, Category = "Reward", meta = (ClampMin = "0.0"))
	float ExperienceReward = 10.0f;

	/** 처치 골드 */
	UPROPERTY(EditDefaultsOnly, Category = "Reward", meta = (ClampMin = "0"))
	int32 GoldReward = 5;

	/** 외형 메시. 비우면 BP의 VisualMesh 그대로 */
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TObjectPtr<UStaticMesh> Mesh;

	/** BP에서 잡은 VisualMesh 크기에 곱하는 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "Visual", meta = (ClampMin = "0.1"))
	float VisualScale = 1.0f;

	/** 슬롯별 머티리얼 교체 (비운 칸은 그대로). 피격 발광이 되려면 적 머티리얼의 인스턴스여야 한다 */
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TArray<TObjectPtr<UMaterialInterface>> OverrideMaterials;

	/** 꼬리 트레일 (선택). 빠른 적만 넣어 화면에서 위협을 먼저 읽히게 한다. 비우면 트레일 없음 */
	UPROPERTY(EditDefaultsOnly, Category = "Visual|Trail")
	TObjectPtr<UParticleSystem> TrailFX;

	/** 트레일 위치 (VisualRoot 기준, 기체 꼬리). 기수가 +X이므로 꼬리는 -X 쪽 */
	UPROPERTY(EditDefaultsOnly, Category = "Visual|Trail")
	TArray<FVector> TrailOffsets;

	/** 트레일 굵기 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "Visual|Trail", meta = (ClampMin = "0.0"))
	float TrailScale = 1.0f;
};
