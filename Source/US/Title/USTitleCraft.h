#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "USTitleCraft.generated.h"

class AUSDroneCharacter;
class USceneComponent;
class UStaticMeshComponent;
class UUSHoverVisualComponent;
class UUSThrusterFXComponent;

/**
 * 타이틀 배경의 기체 한 대 (M-2 타이틀 B + 제자리 비행 A).
 * 화면 속 자리는 그대로 두고 "앞으로 날아가는 중"처럼 보이게 한다: 실제로는 제자리에서 천천히 출렁일 뿐이고,
 * 루트 속도(ComponentVelocity)에 순항 속도를 적어 두면 게임 드론과 같은 연출 컴포넌트가 그 속도를 읽어 앞으로 숙이고 불꽃을 키운다.
 * 바닥 격자는 재질(MI_USSpaceFloor_Title의 ScrollVelocity)이 같은 속도로 뒤로 흘린다 — 두 값이 같아야 미끄러져 보이지 않는다.
 * 모양·둥실·불꽃 설정은 따로 등록하지 않고 CraftClass(기체 BP)의 CDO에서 복사한다 → 판 안의 모습과 늘 같다 (격납고 무대와 같은 방식).
 * 구도를 지키려고 카메라·위치는 움직이지 않는다 (FTL 타이틀처럼 배는 제자리, 배경이 흐른다).
 */
UCLASS()
class US_API AUSTitleCraft : public AActor
{
	GENERATED_BODY()

public:
	AUSTitleCraft();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** CraftClass CDO의 메시·보정 트랜스폼·슬롯 재질을 VisualMesh에 옮긴다 (에디터에서도 보이게 OnConstruction에서 부른다) */
	void ApplyCraftLook();

	/** CraftClass CDO의 둥실·불꽃 컴포넌트를 본떠 만들고 VisualRoot에 연결한다 */
	void CreateEffectComponents();

	/** 모양·연출 설정을 가져올 기체 BP (BP_USPlayerCharacter, BP_USAllyDrone 등) */
	UPROPERTY(EditAnywhere, Category = "Title")
	TSubclassOf<AUSDroneCharacter> CraftClass;

	/** 앞으로 날아가는 속도(cm/s). 연출용 속도일 뿐 실제로 움직이지 않는다. 바닥 ScrollVelocity 크기와 맞춘다 */
	UPROPERTY(EditAnywhere, Category = "Title", meta = (ClampMin = "0.0"))
	float CruiseSpeed = 250.0f;

	/** 제자리 출렁임 폭(cm). X = 앞뒤, Y = 좌우 (액터 기준) */
	UPROPERTY(EditAnywhere, Category = "Title")
	FVector2D SwayAmplitude = FVector2D(15.0f, 30.0f);

	/** 출렁임 한 번 왕복 시간(초). X = 앞뒤, Y = 좌우. 두 주기가 달라 같은 모양이 반복돼 보이지 않는다 */
	UPROPERTY(EditAnywhere, Category = "Title", meta = (ClampMin = "0.1"))
	FVector2D SwayPeriod = FVector2D(11.0f, 8.0f);

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** 둥실·기울기가 움직이는 중심 (드론의 VisualRoot와 같은 역할) */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> VisualRoot;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

private:
	/** CraftClass CDO를 본떠 BeginPlay에서 만든 연출 컴포넌트 */
	UPROPERTY(Transient)
	TObjectPtr<UUSHoverVisualComponent> HoverVisual;

	UPROPERTY(Transient)
	TObjectPtr<UUSThrusterFXComponent> Thruster;

	/** 맵에 놓인 자리. 출렁임은 여기서 더한다 */
	FVector BaseLocation = FVector::ZeroVector;

	/** 출렁임 위상(라디안). 기체마다 랜덤 시작값이라 편대가 똑같이 움직이지 않는다 */
	FVector2D SwayPhase = FVector2D::ZeroVector;
};
