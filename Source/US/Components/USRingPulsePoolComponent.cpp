#include "Components/USRingPulsePoolComponent.h"
#include "US.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** 엔진 기본 Plane 메시의 한 변 길이 (uu) */
	constexpr float RingPlaneMeshSize = 100.0f;
}

UUSRingPulsePoolComponent::UUSRingPulsePoolComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneFinder.Succeeded())
	{
		PlaneMesh = PlaneFinder.Object;
	}
}

void UUSRingPulsePoolComponent::PlayPulse(const FVector& Location, float Radius)
{
	UWorld* World = GetWorld();
	if (World == nullptr || Radius <= 0.0f)
	{
		return;
	}

	if (RingMaterial == nullptr || PlaneMesh == nullptr)
	{
		if (!bWarnedMissingMaterial)
		{
			bWarnedMissingMaterial = true;
			UE_LOG(LogUS, Warning, TEXT("%s: RingPulsePool has no ring material or plane mesh; nothing is drawn"), *GetNameSafe(GetOwner()));
		}
		return;
	}

	if (Rings.IsEmpty())
	{
		CreateRings();
	}

	UStaticMeshComponent* Ring = Rings[NextRingIndex];
	NextRingIndex = (NextRingIndex + 1) % Rings.Num();

	// 판 지름 = 효과 지름. 절대 위치·스케일이라 무기(부모)가 움직이거나 커도 영향이 없다
	const float Scale = Radius * 2.0f / RingPlaneMeshSize;
	Ring->SetWorldLocationAndRotation(Location, FRotator::ZeroRotator);
	Ring->SetWorldScale3D(FVector(Scale, Scale, 1.0f));

	// 머티리얼 Time 노드와 같은 기준인 월드 시간을 적으면 머티리얼이 고리를 퍼뜨리고 0.6초 뒤 스스로 사라진다
	Ring->SetCustomPrimitiveDataFloat(PulseTimeDataIndex, World->GetTimeSeconds());
}

void UUSRingPulsePoolComponent::CreateRings()
{
	AActor* Owner = GetOwner();
	if (Owner == nullptr)
	{
		return;
	}

	Rings.Reserve(PoolSize);
	for (int32 Index = 0; Index < PoolSize; ++Index)
	{
		UStaticMeshComponent* Ring = NewObject<UStaticMeshComponent>(Owner);
		Ring->SetStaticMesh(PlaneMesh);
		Ring->SetMaterial(0, RingMaterial);

		// 표시 전용: 충돌·그림자·내비 영향 없음 (EMP 고리와 같다)
		Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Ring->SetCanEverAffectNavigation(false);
		Ring->SetGenerateOverlapEvents(false);
		Ring->CastShadow = false;

		// 부모(무기)를 따라가지 않고 터진 자리에 남는다
		Ring->SetUsingAbsoluteLocation(true);
		Ring->SetUsingAbsoluteRotation(true);
		Ring->SetUsingAbsoluteScale(true);
		Ring->SetupAttachment(this);
		Ring->RegisterComponent();

		// 아직 터진 적 없음 = 머티리얼 기본값(-100)과 같게 둬서 처음엔 안 보이게
		Ring->SetCustomPrimitiveDataFloat(PulseTimeDataIndex, -100.0f);

		Rings.Add(Ring);
	}
}
