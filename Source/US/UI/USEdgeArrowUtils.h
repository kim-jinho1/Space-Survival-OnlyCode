#pragma once

#include "CoreMinimal.h"

class UUserWidget;

/** 화면 가장자리 화살표 한 개의 자리 (위젯 좌표, 화면 중앙 기준) */
struct FUSEdgeArrowPlacement
{
	/** 화면 중앙에서 대상 쪽 단위 방향 */
	FVector2D Direction = FVector2D::ZeroVector;

	/** 중앙에서 여백만큼 안쪽 사각형 테두리까지의 이동량. 중앙 앵커 위젯의 SetRenderTranslation에 그대로 넣는다 */
	FVector2D EdgeOffset = FVector2D::ZeroVector;

	/** Direction의 각도 (도, 오른쪽 0·시계 방향). 텍스처 보정은 호출자가 더한다 */
	float AngleDegrees = 0.0f;
};

/**
 * 화면 밖 대상 화살표 공용 계산 (보급선·보스 표시기, 무리 예고, 보스 등장 경고가 같은 규칙을 쓴다).
 * 세 위젯이 같은 계산을 따로 들고 있다가 무리 예고에만 "화면 안이면 숨김" 검사가 빠져 있었다 (빌드 테스트 2026-11-02).
 */
namespace USEdgeArrowUtils
{
	/**
	 * WorldLocation이 화면 밖이면 가장자리 화살표 자리를 OutPlacement에 넣고 true.
	 * 화면 안(가장자리에서 InsideMargin 안쪽)이거나 투영에 실패하면 false — 호출자는 화살표를 숨긴다.
	 * EdgeMargin은 화살표를 화면 가장자리에서 띄우는 거리 (위젯 좌표, DPI 배율 반영)
	 */
	bool TryPlaceAtScreenEdge(const UUserWidget& Widget, const FVector& WorldLocation, float EdgeMargin, float InsideMargin,
		FUSEdgeArrowPlacement& OutPlacement);
}
