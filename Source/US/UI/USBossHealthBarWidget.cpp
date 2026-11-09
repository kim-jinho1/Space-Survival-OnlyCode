#include "UI/USBossHealthBarWidget.h"
#include "Agents/USBossEnemyCharacter.h"
#include "UI/USEdgeArrowUtils.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Engine/Texture2D.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	void SetCollapsed(UWidget* Widget)
	{
		if (Widget)
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	void SetShown(UWidget* Widget)
	{
		if (Widget)
		{
			Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}

void UUSBossHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 판 시작부터 뷰포트에 있으므로 보스 전까지는 아무것도 안 보인다
	SetCollapsed(WarningPanel);
	SetCollapsed(EdgeGlow);
	SetCollapsed(DirectionArrow);
	SetCollapsed(BarPanel);
	SetCollapsed(ApproachText);
	SetCollapsed(SignalBand);
	SetCollapsed(CallPanel);
}

void UUSBossHealthBarWidget::NativeDestruct()
{
	Unbind();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WarningTimerHandle);
	}

	Super::NativeDestruct();
}

void UUSBossHealthBarWidget::ShowApproach(const FText& BossName, float SecondsLeft)
{
	if (ApproachText == nullptr)
	{
		return;
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("Boss"), BossName);
	Args.Add(TEXT("Seconds"), FText::AsNumber(FMath::CeilToInt(SecondsLeft)));
	ApproachText->SetText(FText::Format(ApproachFormat, Args));
	SetShown(ApproachText);
}

void UUSBossHealthBarWidget::ShowSignal(const FText& BossName)
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("Boss"), BossName);

	// 글자 한 줄만 바뀌면 싸우다 놓친다 (사용자) → 가운데 띠로 순간을 알리고, 위 가운데 호출 칸을 남긴다.
	// 보스 등장 WARNING 띠와 같은 모양에 색만 주황이라 "예고 → 실제 등장"이 이어진다
	if (CallPanel)
	{
		SetCollapsed(ApproachText);
		if (CallText)
		{
			CallText->SetText(FText::Format(CallFormat, Args));
		}
		SetShown(CallPanel);
	}
	else if (ApproachText)
	{
		ApproachText->SetText(FText::Format(SignalFormat, Args));
		SetShown(ApproachText);
	}

	if (SignalBand)
	{
		SignalBand->SetRenderOpacity(0.0f);
		SetShown(SignalBand);
	}

	if (SignalSound)
	{
		UGameplayStatics::PlaySound2D(this, SignalSound);
	}

	bSignalActive = true;
	SignalElapsed = 0.0f;
}

void UUSBossHealthBarWidget::HideApproach()
{
	SetCollapsed(ApproachText);
	SetCollapsed(SignalBand);
	SetCollapsed(CallPanel);
	bSignalActive = false;
}

void UUSBossHealthBarWidget::ShowWarning(AActor* Focus)
{
	WarningFocus = Focus;
	WarningElapsed = 0.0f;
	bWarningActive = true;

	SetShown(WarningPanel);
	SetShown(EdgeGlow);

	// 첫 틱 전 한 프레임 동안 완전히 보였다가 페이드인하지 않게 투명에서 시작한다
	if (WarningPanel)
	{
		WarningPanel->SetRenderOpacity(0.0f);
	}
	if (EdgeGlow)
	{
		EdgeGlow->SetRenderOpacity(0.0f);
	}

	// 첫 소리는 바로, 나머지는 NativeTick이 WarningSoundInterval마다 (경고 띠와 같은 시계라 따로 타이머를 두지 않는다)
	WarningSoundsPlayed = 0;
	if (WarningSound)
	{
		UGameplayStatics::PlaySound2D(this, WarningSound);
		WarningSoundsPlayed = 1;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(WarningTimerHandle, this, &UUSBossHealthBarWidget::HideWarning, WarningSeconds, false);
	}
}

void UUSBossHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bSignalActive)
	{
		SignalElapsed += InDeltaTime;

		// 띠: 0.15초 페이드인, 끝 0.5초 페이드아웃 뒤 숨김
		if (SignalBand && SignalBand->GetVisibility() != ESlateVisibility::Collapsed)
		{
			const float BandFade = FMath::Clamp(FMath::Min(SignalElapsed / 0.15f, (SignalBandSeconds - SignalElapsed) / 0.5f), 0.0f, 1.0f);
			SignalBand->SetRenderOpacity(BandFade);
			if (SignalElapsed >= SignalBandSeconds)
			{
				SetCollapsed(SignalBand);
			}
		}

		// 호출 칸 B 키: 0.4~1 사이 사인 맥박 (지금 누를 수 있다는 표시)
		if (CallKey)
		{
			const float Phase = FMath::Cos(SignalElapsed / CallKeyPulsePeriod * 2.0f * PI) * 0.5f + 0.5f;
			CallKey->SetRenderOpacity(FMath::Lerp(0.4f, 1.0f, Phase));
		}
	}

	if (!bWarningActive)
	{
		return;
	}

	WarningElapsed += InDeltaTime;

	if (WarningSound && WarningSoundsPlayed < WarningSoundCount && WarningElapsed >= WarningSoundsPlayed * WarningSoundInterval)
	{
		UGameplayStatics::PlaySound2D(this, WarningSound);
		++WarningSoundsPlayed;
	}

	// 줄무늬: 사인으로 부드럽게 깜빡이고, 한 칸 너비로 되감으며 흐른다 (그림이 반복이라 이음매가 안 보인다).
	// WBP에서 줄무늬 이미지 왼쪽을 한 칸만큼 화면 밖으로 내밀어(음수 여백) 움직여도 왼쪽 끝이 비지 않게 한다
	const float BlinkPhase = FMath::Cos(WarningElapsed / StripeBlinkPeriod * 2.0f * PI) * 0.5f + 0.5f;
	const float StripeOpacity = FMath::Lerp(StripeDimOpacity, 1.0f, BlinkPhase);
	// 되감는 길이는 그림 한 칸의 정수배여야 이음매가 안 보인다. 엔진이 반복 칸을 브러시 크기(ImageSize)로 잡든 텍스처 원본 크기로 잡든
	// 맞도록 둘의 최소공배수로 되감는다 (진단 로그: 48로 되감았더니 0.4초마다 줄이 튐 — 실제 반복 칸이 48이 아니었다)
	float WrapWidth = StripeTileWidth;
	if (const UImage* StripeImage = Cast<UImage>(StripeTop))
	{
		const FSlateBrush& Brush = StripeImage->GetBrush();
		const int32 BrushWidth = FMath::Max(1, FMath::RoundToInt(Brush.ImageSize.X));
		const UTexture2D* Texture = Cast<UTexture2D>(Brush.GetResourceObject());
		const int32 TextureWidth = Texture ? FMath::Max(1, Texture->GetSizeX()) : BrushWidth;
		WrapWidth = static_cast<float>(BrushWidth / FMath::GreatestCommonDivisor(BrushWidth, TextureWidth) * TextureWidth);
	}
	const float Scroll = FMath::Fmod(WarningElapsed * StripeScrollSpeed, WrapWidth);
	if (StripeTop)
	{
		StripeTop->SetRenderOpacity(StripeOpacity);
		StripeTop->SetRenderTranslation(FVector2D(Scroll, 0.0f));
	}
	if (StripeBottom)
	{
		StripeBottom->SetRenderOpacity(StripeOpacity);
		StripeBottom->SetRenderTranslation(FVector2D(WrapWidth - Scroll, 0.0f));
	}
	const bool bBright = BlinkPhase > 0.5f;

	// 들어올 때 0.2초 페이드인, 끝나기 전 0.6초 페이드아웃 (사용자: "갑자기 사라짐"). 띠·테두리·화살표에 같이 곱한다
	constexpr float FadeInSeconds = 0.2f;
	constexpr float FadeOutSeconds = 0.6f;
	const float Fade = FMath::Clamp(FMath::Min(WarningElapsed / FadeInSeconds, (WarningSeconds - WarningElapsed) / FadeOutSeconds), 0.0f, 1.0f);
	if (WarningPanel)
	{
		WarningPanel->SetRenderOpacity(Fade);
	}

	// 테두리: 0.35~1 사이에서 사인 맥박
	if (EdgeGlow)
	{
		const float Phase = FMath::Sin(WarningElapsed / EdgeGlowPeriod * 2.0f * PI) * 0.5f + 0.5f;
		EdgeGlow->SetRenderOpacity(FMath::Lerp(0.35f, 1.0f, Phase) * Fade);
	}

	if (DirectionArrow)
	{
		const AActor* Focus = WarningFocus.Get();
		const bool bShowArrow = Focus && PlaceArrow(Focus->GetActorLocation());
		DirectionArrow->SetVisibility(bShowArrow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		DirectionArrow->SetRenderOpacity((bBright ? 1.0f : 0.5f) * Fade);
	}
}

bool UUSBossHealthBarWidget::PlaceArrow(const FVector& WorldLocation) const
{
	if (DirectionArrow == nullptr)
	{
		return false;
	}

	// 화면 안 판정은 화면 끝 그대로(여백 0) — 보스가 조금이라도 보이면 경고 화살표는 필요 없다
	FUSEdgeArrowPlacement Placement;
	if (!USEdgeArrowUtils::TryPlaceAtScreenEdge(*this, WorldLocation, ArrowEdgeMargin, 0.0f, Placement))
	{
		return false;
	}

	DirectionArrow->SetRenderTranslation(Placement.EdgeOffset);
	DirectionArrow->SetRenderTransformAngle(Placement.AngleDegrees);
	return true;
}

void UUSBossHealthBarWidget::HideWarning()
{
	bWarningActive = false;
	WarningFocus.Reset();
	SetCollapsed(WarningPanel);
	SetCollapsed(EdgeGlow);
	SetCollapsed(DirectionArrow);
}

void UUSBossHealthBarWidget::SetBoss(AUSBossEnemyCharacter* Boss, const FText& BossName)
{
	Unbind();
	if (Boss == nullptr)
	{
		return;
	}

	BoundBoss = Boss;
	HealthChangedHandle = Boss->OnBossHealthChanged.AddUObject(this, &UUSBossHealthBarWidget::HandleBossHealthChanged);

	if (NameText)
	{
		NameText->SetText(BossName);
	}
	HandleBossHealthChanged(Boss->GetCurrentHealth(), Boss->GetMaxHealth());

	SetShown(BarPanel);
}

void UUSBossHealthBarWidget::HideAll()
{
	Unbind();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(WarningTimerHandle);
	}
	HideWarning();
	SetCollapsed(BarPanel);
	HideApproach();
}

void UUSBossHealthBarWidget::HandleBossHealthChanged(float CurrentHealth, float MaxHealth)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f);
	}
}

void UUSBossHealthBarWidget::Unbind()
{
	if (AUSBossEnemyCharacter* Boss = BoundBoss.Get())
	{
		Boss->OnBossHealthChanged.Remove(HealthChangedHandle);
	}
	BoundBoss.Reset();
	HealthChangedHandle.Reset();
}
