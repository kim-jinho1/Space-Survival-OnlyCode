#include "Components/USTargetRingComponent.h"
#include "US.h"
#include "Components/USAimComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"

UUSTargetRingComponent::UUSTargetRingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 표시 전용: 충돌·그림자·내비 영향 없음
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCanEverAffectNavigation(false);
	CastShadow = false;
	SetGenerateOverlapEvents(false);

	// 표적의 스케일(적 종류 배율)을 물려받지 않게 크기는 월드 기준으로 직접 정한다
	SetUsingAbsoluteScale(true);

	// 표적이 생기기 전에는 숨겨 둔다
	SetHiddenInGame(true);
}

void UUSTargetRingComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetStaticMesh() == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: TargetRing has no mesh; the target ring is invisible"), *GetNameSafe(GetOwner()));
	}
	AutoMaterial = GetMaterial(0);

	UUSAimComponent* Aim = GetOwner() ? GetOwner()->FindComponentByClass<UUSAimComponent>() : nullptr;
	if (Aim == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: TargetRing found no aim component; the ring will never show"), *GetNameSafe(GetOwner()));
		return;
	}

	AimTargetChangedHandle = Aim->OnAimTargetChanged.AddUObject(this, &UUSTargetRingComponent::HandleAimTargetChanged);
	BoundAim = Aim;
}

void UUSTargetRingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UUSAimComponent* Aim = BoundAim.Get())
	{
		Aim->OnAimTargetChanged.Remove(AimTargetChangedHandle);
	}
	BoundAim = nullptr;
	AimTargetChangedHandle.Reset();

	Super::EndPlay(EndPlayReason);
}

void UUSTargetRingComponent::HandleAimTargetChanged(AActor* NewTarget, bool bManual)
{
	if (NewTarget == nullptr || (!bManual && !bShowOnAutoTarget))
	{
		HideRing();
		return;
	}

	ShowOn(NewTarget, bManual);
}

void UUSTargetRingComponent::ShowOn(AActor* Target, bool bManual)
{
	USceneComponent* TargetRoot = Target ? Target->GetRootComponent() : nullptr;
	if (TargetRoot == nullptr)
	{
		HideRing();
		return;
	}

	// 표적 루트(캡슐)에 붙이면 표적이 움직일 때 엔진이 같이 옮겨 준다. 높이는 캡슐 바닥, 크기는 캡슐 반지름 기준
	AttachToComponent(TargetRoot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);

	float Radius = 0.0f;
	float HalfHeight = 0.0f;
	Target->GetSimpleCollisionCylinder(Radius, HalfHeight);
	SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -HalfHeight + GroundOffset), FRotator::ZeroRotator);

	// 기체 메시가 캡슐보다 크면(적 종류 배율 포함) 메시 수평 반폭을 쓴다. 체력바 위젯·불꽃 같은 비메시 컴포넌트는 보지 않는다
	Target->ForEachComponent<UStaticMeshComponent>(false, [&Radius](const UStaticMeshComponent* Component)
	{
		if (Component->IsVisible() && Component->GetStaticMesh() != nullptr)
		{
			const FVector Extent = Component->Bounds.BoxExtent;
			Radius = FMath::Max(Radius, FMath::Max(Extent.X, Extent.Y));
		}
	});

	// 판 메시 크기(엔진 Plane은 100)를 읽어 링 지름을 맞춘다. 메시를 바꿔도 크기 계산이 그대로 맞는다
	const UStaticMesh* Mesh = GetStaticMesh();
	const float MeshHalfSize = Mesh ? FMath::Max(Mesh->GetBounds().BoxExtent.X, 1.0f) : 50.0f;
	const float Scale = RadiusScale * (bManual ? ManualRadiusMultiplier : 1.0f);
	const float PlaneScale = FMath::Max(Radius, 1.0f) * Scale / MeshHalfSize;
	SetWorldScale3D(FVector(PlaneScale, PlaneScale, 1.0f));

	UMaterialInterface* WantedMaterial = bManual && ManualMaterial ? ManualMaterial.Get() : AutoMaterial.Get();
	if (WantedMaterial && GetMaterial(0) != WantedMaterial)
	{
		SetMaterial(0, WantedMaterial);
	}

	SetHiddenInGame(false);
}

void UUSTargetRingComponent::HideRing()
{
	SetHiddenInGame(true);

	// 남의 액터에 붙은 채로 두면 그 적이 제거될 때 같이 끌려간다 → 주인에게 되돌린다
	AActor* Owner = GetOwner();
	USceneComponent* OwnerRoot = Owner ? Owner->GetRootComponent() : nullptr;
	if (OwnerRoot && GetAttachParent() != OwnerRoot)
	{
		AttachToComponent(OwnerRoot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
}
