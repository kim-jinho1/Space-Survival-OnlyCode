#include "UI/USCreditsWidget.h"
#include "US.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

UUSCreditsWidget::UUSCreditsWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 엔진 고지 두 줄은 Unreal Engine EULA가 배포 제품의 크레딧에 넣으라고 요구하는 문구다 (연도는 배포 연도)
	CreditsBody = NSLOCTEXT("USCreditsWidget", "CreditsBody",
		"개발\n"
		"조이영어 (zoeyeongeo)\n"
		"\n"
		"기체 모델\n"
		"Ultimate Spaceships Creator — Ebal Studios (Fab Standard License)\n"
		"\n"
		"UI 그림 · UI 효과음 · 배경\n"
		"Kenney (kenney.nl) — CC0 1.0\n"
		"Space Kit, UI Pack: Space Expansion, Game Icons, Interface Sounds, Sci-Fi Sounds\n"
		"\n"
		"능력 아이콘\n"
		"game-icons.net — Lorc, Delapouite — CC BY 3.0\n"
		"\n"
		"적 드론 · 이펙트 · 사운드\n"
		"Paragon: Twinblast, Lt. Belica 외 Paragon 애셋 — Epic Games\n"
		"효과음 일부: Lyra Starter Game — Epic Games\n"
		"\n"
		"배경음악\n"
		"space city — MintoDog (OpenGameArt) — CC0 1.0\n"
		"outer space loop — wipics (OpenGameArt) — CC0 1.0\n"
		"\n"
		"폰트\n"
		"Pretendard — Copyright (c) 2021, Kil Hyung-jin\n"
		"SIL Open Font License 1.1 (라이선스 원문: Content/Licenses/Pretendard-OFL.txt)\n"
		"\n"
		"스페이스 서바이벌 uses the Unreal® Engine. Unreal® is a trademark or registered trademark of Epic Games, Inc. in the United States of America and elsewhere.\n"
		"Unreal® Engine, Copyright 1998 – 2026, Epic Games, Inc. All rights reserved.");
}

void UUSCreditsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// BindWidget이라 WBP에 없으면 컴파일 단계에서 막히지만, 다시 열릴 때 중복 바인딩되지 않게 AddUnique
	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UUSCreditsWidget::HandleCloseClicked);
	}

	// 부모 WBP 안에 들어 있어 처음엔 숨겨 둔다 (Open으로 연다)
	SetVisibility(ESlateVisibility::Collapsed);
}

void UUSCreditsWidget::Open()
{
	if (CreditsText)
	{
		CreditsText->SetText(CreditsBody);
	}
	if (CreditsBody.IsEmpty())
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] CreditsBody is empty. Credits window shows nothing."), *GetName());
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UUSCreditsWidget::Close()
{
	SetVisibility(ESlateVisibility::Collapsed);
	OnClosed.ExecuteIfBound();
}

bool UUSCreditsWidget::IsOpen() const
{
	return GetVisibility() != ESlateVisibility::Collapsed && GetVisibility() != ESlateVisibility::Hidden;
}

void UUSCreditsWidget::HandleCloseClicked()
{
	Close();
}
