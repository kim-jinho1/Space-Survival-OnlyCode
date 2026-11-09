#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * "지금 화면에 보이는가" 판정. 자동 조준·아군·미사일이 화면 밖 적을 먼저 쏘지 않게 한다 (공격이 항상 보이게).
 * 만들 때 화면(첫 로컬 플레이어 뷰포트)의 투영 행렬을 한 번 계산해 두고 Contains로 여러 위치를 싸게 검사한다.
 * 화면이 없으면(뷰포트 없음) 모든 위치를 화면 안으로 본다 — 판정 때문에 공격이 막히지 않게.
 */
struct FUSScreenBounds
{
	/** EdgeMarginRatio: 화면 가장자리에서 안쪽으로 이 비율만큼은 밖으로 본다 (0.03 = 폭·높이의 3%) */
	explicit FUSScreenBounds(const UWorld* World, float EdgeMarginRatio = 0.03f);

	/** Location이 화면 안(가장자리 여백 제외)에 보이는가 */
	bool Contains(const FVector& Location) const;

private:
	FMatrix ViewProjectionMatrix = FMatrix::Identity;
	FIntRect ViewRect;
	FIntRect InnerRect;
	bool bValid = false;
};

/**
 * 위치 찾기 공용 함수. 적 스포너와 보급선처럼 "대장 주변 링 위의 바닥 지점"이 필요한 곳이 같은 규칙을 쓴다.
 */
namespace USLocationUtils
{
	/** 링 후보를 다시 뽑는 최대 횟수. 대장이 벽에 붙어 있어도 링의 절반 이상은 안쪽이라 이 정도면 거의 항상 찾는다 */
	constexpr int32 MaxRingAttempts = 8;

	/**
	 * Center 주변 링(RadiusMin~RadiusMax) 위의 무작위 지점 중 내비메시나 바닥 위에 있는 곳을 찾는다.
	 * 바닥이 곧 경기장이므로 바닥에 닿지 않는 후보(벽 밖)는 버리고 MaxRingAttempts번까지 다시 뽑는다.
	 * 찾으면 바닥에서 HeightOffset만큼 띄운 위치를 OutLocation에 넣고 true. 모두 실패하면 false (허공에 두지 않는다)
	 */
	bool TryFindFloorLocationInRing(const UWorld* World, const FVector& Center, float RadiusMin, float RadiusMax,
		float HeightOffset, FVector& OutLocation);

	/**
	 * TryFindFloorLocationInRing과 같지만 방향을 Angle(라디안) ± AngleJitter 안에서 먼저 찾는다.
	 * 그 방향이 벽 밖이면(대장이 가장자리에 있을 때) 링 전체에서 찾는다
	 */
	bool TryFindFloorLocationNearAngle(const UWorld* World, const FVector& Center, float Angle, float AngleJitter,
		float RadiusMin, float RadiusMax, float HeightOffset, FVector& OutLocation);

	/**
	 * 정해진 지점 Candidate(XY) 아래의 바닥을 찾는다 (내비메시 → 지면 트레이스). 무리 스폰처럼 모양을 정해 놓고 놓을 때 쓴다.
	 * ReferenceZ는 트레이스 기준 높이(보통 대장 위치의 Z). 바닥이 없으면(벽 밖) false
	 */
	bool TryFindFloorLocationAt(const UWorld* World, const FVector& Candidate, float ReferenceZ, float HeightOffset,
		FVector& OutLocation);

	/**
	 * From에서 To(XY)까지 From 높이의 수평선이 벽(WorldStatic)에 막히지 않는가.
	 * 링 함수들은 이미 이 검사를 한다. 바닥이 벽 밖까지 깔린 맵에서 벽 너머 자리를 거르는 데 쓴다
	 */
	bool HasClearPathFlat(const UWorld* World, const FVector& From, const FVector& To);
}
