#include "Environment/USSpaceDust.h"
#include "US.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** 엔진 Plane은 100cm 정사각형 */
	constexpr float DustPlaneSize = 100.0f;
}

AUSSpaceDust::AUSSpaceDust()
{
	PrimaryActorTick.bCanEverTick = false;

	Specks = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Specks"));
	SetRootComponent(Specks);
	// 점을 옮기므로 Movable
	Specks->SetMobility(EComponentMobility::Movable);
	Specks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Specks->SetCanEverAffectNavigation(false);
	Specks->CastShadow = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneFinder.Succeeded())
	{
		Specks->SetStaticMesh(PlaneFinder.Object);
	}
}

void AUSSpaceDust::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 에디터 미리보기: 액터 자리 둘레에 깐다. 게임에서는 BeginPlay가 대장 둘레로 다시 깐다
	BuildDust(GetActorLocation());
}

void AUSSpaceDust::BeginPlay()
{
	Super::BeginPlay();

	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	BuildDust(Player ? Player->GetActorLocation() : GetActorLocation());

	GetWorldTimerManager().SetTimer(WrapTimerHandle, this, &AUSSpaceDust::WrapDust, WrapInterval, true);
}

void AUSSpaceDust::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(WrapTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AUSSpaceDust::BuildDust(const FVector& Center)
{
	Specks->ClearInstances();
	SpeckLocations.Reset();
	SpeckScales.Reset();

	if (DustMaterial == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("AUSSpaceDust: %s has no DustMaterial; no dust"), *GetName());
		return;
	}
	Specks->SetMaterial(0, DustMaterial);

	FRandomStream Random(Seed);
	const float HalfTile = TileSize * 0.5f;
	const float MinSize = FMath::Min(SpeckSize.X, SpeckSize.Y);
	const float MaxSize = FMath::Max(SpeckSize.X, SpeckSize.Y);
	const float MinZ = FMath::Min(DepthRange.X, DepthRange.Y);
	const float MaxZ = FMath::Max(DepthRange.X, DepthRange.Y);

	TArray<FTransform> Instances;
	Instances.Reserve(DustCount);
	SpeckLocations.Reserve(DustCount);
	SpeckScales.Reserve(DustCount);
	for (int32 Index = 0; Index < DustCount; ++Index)
	{
		const FVector Location(
			Center.X + Random.FRandRange(-HalfTile, HalfTile),
			Center.Y + Random.FRandRange(-HalfTile, HalfTile),
			Random.FRandRange(MinZ, MaxZ));
		const float Scale = Random.FRandRange(MinSize, MaxSize) / DustPlaneSize;

		SpeckLocations.Add(Location);
		SpeckScales.Add(Scale);
		Instances.Add(FTransform(FRotator::ZeroRotator, Location, FVector(Scale, Scale, 1.0f)));
	}

	// 인스턴스 좌표는 월드 기준 (액터를 어디 두든 상관없게)
	Specks->AddInstances(Instances, false, true);
}

void AUSSpaceDust::WrapDust()
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Player == nullptr || SpeckLocations.IsEmpty())
	{
		return;
	}

	const FVector Center = Player->GetActorLocation();
	const float HalfTile = TileSize * 0.5f;
	bool bAnyMoved = false;

	for (int32 Index = 0; Index < SpeckLocations.Num(); ++Index)
	{
		FVector& Location = SpeckLocations[Index];
		bool bMoved = false;

		// 칸 반 크기를 넘으면 반대편으로 한 칸 옮긴다 — 점 사이 간격·분포는 그대로 유지된다
		if (Location.X - Center.X > HalfTile) { Location.X -= TileSize; bMoved = true; }
		else if (Center.X - Location.X > HalfTile) { Location.X += TileSize; bMoved = true; }
		if (Location.Y - Center.Y > HalfTile) { Location.Y -= TileSize; bMoved = true; }
		else if (Center.Y - Location.Y > HalfTile) { Location.Y += TileSize; bMoved = true; }

		if (bMoved)
		{
			// 렌더 상태는 마지막에 한 번만 갱신한다. 순간이동(bTeleport)이라 이전 위치와의 속도로 번지지 않는다
			Specks->UpdateInstanceTransform(Index, MakeSpeckTransform(Index), true, false, true);
			bAnyMoved = true;
		}
	}

	// 워프로 대장이 멀리 날면 점이 한꺼번에 옮겨지므로 전체 재생성 대신 인스턴스만 갱신한다 (끊김 방지)
	if (bAnyMoved)
	{
		Specks->MarkRenderInstancesDirty();
	}
}

FTransform AUSSpaceDust::MakeSpeckTransform(int32 Index) const
{
	const float Scale = SpeckScales[Index];
	return FTransform(FRotator(0.0f, WarpYaw, 0.0f), SpeckLocations[Index], FVector(Scale * WarpStretch, Scale, 1.0f));
}

void AUSSpaceDust::SetWarpStretch(float Stretch, const FVector& Direction)
{
	WarpStretch = FMath::Max(1.0f, Stretch);
	WarpYaw = Direction.IsNearlyZero() ? 0.0f : Direction.Rotation().Yaw;
	if (SpeckLocations.IsEmpty())
	{
		return;
	}

	// 연출 동안 프레임마다 불리므로 한 번에 넣는다.
	// 렌더 상태 전체 재생성(bMarkRenderStateDirty)은 프레임마다 하면 끊긴다 → 인스턴스 변환만 GPU에 보내는 부분 갱신(MarkRenderInstancesDirty)
	TArray<FTransform> Transforms;
	Transforms.Reserve(SpeckLocations.Num());
	for (int32 Index = 0; Index < SpeckLocations.Num(); ++Index)
	{
		Transforms.Add(MakeSpeckTransform(Index));
	}
	Specks->BatchUpdateInstancesTransforms(0, Transforms, true, false, true);
	Specks->MarkRenderInstancesDirty();
}
