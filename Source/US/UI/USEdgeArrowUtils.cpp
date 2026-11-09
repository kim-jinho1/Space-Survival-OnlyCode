#include "UI/USEdgeArrowUtils.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"

bool USEdgeArrowUtils::TryPlaceAtScreenEdge(const UUserWidget& Widget, const FVector& WorldLocation, float EdgeMargin, float InsideMargin,
	FUSEdgeArrowPlacement& OutPlacement)
{
	APlayerController* PlayerController = Widget.GetOwningPlayer();
	const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(&Widget);
	if (PlayerController == nullptr || ViewportScale <= 0.0f)
	{
		return false;
	}

	// 위젯 좌표계(DPI 배율 반영)의 화면 크기와 중앙
	const FVector2D ScreenSize = UWidgetLayoutLibrary::GetViewportSize(&Widget) / ViewportScale;
	const FVector2D Center = ScreenSize * 0.5f;

	// 쿼터뷰 카메라는 대상이 카메라 뒤로 가지 않지만, 투영에 실패하면 방향을 알 수 없으니 숨긴다
	FVector2D ScreenPosition;
	if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, WorldLocation, ScreenPosition, false))
	{
		return false;
	}

	// 이미 화면 안에 보이면 화살표가 필요 없다
	const bool bOnScreen = ScreenPosition.X >= InsideMargin && ScreenPosition.X <= ScreenSize.X - InsideMargin
		&& ScreenPosition.Y >= InsideMargin && ScreenPosition.Y <= ScreenSize.Y - InsideMargin;
	if (bOnScreen)
	{
		return false;
	}

	const FVector2D Direction = (ScreenPosition - Center).GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		return false;
	}

	// 중앙에서 대상 쪽으로 뻗은 선이 여백만큼 안쪽 사각형 테두리와 만나는 점
	const FVector2D HalfExtent = Center - FVector2D(EdgeMargin);
	const float ScaleX = FMath::Abs(Direction.X) > KINDA_SMALL_NUMBER ? HalfExtent.X / FMath::Abs(Direction.X) : BIG_NUMBER;
	const float ScaleY = FMath::Abs(Direction.Y) > KINDA_SMALL_NUMBER ? HalfExtent.Y / FMath::Abs(Direction.Y) : BIG_NUMBER;

	OutPlacement.Direction = Direction;
	OutPlacement.EdgeOffset = Direction * FMath::Min(ScaleX, ScaleY);
	OutPlacement.AngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X));
	return true;
}
