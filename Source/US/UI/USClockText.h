#pragma once

#include "CoreMinimal.h"

/**
 * 초 → "분", "초(두 자리)" 텍스트. HUD 남은 시간·결산 생존 시간이 같은 모양으로 보이게 한 곳에 둔다.
 * FText::Format은 {0:D2} 같은 서식을 모르므로 초는 두 자리 숫자 옵션으로 만든다.
 */
namespace USClockText
{
	/** Seconds(음수는 0)를 분·초 텍스트로 나눈다 */
	void Split(float Seconds, FText& OutMinutes, FText& OutSeconds);
}
