#include "Core/USLocationUtils.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "NavigationSystem.h"
#include "SceneView.h"

FUSScreenBounds::FUSScreenBounds(const UWorld* World, float EdgeMarginRatio)
{
	const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	if (LocalPlayer == nullptr || LocalPlayer->ViewportClient == nullptr || LocalPlayer->ViewportClient->Viewport == nullptr)
	{
		return;
	}

	// APlayerController::ProjectWorldLocationToScreen은 부를 때마다 투영 행렬을 새로 만든다.
	// 적 수십 마리를 검사하므로 행렬은 한 번만 만들고 FSceneView::ProjectWorldToScreen으로 점만 투영한다
	FSceneViewProjectionData ProjectionData;
	if (!LocalPlayer->GetProjectionData(LocalPlayer->ViewportClient->Viewport, ProjectionData))
	{
		return;
	}

	ViewProjectionMatrix = ProjectionData.ComputeViewProjectionMatrix();
	ViewRect = ProjectionData.GetConstrainedViewRect();

	const int32 MarginX = FMath::RoundToInt(ViewRect.Width() * EdgeMarginRatio);
	const int32 MarginY = FMath::RoundToInt(ViewRect.Height() * EdgeMarginRatio);
	InnerRect = FIntRect(ViewRect.Min.X + MarginX, ViewRect.Min.Y + MarginY, ViewRect.Max.X - MarginX, ViewRect.Max.Y - MarginY);
	bValid = InnerRect.Width() > 0 && InnerRect.Height() > 0;
}

bool FUSScreenBounds::Contains(const FVector& Location) const
{
	if (!bValid)
	{
		return true;
	}

	FVector2D ScreenPosition;
	if (!FSceneView::ProjectWorldToScreen(Location, ViewRect, ViewProjectionMatrix, ScreenPosition))
	{
		// 카메라 뒤
		return false;
	}

	return ScreenPosition.X >= InnerRect.Min.X && ScreenPosition.X <= InnerRect.Max.X
		&& ScreenPosition.Y >= InnerRect.Min.Y && ScreenPosition.Y <= InnerRect.Max.Y;
}

namespace
{
	/** 후보 지점 하나를 바닥(내비메시 또는 지면)으로 내린다. 바닥이 없으면(벽 밖) false */
	bool TryProjectPointToFloor(const UWorld* World, const UNavigationSystemV1* NavSys, const FVector& Candidate,
		float ReferenceZ, float HeightOffset, FVector& OutLocation)
	{
		// 1순위: NavMesh 위로 투영 (네브메시 볼륨이 있을 때)
		if (NavSys)
		{
			FNavLocation NavLocation;
			if (NavSys->ProjectPointToNavigation(Candidate, NavLocation, FVector(300.0f, 300.0f, 600.0f)))
			{
				OutLocation = NavLocation.Location + FVector(0.0f, 0.0f, HeightOffset);
				return true;
			}
		}

		// 2순위: 지면으로 라인트레이스 (네브메시가 없을 때)
		// 중심 높이 바로 위에서 아래로 쏴 '바닥'을 찾는다.
		// (후보지점 한참 위에서 쏘면 주방 천장 윗면에 맞아 적이 천장에 스폰되는 버그가 있었음)
		FHitResult Hit;
		const FVector TraceStart = FVector(Candidate.X, Candidate.Y, ReferenceZ + 200.0f);
		const FVector TraceEnd = FVector(Candidate.X, Candidate.Y, ReferenceZ - 2000.0f);
		if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic))
		{
			OutLocation = Hit.ImpactPoint + FVector(0.0f, 0.0f, HeightOffset);
			return true;
		}

		return false;
	}

	/** 링 위 후보 지점 하나를 바닥으로 내린다. 중심에서 벽에 막힌 자리(벽 너머)는 버린다 */
	bool TryProjectToFloor(const UWorld* World, const UNavigationSystemV1* NavSys, const FVector& Center, float Angle,
		float Radius, float HeightOffset, FVector& OutLocation)
	{
		const FVector Candidate = Center + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
		return TryProjectPointToFloor(World, NavSys, Candidate, Center.Z, HeightOffset, OutLocation)
			&& USLocationUtils::HasClearPathFlat(World, Center, OutLocation);
	}
}

bool USLocationUtils::HasClearPathFlat(const UWorld* World, const FVector& From, const FVector& To)
{
	if (World == nullptr)
	{
		return false;
	}

	// 벽 밖에도 바닥·내비메시가 있는 맵이 있어 "바닥이 있다"만으로는 경기장 안인지 모른다 (맵 끝에서 벽 너머 스폰).
	// From 높이(대장 몸 높이)에서 수평으로 쏴 바닥은 스치지 않고 벽만 맞게 한다.
	// 채널 트레이스는 Pawn 캡슐도 맞으므로 오브젝트 타입 WorldStatic(벽·BlockingVolume)만 본다
	const FVector FlatTo(To.X, To.Y, From.Z);
	return !World->LineTraceTestByObjectType(From, FlatTo, FCollisionObjectQueryParams(ECC_WorldStatic));
}

bool USLocationUtils::TryFindFloorLocationInRing(const UWorld* World, const FVector& Center, float RadiusMin, float RadiusMax,
	float HeightOffset, FVector& OutLocation)
{
	if (World == nullptr)
	{
		return false;
	}

	const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);

	for (int32 Attempt = 0; Attempt < MaxRingAttempts; ++Attempt)
	{
		const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
		const float Radius = FMath::FRandRange(RadiusMin, RadiusMax);
		if (TryProjectToFloor(World, NavSys, Center, Angle, Radius, HeightOffset, OutLocation))
		{
			return true;
		}
	}

	return false;
}

bool USLocationUtils::TryFindFloorLocationNearAngle(const UWorld* World, const FVector& Center, float Angle, float AngleJitter,
	float RadiusMin, float RadiusMax, float HeightOffset, FVector& OutLocation)
{
	if (World == nullptr)
	{
		return false;
	}

	const UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);

	// 시도의 절반은 원하는 방향 부근에서
	constexpr int32 NearAngleAttempts = MaxRingAttempts / 2;
	for (int32 Attempt = 0; Attempt < NearAngleAttempts; ++Attempt)
	{
		const float CandidateAngle = Angle + FMath::FRandRange(-AngleJitter, AngleJitter);
		const float Radius = FMath::FRandRange(RadiusMin, RadiusMax);
		if (TryProjectToFloor(World, NavSys, Center, CandidateAngle, Radius, HeightOffset, OutLocation))
		{
			return true;
		}
	}

	// 그 방향이 벽 밖이면 링 전체에서
	return TryFindFloorLocationInRing(World, Center, RadiusMin, RadiusMax, HeightOffset, OutLocation);
}

bool USLocationUtils::TryFindFloorLocationAt(const UWorld* World, const FVector& Candidate, float ReferenceZ, float HeightOffset,
	FVector& OutLocation)
{
	if (World == nullptr)
	{
		return false;
	}

	return TryProjectPointToFloor(World, FNavigationSystem::GetCurrent<UNavigationSystemV1>(World), Candidate, ReferenceZ,
		HeightOffset, OutLocation);
}
