#pragma once

#include "CoreMinimal.h"

class UPanelWidget;
class UWidgetTree;

/**
 * 단계 점(●●○○○) 또는 단계 칸(■■□□□) 한 줄. 격납고 강화 칸과 레벨업 카드가 같은 함수를 쓴다.
 * 점은 텍스처 없이 Slate 둥근 사각형 브러시로 그리고, 개수가 바뀔 때만 위젯을 다시 만든다(평소엔 브러시만 바꾼다).
 */
namespace USPipRow
{
	/** 점 모양·색 */
	struct FStyle
	{
		/** 너비 (Height가 0이면 높이도 이 값 = 정사각형) */
		float Size = 9.0f;
		/** 높이. 0이면 Size */
		float Height = 0.0f;
		/** 모서리 반지름. 음수면 짧은 변의 절반 = 원·알약 모양 */
		float CornerRadius = -1.0f;
		float Spacing = 3.0f;
		FLinearColor FilledColor = FLinearColor(0.37f, 0.88f, 0.95f, 1.0f);
		FLinearColor EmptyColor = FLinearColor(0.16f, 0.23f, 0.31f, 1.0f);
		/** 다음에 채울 칸(Level번째)의 테두리 색. 알파가 0이면 테두리 없이 빈 칸과 같다 */
		FLinearColor NextOutlineColor = FLinearColor::Transparent;
		float OutlineWidth = 1.0f;
	};

	/** Row(HorizontalBox)에 MaxLevel개 칸을 두고 앞의 Level개를 채운 색으로 칠한다. MaxLevel이 0이면 줄을 숨긴다 */
	void Refresh(UWidgetTree& Tree, UPanelWidget& Row, int32 Level, int32 MaxLevel, const FStyle& Style);
}
