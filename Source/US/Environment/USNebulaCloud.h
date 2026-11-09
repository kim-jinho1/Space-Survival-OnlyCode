#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "USNebulaCloud.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;

/** 연기 판을 흩뿌리는 모양 */
UENUM(BlueprintType)
enum class EUSNebulaShape : uint8
{
	/** 액터 위치를 중심으로 반경 안에 뭉친 덩어리 */
	Blob,
	/** 액터 위치를 중심으로 한 직사각형 테두리를 따라 두껍게 (맵 끝 구름 벽) */
	Border,
};

/**
 * 성운 가스 구름 (배경 장식, 충돌 없음).
 * 연기 판(엔진 Plane)을 InstancedStaticMesh로 여러 장 겹쳐 한 덩어리를 만든다 — 판 N장이 그리기 호출 1번이다.
 * 판마다 다른 연기 칸을 고르는 건 재질(M_USNebulaPuff)이 PerInstanceRandom으로 한다.
 * 배치는 Seed로 정해지는 의사 난수라 에디터에서 보이는 모습과 게임 모습이 같다 (OnConstruction에서 만든다).
 * 바닥 재질로 그린 노이즈 구름은 납작해 보여서(2026-10-19 비교) 실제 연기 텍스처를 겹치는 쪽을 택했다.
 */
UCLASS()
class US_API AUSNebulaCloud : public AActor
{
	GENERATED_BODY()

public:
	AUSNebulaCloud();

	virtual void OnConstruction(const FTransform& Transform) override;

protected:
	/** 판 위치·크기·회전을 Seed로 다시 만든다 */
	void RebuildPuffs();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Puffs;

	/** 연기 판 재질 (M_USNebulaPuff 또는 그 인스턴스) */
	UPROPERTY(EditAnywhere, Category = "Nebula")
	TObjectPtr<UMaterialInterface> PuffMaterial;

	UPROPERTY(EditAnywhere, Category = "Nebula")
	EUSNebulaShape Shape = EUSNebulaShape::Blob;

	/** 판 개수 */
	UPROPERTY(EditAnywhere, Category = "Nebula", meta = (ClampMin = "1", ClampMax = "1000"))
	int32 PuffCount = 40;

	/** 같은 Seed면 같은 모양 */
	UPROPERTY(EditAnywhere, Category = "Nebula")
	int32 Seed = 1;

	/** 판 한 장의 지름(cm) 범위. X = 최소, Y = 최대 */
	UPROPERTY(EditAnywhere, Category = "Nebula")
	FVector2D PuffSize = FVector2D(1200.0f, 2800.0f);

	/** Blob: 판 중심이 놓이는 반경(cm). 가운데일수록 촘촘하고 판이 크다 */
	UPROPERTY(EditAnywhere, Category = "Nebula|Blob", meta = (ClampMin = "0.0"))
	float Radius = 2500.0f;

	/** Border: 테두리 직사각형의 반 크기(cm). 맵 벽 위치와 맞춘다 */
	UPROPERTY(EditAnywhere, Category = "Nebula|Border")
	FVector2D BorderHalfExtent = FVector2D(13550.0f, 13550.0f);

	/** Border: 테두리 안쪽으로 파고드는 깊이(cm). 맵 안에서 구름 끝이 보이는 정도 */
	UPROPERTY(EditAnywhere, Category = "Nebula|Border", meta = (ClampMin = "0.0"))
	float BorderInset = 900.0f;

	/** Border: 테두리 바깥으로 쌓는 두께(cm). 카메라가 벽에 붙어도 바깥 허공이 안 보일 만큼 */
	UPROPERTY(EditAnywhere, Category = "Nebula|Border", meta = (ClampMin = "0.0"))
	float BorderThickness = 3200.0f;

	/** 판끼리 높이를 조금씩 달리해 겹친 판이 같은 깊이에서 깜빡이지 않게 한다(cm) */
	UPROPERTY(EditAnywhere, Category = "Nebula", meta = (ClampMin = "0.0"))
	float LayerHeightStep = 0.5f;
};
