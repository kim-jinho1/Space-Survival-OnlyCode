#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "USHangarModelStage.generated.h"

class AUSDroneCharacter;
class UMaterialInterface;
class UPointLightComponent;
class USceneCaptureComponent2D;
class USceneComponent;
class UStaticMeshComponent;

/**
 * 격납고 설계도 화면 왼쪽의 3D 기체 무대 (M-2 격납고 B, 구도 A: 비스듬한 3/4 시점 + 천천히 회전).
 * 맵 밖 멀리 놓인 무대에서 기체 하나만 SceneCapture로 찍어 렌더 타겟에 그리고, 위젯(WBP_USHangarBlueprint)이 그 렌더 타겟을 이미지로 띄운다.
 * 기체 모양은 따로 등록하지 않고 기체 BP의 CDO(AUSDroneCharacter::VisualMesh·SlotMaterials)에서 읽는다 → 판 안의 모습과 늘 같다.
 * 구도는 렌더 타겟(고정 크기) 기준이라 창 비율과 상관없다. 예전 월드 카메라 방식(창 비율마다 구도가 달라짐)을 버린 이유가 이것이다 (TASK.md D10-UI2).
 * 카메라 거리는 기체 경계 구 반지름으로 정한다: 돌아가도 화면 폭의 CraftWidthFraction 안에 들어온다.
 * 렌더 타겟은 CaptureComponent의 TextureTarget에 에디터에서 넣는다.
 */
UCLASS()
class US_API AUSHangarModelStage : public AActor
{
	GENERATED_BODY()

public:
	AUSHangarModelStage();

	virtual void Tick(float DeltaSeconds) override;

	/** CraftClass 기체를 무대에 올린다. bLocked면 잠긴 기종 재질(LockedMaterial) 하나로 덮는다 */
	void ShowCraft(TSubclassOf<AUSDroneCharacter> CraftClass, bool bLocked);

protected:
	/** 기체 경계 구 가운데를 회전 중심에 맞추고, 반지름으로 카메라 거리를 정한다 */
	void FrameCraft();

	/** 한 바퀴 도는 시간(초). 0이면 돌지 않는다 */
	UPROPERTY(EditAnywhere, Category = "Stage", meta = (ClampMin = "0.0"))
	float RotationPeriod = 12.0f;

	/** 카메라가 내려다보는 각도(도) */
	UPROPERTY(EditAnywhere, Category = "Stage", meta = (ClampMin = "0.0", ClampMax = "89.0"))
	float CameraPitch = 35.0f;

	/** 기체(경계 구 지름)가 화면 폭에서 차지하는 비율 */
	UPROPERTY(EditAnywhere, Category = "Stage", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float CraftWidthFraction = 0.75f;

	/** 잠긴 기종을 덮는 재질 (반투명 홀로그램). 비우면 원래 재질 그대로 */
	UPROPERTY(EditAnywhere, Category = "Stage")
	TObjectPtr<UMaterialInterface> LockedMaterial;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> StageRoot;

	/** 회전 중심. 기체 경계 구 가운데가 여기 온다 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> Turntable;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CraftMesh;

	/** 기체만 찍는 카메라 (ShowOnlyComponents). TextureTarget에 렌더 타겟을 넣는다 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneCaptureComponent2D> CaptureComponent;

	/** 앞 위쪽 주 조명 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> KeyLight;

	/** 뒤쪽 윤곽 조명 */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPointLightComponent> RimLight;
};
