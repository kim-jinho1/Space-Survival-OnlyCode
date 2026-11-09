#include "Environment/USNebulaCloud.h"
#include "US.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** 엔진 Plane은 100cm 정사각형 */
	constexpr float PlaneSize = 100.0f;
}

AUSNebulaCloud::AUSNebulaCloud()
{
	PrimaryActorTick.bCanEverTick = false;

	Puffs = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Puffs"));
	SetRootComponent(Puffs);
	Puffs->SetMobility(EComponentMobility::Static);
	Puffs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Puffs->SetCanEverAffectNavigation(false);
	Puffs->CastShadow = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneFinder.Succeeded())
	{
		Puffs->SetStaticMesh(PlaneFinder.Object);
	}
}

void AUSNebulaCloud::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RebuildPuffs();
}

void AUSNebulaCloud::RebuildPuffs()
{
	Puffs->ClearInstances();

	if (PuffMaterial == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("AUSNebulaCloud: %s has no PuffMaterial; no puffs"), *GetName());
		return;
	}
	Puffs->SetMaterial(0, PuffMaterial);

	FRandomStream Random(Seed);
	const float MinSize = FMath::Min(PuffSize.X, PuffSize.Y);
	const float MaxSize = FMath::Max(PuffSize.X, PuffSize.Y);

	TArray<FTransform> Instances;
	Instances.Reserve(PuffCount);
	for (int32 Index = 0; Index < PuffCount; ++Index)
	{
		FVector2D Local = FVector2D::ZeroVector;
		// 0 = 가장자리, 1 = 중심. 중심 쪽 판을 크게 해 덩어리가 가운데로 볼록해 보이게 한다
		float Core = 0.5f;

		if (Shape == EUSNebulaShape::Blob)
		{
			// 반경의 제곱근 분포 대신 그대로 써서 가운데가 더 촘촘하게 한다
			const float Distance = Radius * Random.FRand();
			const float Angle = Random.FRandRange(0.0f, 2.0f * PI);
			Local = FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Distance;
			Core = Radius > 0.0f ? 1.0f - Distance / Radius : 1.0f;
		}
		else
		{
			// 둘레 길이에 비례해 변을 고른 뒤, 그 변을 따라 위치·깊이를 정한다
			const float HalfX = BorderHalfExtent.X;
			const float HalfY = BorderHalfExtent.Y;
			const float Depth = Random.FRandRange(-BorderInset, BorderThickness);
			const bool bAlongX = Random.FRand() < HalfX / FMath::Max(HalfX + HalfY, 1.0f);
			const float Sign = Random.FRand() < 0.5f ? -1.0f : 1.0f;
			if (bAlongX)
			{
				Local = FVector2D(Random.FRandRange(-HalfX - BorderThickness, HalfX + BorderThickness), Sign * (HalfY + Depth));
			}
			else
			{
				Local = FVector2D(Sign * (HalfX + Depth), Random.FRandRange(-HalfY - BorderThickness, HalfY + BorderThickness));
			}
			// 안쪽 끝은 작은 판으로 들쭉날쭉하게, 바깥은 큰 판으로 빈틈없이
			const float Range = FMath::Max(BorderInset + BorderThickness, 1.0f);
			Core = (Depth + BorderInset) / Range;
		}

		const float Size = FMath::Lerp(MinSize, MaxSize, FMath::Clamp(0.35f * Core + 0.65f * Random.FRand(), 0.0f, 1.0f));
		const FRotator Rotation(0.0f, Random.FRandRange(0.0f, 360.0f), 0.0f);
		const FVector Location(Local.X, Local.Y, Index * LayerHeightStep);
		Instances.Add(FTransform(Rotation, Location, FVector(Size / PlaneSize, Size / PlaneSize, 1.0f)));
	}

	Puffs->AddInstances(Instances, false, false, false);
}
