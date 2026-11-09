#include "Core/USLevelTravelSubsystem.h"
#include "US.h"
#include "Core/USLoadingScreenSettings.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "MoviePlayer.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Styling/CoreStyle.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/** 검은 판을 다른 UI(HUD·결산 창, UMG 기본 ZOrder 0~수십) 위에 둔다 */
	constexpr int32 OverlayZOrder = 10000;
}

void UUSLevelTravelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadingFont = GetDefault<UUSLoadingScreenSettings>()->Font.LoadSynchronous();

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UUSLevelTravelSubsystem::HandlePreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UUSLevelTravelSubsystem::HandlePostLoadMap);
}

void UUSLevelTravelSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
	RemoveOverlay();

	Super::Deinitialize();
}

void UUSLevelTravelSubsystem::TravelFrom(const UObject* WorldContextObject, const TSoftObjectPtr<UWorld>& Map)
{
	TravelByNameFrom(WorldContextObject, FName(*Map.ToSoftObjectPath().GetLongPackageName()));
}

void UUSLevelTravelSubsystem::TravelByNameFrom(const UObject* WorldContextObject, const FName& LevelName)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	if (UUSLevelTravelSubsystem* Travel = GameInstance ? GameInstance->GetSubsystem<UUSLevelTravelSubsystem>() : nullptr)
	{
		Travel->TravelByName(LevelName);
		return;
	}

	UE_LOG(LogUS, Warning, TEXT("[USLevelTravelSubsystem] No travel subsystem. Opening %s without a fade."), *LevelName.ToString());
	UGameplayStatics::OpenLevel(WorldContextObject, LevelName);
}

void UUSLevelTravelSubsystem::Travel(const TSoftObjectPtr<UWorld>& Map)
{
	TravelByName(FName(*Map.ToSoftObjectPath().GetLongPackageName()));
}

void UUSLevelTravelSubsystem::TravelByName(const FName& LevelName)
{
	// 페이드아웃 중에 버튼을 한 번 더 눌러도 한 번만 떠난다
	if (FadeState == EFadeState::FadingOut || !PendingLevel.IsNone())
	{
		return;
	}

	PendingLevel = LevelName;
	FadeState = EFadeState::FadingOut;
	StateStartTime = FPlatformTime::Seconds();
	// 밝아지는 중에 떠나면 지금 어두운 정도에서 이어서 어두워진다
	AddOverlay();
	StartTicker();
}

void UUSLevelTravelSubsystem::HandlePreLoadMap(const FString& MapName)
{
	// 엔진이 맵을 바꾸며 뷰포트 위젯을 전부 지운다 → 우리 판도 이제 없는 것으로
	Overlay.Reset();
	PendingLevel = NAME_None;

	if (!IsMoviePlayerEnabled() || IsRunningDedicatedServer())
	{
		return;
	}

	const UUSLoadingScreenSettings* Settings = GetDefault<UUSLoadingScreenSettings>();
	FLoadingScreenAttributes Attributes;
	Attributes.bAutoCompleteWhenLoadingCompletes = true;
	Attributes.MinimumLoadingScreenDisplayTime = Settings->MinimumDisplaySeconds;
	Attributes.WidgetLoadingScreen = MakeLoadingScreenWidget();
	GetMoviePlayer()->SetupLoadingScreen(Attributes);
}

void UUSLevelTravelSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	if (LoadedWorld == nullptr || !LoadedWorld->IsGameWorld() || LoadedWorld->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	// 새 맵은 검은 상태로 시작해 HoldSeconds 동안 이펙트를 미리 틀고 밝아진다
	OverlayAlpha = 1.0f;
	AddOverlay();
	FadeState = EFadeState::Holding;
	StateStartTime = FPlatformTime::Seconds();
	StartTicker();

	WarmupEffects(LoadedWorld);
}

bool UUSLevelTravelSubsystem::HandleTicker(float DeltaTime)
{
	const UUSLoadingScreenSettings* Settings = GetDefault<UUSLoadingScreenSettings>();
	const double Elapsed = FPlatformTime::Seconds() - StateStartTime;

	switch (FadeState)
	{
	case EFadeState::FadingOut:
	{
		const float StartAlpha = OverlayAlpha;
		OverlayAlpha = Settings->FadeOutSeconds > 0.0f ? FMath::Max(StartAlpha, FMath::Clamp(static_cast<float>(Elapsed / Settings->FadeOutSeconds), 0.0f, 1.0f)) : 1.0f;
		if (OverlayAlpha >= 1.0f)
		{
			// OpenLevel은 다음 틱에 실제로 로드한다. 그동안 판은 검게 둔다
			FadeState = EFadeState::None;
			TickerHandle.Reset();
			if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
			{
				UGameplayStatics::OpenLevel(World, PendingLevel);
			}
			return false;
		}
		return true;
	}
	case EFadeState::Holding:
		if (Elapsed >= Settings->HoldSeconds)
		{
			ClearWarmupEffects();
			FadeState = EFadeState::FadingIn;
			StateStartTime = FPlatformTime::Seconds();
		}
		return true;
	case EFadeState::FadingIn:
		OverlayAlpha = Settings->FadeInSeconds > 0.0f ? 1.0f - FMath::Clamp(static_cast<float>(Elapsed / Settings->FadeInSeconds), 0.0f, 1.0f) : 0.0f;
		if (OverlayAlpha <= 0.0f)
		{
			FadeState = EFadeState::None;
			RemoveOverlay();
			TickerHandle.Reset();
			return false;
		}
		return true;
	default:
		TickerHandle.Reset();
		return false;
	}
}

void UUSLevelTravelSubsystem::StartTicker()
{
	if (!TickerHandle.IsValid())
	{
		TickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UUSLevelTravelSubsystem::HandleTicker));
	}
}

void UUSLevelTravelSubsystem::AddOverlay()
{
	if (Overlay.IsValid())
	{
		return;
	}

	UGameViewportClient* Viewport = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr;
	if (Viewport == nullptr)
	{
		return;
	}

	TWeakObjectPtr<UUSLevelTravelSubsystem> WeakThis(this);
	Overlay = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor_Lambda([WeakThis]()
		{
			return FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, WeakThis.IsValid() ? WeakThis->OverlayAlpha : 0.0f));
		})
		// 어두워지는·검은 동안은 클릭을 막아 두 번 떠나거나 검은 화면 뒤 버튼을 누르지 않게, 밝아지는 동안은 통과
		.Visibility_Lambda([WeakThis]()
		{
			const bool bBlocking = WeakThis.IsValid() && WeakThis->FadeState != EFadeState::FadingIn && WeakThis->OverlayAlpha > 0.01f;
			return bBlocking ? EVisibility::Visible : EVisibility::HitTestInvisible;
		});

	Viewport->AddViewportWidgetContent(Overlay.ToSharedRef(), OverlayZOrder);
}

void UUSLevelTravelSubsystem::RemoveOverlay()
{
	if (!Overlay.IsValid())
	{
		return;
	}

	if (UGameViewportClient* Viewport = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr)
	{
		Viewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
	}
	Overlay.Reset();
}

TSharedRef<SWidget> UUSLevelTravelSubsystem::MakeLoadingScreenWidget() const
{
	const UUSLoadingScreenSettings* Settings = GetDefault<UUSLoadingScreenSettings>();

	FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Regular", 28);
	FSlateFontInfo TipFont = FCoreStyle::GetDefaultFontStyle("Regular", 18);
	if (LoadingFont)
	{
		LabelFont = FSlateFontInfo(LoadingFont, 28);
		TipFont = FSlateFontInfo(LoadingFont, 18);
	}

	const FText Tip = Settings->Tips.IsEmpty() ? FText::GetEmpty() : Settings->Tips[FMath::RandRange(0, Settings->Tips.Num() - 1)];

	// 타이틀·HUD와 같은 하늘색 계열 (글자), 바탕은 검정 — 앞뒤 페이드의 검은 판과 이어진다
	return SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor::Black)
		.Padding(FMargin(80.0f, 64.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Text(Settings->LoadingLabel)
				.Font(LabelFont)
				.ColorAndOpacity(FLinearColor(0.75f, 0.95f, 1.0f, 1.0f))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(FMargin(0.0f, 12.0f, 0.0f, 0.0f))
			[
				SNew(STextBlock)
				.Text(Tip)
				.Font(TipFont)
				.ColorAndOpacity(FLinearColor(0.5f, 0.58f, 0.68f, 1.0f))
			]
		];
}

void UUSLevelTravelSubsystem::WarmupEffects(UWorld* World)
{
	WarmupComponents.Reset();

	// 대장이 없는 맵(타이틀·격납고)은 전투 이펙트를 쓰지 않는다
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (Pawn == nullptr)
	{
		return;
	}

	// 실제로 화면에 한 번 그려져야 셰이더 상태가 준비되므로 대장 위치(카메라 안)에서 튼다. 검은 판이 덮고 있어 보이지 않는다
	const FVector Location = Pawn->GetActorLocation();
	int32 SpawnedCount = 0;
	for (const TSoftObjectPtr<UParticleSystem>& EffectPtr : GetDefault<UUSLoadingScreenSettings>()->WarmupEffects)
	{
		if (UParticleSystem* Effect = EffectPtr.LoadSynchronous())
		{
			WarmupComponents.Add(UGameplayStatics::SpawnEmitterAtLocation(World, Effect, Location, FRotator::ZeroRotator, FVector(1.0f), true));
			++SpawnedCount;
		}
	}

	UE_LOG(LogUS, Log, TEXT("[USLevelTravelSubsystem] Warmed up %d effects in %s."), SpawnedCount, *GetNameSafe(World));
}

void UUSLevelTravelSubsystem::ClearWarmupEffects()
{
	// 셰이더 상태는 검은 동안 이미 준비됐다. 입자까지 바로 지워야 밝아질 때 폭발 잔해가 안 보인다 (Deactivate만 하면 이미 나온 입자는 남는다)
	for (const TWeakObjectPtr<UParticleSystemComponent>& Component : WarmupComponents)
	{
		if (Component.IsValid())
		{
			Component->KillParticlesForced();
			Component->DestroyComponent();
		}
	}
	WarmupComponents.Reset();
}
