#include "Hangar/USHangarModelStage.h"
#include "US.h"
#include "Agents/USDroneCharacter.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

AUSHangarModelStage::AUSHangarModelStage()
{
	PrimaryActorTick.bCanEverTick = true;

	StageRoot = CreateDefaultSubobject<USceneComponent>(TEXT("StageRoot"));
	SetRootComponent(StageRoot);

	Turntable = CreateDefaultSubobject<USceneComponent>(TEXT("Turntable"));
	Turntable->SetupAttachment(StageRoot);

	// 무대는 캡처에만 보인다: 맵 밖이라도 메인 화면·충돌·그림자에 끼지 않게 막는다
	CraftMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CraftMesh"));
	CraftMesh->SetupAttachment(Turntable);
	CraftMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CraftMesh->SetCastShadow(false);
	CraftMesh->bVisibleInSceneCaptureOnly = true;
	// 캡처에만 보이는 물체는 텍스처 스트리밍이 "안 보임"으로 봐서 낮은 밉만 올린다 → 기체 한 대라 밉을 전부 붙잡아 둔다
	CraftMesh->bForceMipStreaming = true;

	// 기체 하나만 찍고(ShowOnly) 배경은 비운다. SceneColorHDR은 알파에 "덮이지 않은 정도"를 담아 UI 재질이 배경을 투명하게 뺄 수 있다
	CaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CaptureComponent"));
	CaptureComponent->SetupAttachment(StageRoot);
	CaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	CaptureComponent->ShowOnlyComponents.Add(CraftMesh);
	CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	CaptureComponent->bCaptureEveryFrame = true;
	CaptureComponent->bCaptureOnMovement = false;
	CaptureComponent->FOVAngle = 30.0f;
	CaptureComponent->ShowFlags.SetAtmosphere(false);
	CaptureComponent->ShowFlags.SetFog(false);
	CaptureComponent->ShowFlags.SetVolumetricFog(false);

	KeyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("KeyLight"));
	KeyLight->SetupAttachment(StageRoot);
	KeyLight->SetCastShadows(false);

	RimLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("RimLight"));
	RimLight->SetupAttachment(StageRoot);
	RimLight->SetCastShadows(false);
}

void AUSHangarModelStage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (RotationPeriod > 0.0f)
	{
		Turntable->AddLocalRotation(FRotator(0.0f, 360.0f / RotationPeriod * DeltaSeconds, 0.0f));
	}
}

void AUSHangarModelStage::ShowCraft(TSubclassOf<AUSDroneCharacter> CraftClass, bool bLocked)
{
	const AUSDroneCharacter* CraftDefaults = CraftClass ? CraftClass->GetDefaultObject<AUSDroneCharacter>() : nullptr;
	const UStaticMeshComponent* SourceMesh = CraftDefaults ? CraftDefaults->GetVisualMesh() : nullptr;
	if (SourceMesh == nullptr || SourceMesh->GetStaticMesh() == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: %s has no visual mesh; hangar stage is empty"), *GetName(), *GetNameSafe(CraftClass));
		CraftMesh->SetStaticMesh(nullptr);
		return;
	}

	// 판 안의 모습과 같게: BP에서 보정한 회전·크기와 슬롯 재질을 그대로 옮긴다 (위치는 FrameCraft가 다시 잡는다)
	CraftMesh->SetStaticMesh(SourceMesh->GetStaticMesh());
	CraftMesh->SetRelativeRotation(SourceMesh->GetRelativeRotation());
	CraftMesh->SetRelativeScale3D(SourceMesh->GetRelativeScale3D());
	CraftMesh->EmptyOverrideMaterials();

	const int32 NumMaterials = CraftMesh->GetNumMaterials();
	if (bLocked && LockedMaterial)
	{
		for (int32 Slot = 0; Slot < NumMaterials; ++Slot)
		{
			CraftMesh->SetMaterial(Slot, LockedMaterial);
		}
	}
	else
	{
		for (int32 Slot = 0; Slot < NumMaterials; ++Slot)
		{
			CraftMesh->SetMaterial(Slot, SourceMesh->GetMaterial(Slot));
		}
		for (const TPair<FName, TObjectPtr<UMaterialInterface>>& Pair : CraftDefaults->GetSlotMaterials())
		{
			const int32 Slot = CraftMesh->GetMaterialIndex(Pair.Key);
			if (Pair.Value && Slot != INDEX_NONE)
			{
				CraftMesh->SetMaterial(Slot, Pair.Value);
			}
		}
	}

	FrameCraft();
}

void AUSHangarModelStage::FrameCraft()
{
	// 새 기체는 정면부터 돌기 시작한다. 회전을 먼저 되돌려야 경계 구 가운데를 맞출 때 기울어진 값이 섞이지 않는다
	Turntable->SetRelativeRotation(FRotator::ZeroRotator);
	CraftMesh->SetRelativeLocation(FVector::ZeroVector);
	CraftMesh->UpdateBounds();

	const FBoxSphereBounds Bounds = CraftMesh->Bounds;
	CraftMesh->AddWorldOffset(Turntable->GetComponentLocation() - Bounds.Origin);

	// 돌아가는 동안 화면 폭에서 차지하는 최대 크기 = 경계 구 지름. 지름 / (2 · D · tan(FOV/2)) = CraftWidthFraction 을 D로 푼다
	const float Radius = FMath::Max(Bounds.SphereRadius, 1.0f);
	const float HalfFovTan = FMath::Tan(FMath::DegreesToRadians(CaptureComponent->FOVAngle * 0.5f));
	const float Distance = Radius / (CraftWidthFraction * HalfFovTan);

	const FRotator ViewRotation(-CameraPitch, 180.0f, 0.0f);
	const FVector Pivot = Turntable->GetRelativeLocation();
	CaptureComponent->SetRelativeLocationAndRotation(Pivot - ViewRotation.Vector() * Distance, ViewRotation);

	// 조명도 기체 크기에 맞춰 놓는다: 앞 위쪽 주 조명, 뒤쪽 윤곽 조명
	KeyLight->SetRelativeLocation(Pivot + FVector(Radius * 3.0f, -Radius * 2.0f, Radius * 3.0f));
	RimLight->SetRelativeLocation(Pivot + FVector(-Radius * 3.0f, Radius * 2.0f, Radius * 1.5f));
	KeyLight->SetAttenuationRadius(Radius * 12.0f);
	RimLight->SetAttenuationRadius(Radius * 12.0f);
}
