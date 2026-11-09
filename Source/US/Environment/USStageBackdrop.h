#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "USStageBackdrop.generated.h"

class UMaterialInterface;

/**
 * 스테이지 배경: 판 시작에 출격한 스테이지의 배경 재질(UUSStageData::GetBackdropMaterial)을 먼 별판에 씌우고,
 * 구역 행성(맵의 3D 구 액터)을 스테이지의 위치·반지름으로 옮긴다. 행성은 진짜 구라 원근·조명이 맞는다 (그림 행성은 판 위에서 찌그러져 버림).
 * 전투 맵은 하나라 지역마다 먼 하늘 그림(성운·행성·고리 …, Tools/gen_stage_backdrop.py)만 바꿔 "다른 장소"를 보여 준다
 * (사용자 결정 2026-10-28: 3D 소품 장식은 모델 품질·화풍 문제로 버리고 그림만).
 * 격납고를 거치지 않고 전투 맵을 바로 열면(PIE) 스테이지가 없으므로 PreviewMaterial을 쓴다. 한 번 바꾸면 Tick·타이머 없음.
 */
UCLASS()
class US_API AUSStageBackdrop : public AActor
{
	GENERATED_BODY()

public:
	AUSStageBackdrop();

	virtual void BeginPlay() override;

protected:
	/** 그림을 씌울 별판 액터 (맵의 StaticMeshActor, 첫 재질 슬롯) */
	UPROPERTY(EditInstanceOnly, Category = "Backdrop")
	TObjectPtr<AActor> BackdropActor;

	/** 스테이지 없이 열었을 때 쓸 재질 (비우면 별판 그대로) */
	UPROPERTY(EditAnywhere, Category = "Backdrop")
	TObjectPtr<UMaterialInterface> PreviewMaterial;

	/** 구역 행성 (맵의 구 메시 StaticMeshActor). 비우면 행성 없이 그림만 */
	UPROPERTY(EditInstanceOnly, Category = "Backdrop|Planet")
	TObjectPtr<AActor> PlanetActor;

	/** 행성 메시의 배율 1일 때 반지름 (엔진 기본 구 = 50) */
	UPROPERTY(EditAnywhere, Category = "Backdrop|Planet", meta = (ClampMin = "1.0"))
	float PlanetMeshRadius = 50.0f;

	/** 스테이지 없이 열었을 때 행성 위치·반지름 */
	UPROPERTY(EditAnywhere, Category = "Backdrop|Planet")
	FVector PreviewPlanetLocation = FVector(0.0f, 0.0f, -12000.0f);

	UPROPERTY(EditAnywhere, Category = "Backdrop|Planet", meta = (ClampMin = "0.0"))
	float PreviewPlanetRadius = 0.0f;

private:
	/** 행성을 Location으로 옮기고 반지름 Radius에 맞춰 키운다. 0 이하면 숨긴다 */
	void PlacePlanet(const FVector& Location, float Radius) const;
};
