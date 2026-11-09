#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateTypes.h"

class UButton;
struct FKey;

/**
 * 세로 메뉴의 "지금 고른 항목" 하나 (타이틀·일시정지 창이 같이 쓴다).
 * ↑/↓(W/S, 게임패드 십자키)로 옮기고 Enter/Space(게임패드 A)로 실행, 마우스를 올려도 옮긴다.
 * 고른 항목만 채운 스타일(첫 버튼의 WBP 모양), 나머지는 글자만(둘째 버튼의 WBP 모양)으로 칠한다 — 디자인 수정은 WBP 한 곳에서 끝난다.
 * 위젯 클래스가 아니라 위젯이 멤버로 들고 쓰는 도우미다 (상속 대신 조합). 버튼은 위젯 트리가 소유하므로 약한 참조로 든다.
 */
class FUSKeyMenu
{
public:
	/** 키 처리 결과 */
	enum class EResult : uint8
	{
		/** 메뉴 키가 아님 (부른 쪽이 Super로 넘긴다) */
		Unhandled,
		/** 고른 항목을 옮겼다 */
		Moved,
		/** 고른 항목을 실행해야 한다 (GetSelectedIndex) */
		Activate
	};

	/**
	 * 버튼 목록(위에서부터)과 글자색을 받아 스타일을 복사하고 첫 항목을 고른다. 버튼이 2개보다 적으면 기본 스타일을 쓴다.
	 * OwnIdleStyleButtons에 든 버튼은 고르지 않았을 때 WBP에 칠해 둔 자기 모양·글자색을 쓴다 (예: 빨간 테두리 "게임 종료")
	 */
	void Initialize(const TArray<UButton*>& InButtons, const FSlateColor& InSelectedLabelColor, const FSlateColor& InIdleLabelColor,
		const TArray<UButton*>& OwnIdleStyleButtons = {});

	/** ↑/↓/Enter 처리 */
	EResult HandleKey(const FKey& Key);

	/** Index 항목을 고르고 다시 칠한다 (범위 밖이면 무시) */
	void Select(int32 Index);

	int32 GetSelectedIndex() const { return SelectedIndex; }

private:
	void Refresh() const;

	TArray<TWeakObjectPtr<UButton>> Buttons;
	FButtonStyle SelectedStyle;
	FButtonStyle IdleStyle;
	FSlateColor SelectedLabelColor;
	FSlateColor IdleLabelColor;
	/** 자기 모양을 쓰는 항목: 목록 번호 → 고르지 않았을 때 모양·글자색 */
	TMap<int32, FButtonStyle> OwnIdleStyles;
	TMap<int32, FSlateColor> OwnIdleLabelColors;
	int32 SelectedIndex = 0;
};
