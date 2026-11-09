#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "USSpaceDust.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;

/**
 * 기체 바로 아래 깊이에 흩뿌린 작은 먼지 점 (배경 장식, 충돌 없음). 속도감용.
 * 먼 별판(z -15000)은 화면에서 아주 느리게 흐르므로, 가까운 깊이(z -300 ~ -2500)에 점을 깔아 "지나간다"는 느낌을 더한다.
 * 점은 월드에 고정된 진짜 물체라 원근 시차가 저절로 생기고, 화면 위치가 바뀌어도 속도 버퍼가 맞아 TSR 번짐이 없다.
 * 맵 전체에 깔면 수가 많아지므로 대장 주변 TileSize 정사각형에만 두고, 짧은 타이머로 칸 밖으로 벗어난 점을
 * 반대편으로 TileSize만큼 옮긴다(화면 밖에서 일어나 안 보인다). 매 프레임 Tick 대신 WrapInterval 타이머.
 */
UCLASS()
class US_API AUSSpaceDust : public AActor
{
	GENERATED_BODY()

public:
	AUSSpaceDust();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * 워프 연출: 점을 Direction(바닥 평면) 쪽으로 Stretch배 길게 늘인다 (1 = 원래 점). 별이 선으로 늘어나는 "광속" 느낌.
	 * 점 판(Plane)을 방향 쪽으로 돌려 한 축만 키우므로 둥근 점 재질이 그대로 가는 선이 된다
	 */
	void SetWarpStretch(float Stretch, const FVector& Direction);

protected:
	/** Center 둘레 TileSize 칸에 점을 Seed로 다시 만든다 */
	void BuildDust(const FVector& Center);

	/** 대장 위치 기준으로 칸 밖에 나간 점을 반대편으로 옮긴다 (WrapInterval 타이머) */
	void WrapDust();

	/** Index 점의 지금 변환 (위치·크기 + 워프 늘임) */
	FTransform MakeSpeckTransform(int32 Index) const;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInstancedStaticMeshComponent> Specks;

	/** 점 재질 (가산 발광 원, M_USSpaceDust) */
	UPROPERTY(EditAnywhere, Category = "Dust")
	TObjectPtr<UMaterialInterface> DustMaterial;

	/** 점 개수 */
	UPROPERTY(EditAnywhere, Category = "Dust", meta = (ClampMin = "1", ClampMax = "2000"))
	int32 DustCount = 300;

	/** 점을 두는 정사각형 한 변(uu). 카메라에 보이는 범위보다 넉넉히 커야 옮기는 순간이 안 보인다 */
	UPROPERTY(EditAnywhere, Category = "Dust", meta = (ClampMin = "1000.0"))
	float TileSize = 12000.0f;

	/** 점 높이(월드 Z) 범위. X = 가장 깊은 곳, Y = 가장 얕은 곳. 깊을수록 화면에서 느리게 흐른다 */
	UPROPERTY(EditAnywhere, Category = "Dust")
	FVector2D DepthRange = FVector2D(-2500.0f, -300.0f);

	/** 점 지름(uu) 범위 */
	UPROPERTY(EditAnywhere, Category = "Dust")
	FVector2D SpeckSize = FVector2D(8.0f, 20.0f);

	/** 칸 밖 점을 옮기는 간격(초). 달리기 1800uu/s × 이 값이 칸 반 크기보다 한참 작아야 한다 */
	UPROPERTY(EditAnywhere, Category = "Dust", meta = (ClampMin = "0.05"))
	float WrapInterval = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Dust")
	int32 Seed = 7;

private:
	/** 점 위치 (ISM에서 매번 읽지 않도록 따로 들고 있는다) */
	TArray<FVector> SpeckLocations;

	/** 점 크기 (옮겨도 그대로) */
	TArray<float> SpeckScales;

	/** 워프 늘임 배율과 방향 (yaw) */
	float WarpStretch = 1.0f;
	float WarpYaw = 0.0f;

	FTimerHandle WrapTimerHandle;
};
