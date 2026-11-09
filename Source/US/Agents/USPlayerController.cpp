#include "Agents/USPlayerController.h"
#include "Core/USLevelTravelSubsystem.h"
#include "US.h"
#include "Progression/USUpgradeComponent.h"
#include "Shop/USShopComponent.h"
#include "UI/USPlayerHUD.h"
#include "UI/USLevelUpWidget.h"
#include "UI/USShopWidget.h"
#include "UI/USDamageNumberWidget.h"
#include "UI/USDamageVignetteWidget.h"
#include "UI/USSquadWidget.h"
#include "UI/USOffscreenIndicatorWidget.h"
#include "UI/USSupplyNoticeWidget.h"
#include "UI/USHordeNoticeWidget.h"
#include "UI/USRadarWidget.h"
#include "UI/USGameOverWidget.h"
#include "UI/USClearWidget.h"
#include "UI/USBossHealthBarWidget.h"
#include "UI/USStageBannerWidget.h"
#include "Components/USStageTransitionComponent.h"
#include "Pickups/USPickup.h"
#include "EngineUtils.h"
#include "Agents/USBossEnemyCharacter.h"
#include "Datas/USEnemyTypeData.h"
#include "UI/USPauseWidget.h"
#include "UI/USControlsGuideWidget.h"
#include "Core/USRunStatsSubsystem.h"
#include "Progression/USMetaProgressionSubsystem.h"
#include "Datas/USMetaUpgradeData.h"
#include "Datas/USStageData.h"
#include "Datas/USZoneData.h"
#include "Spawning/USEnemySpawner.h"
#include "Shop/USSupplyShip.h"
#include "Agents/USPlayerCharacter.h"
#include "Agents/USAllyDroneCharacter.h"
#include "Components/USSquadComponent.h"
#include "Core/USCameraShake.h"
#include "Core/USGameUserSettings.h"
#include "Core/USCombatFeedbackSubsystem.h"
#include "Core/USTeamSubsystem.h"
#include "Components/USAttributeComponent.h"
#include "Components/USCurrencyComponent.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Components/USExperienceComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HAL/IConsoleManager.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
	/**
	 * 시험용: 1이면 레벨업 카드 창 없이 첫 카드를 바로 고른다 (MCP 자동 측정이 카드 창에서 멈추지 않게).
	 * ECVF_Cheat로 두면 안 된다: 모듈이 로드되며 늦게 등록되는 치트 변수는 엔진이 ini 값을 버린다(치트 방지, ConsoleManager.cpp).
	 * MCP에는 콘솔 실행 도구가 없어 Saved/Config/WindowsEditor/Engine.ini [ConsoleVariables]로 켠다
	 */
	TAutoConsoleVariable<int32> CVarAutoPickCards(
		TEXT("US.Test.AutoPickCards"),
		0,
		TEXT("1 = pick the first level-up card automatically (no card widget, no pause). For automated balance runs."),
		ECVF_Default);

#if !UE_BUILD_SHIPPING
	/** 시험용: 5분을 기다리지 않고 보스전을 바로 연다 (경고 띠·체력바·격파·클리어 창까지 실제 흐름 그대로) */
	FAutoConsoleCommandWithWorld GUSTestBossCommand(
		TEXT("US.Test.Boss"),
		TEXT("Show the boss signal now (skips the clear time). Run again while the signal is up to summon without B."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (AUSPlayerController* PC = World ? Cast<AUSPlayerController>(World->GetFirstPlayerController()) : nullptr)
			{
				PC->DebugStartBossFight();
			}
		}));

	/** 시험용: 1:30을 기다리지 않고 지금 스테이지를 클리어 시각으로 넘긴다 (보스 스테이지면 경고 뒤 보스) */
	FAutoConsoleCommandWithWorld GUSTestClearStageCommand(
		TEXT("US.Test.ClearStage"),
		TEXT("Jump the stage clock to the clear time (boss stages summon the boss)."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (AUSPlayerController* PC = World ? Cast<AUSPlayerController>(World->GetFirstPlayerController()) : nullptr)
			{
				PC->DebugClearStage();
			}
		}));

	/** 시험용: 살아 있는 부하 한 기를 격추한다 (격추 알림·부활 확인용). 실제 피해 경로(OnTakeAnyDamage)를 탄다 */
	FAutoConsoleCommandWithWorld GUSTestKillAllyCommand(
		TEXT("US.Test.KillAlly"),
		TEXT("Shoot down one living squad member now."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
			const AUSPlayerCharacter* Leader = PC ? Cast<AUSPlayerCharacter>(PC->GetPawn()) : nullptr;
			const UUSSquadComponent* Squad = Leader ? Leader->GetSquadComponent() : nullptr;
			if (Squad == nullptr)
			{
				return;
			}

			for (const FUSSquadMember& Member : Squad->GetMembers())
			{
				if (Member.IsActive() && Member.Ally)
				{
					UGameplayStatics::ApplyDamage(Member.Ally, 1.0e6f, nullptr, nullptr, nullptr);
					return;
				}
			}
			UE_LOG(LogUS, Warning, TEXT("US.Test.KillAlly: no living squad member."));
		}));
#endif
}

AUSPlayerController::AUSPlayerController()
{
	UpgradeComponent = CreateDefaultSubobject<UUSUpgradeComponent>(TEXT("UpgradeComponent"));
	ShopComponent = CreateDefaultSubobject<UUSShopComponent>(TEXT("ShopComponent"));
	StageTransitionComponent = CreateDefaultSubobject<UUSStageTransitionComponent>(TEXT("StageTransitionComponent"));

	// 쿼터뷰 커서 조준: 커서가 곧 조준점이므로 항상 보이고, 십자선 모양으로.
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;

	// BP에서 따로 지정하지 않아도 흔들리도록 기본 흔들림을 둔다
	HitShakeClass = UUSCameraShake::StaticClass();
}

void AUSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	InitializeRunDuration();
	InitializeChainExplosion();

	if (const UUSMetaProgressionSubsystem* Meta = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr)
	{
		CardRerollsLeft = FMath::RoundToInt(Meta->GetAdditiveTotal(MetaTree, EUSMetaEffect::CardReroll));
	}

	if (UWorld* World = GetWorld())
	{
		RunStartTime = World->GetTimeSeconds();
		StageStartTime = RunStartTime;

		// 남은 시간은 초 단위로만 보이므로 매 프레임이 아니라 1초 타이머로 갱신한다
		if (ClearTimeSeconds > 0.0f || HasRunTimeLimit())
		{
			World->GetTimerManager().SetTimer(RunClockTimerHandle, this, &AUSPlayerController::RefreshRunClock, 1.0f, true);
		}
	}

	// 스포너는 맵에 배치된 한 대라 한 번 찾아 둔다 (로그용이라 없으면 배율만 빠진다)
	EnemySpawner = Cast<AUSEnemySpawner>(UGameplayStatics::GetActorOfClass(this, AUSEnemySpawner::StaticClass()));

	// 격납고 지역 지도에서 고른 스테이지 (PIE로 전투 맵을 바로 열면 없음 → 맵 스포너 설정 그대로)
	if (const UUSMetaProgressionSubsystem* Meta = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr)
	{
		CurrentZone = Meta->GetSortieZone();
		CurrentStage = Meta->GetSortieStage();
	}
	ApplyStageToSpawner();

	if (ShopComponent)
	{
		ShopComponent->OnRunTimeExtended.AddUObject(this, &AUSPlayerController::HandleRunTimeExtended);
	}

	BindCombatFeedback();

	AddUIMappingContext();
	ApplyGameplayInputMode();

	if (IsLocalController() && FSlateApplication::IsInitialized())
	{
		ActivationChangedHandle = FSlateApplication::Get().OnApplicationActivationStateChanged()
			.AddUObject(this, &AUSPlayerController::HandleApplicationActivationChanged);
	}

	// 단일 플레이어: 실제 셋업은 OnPossess에서 일괄 처리(이중 바인딩 방지).
	// 단, BeginPlay 시점에 이미 폰을 소유 중인데 OnPossess를 놓친 경우만 보강.
	if (GetPawn() != nullptr && !PawnAttribute.IsValid())
	{
		// 늦게 불려도 분대가 시작 부하를 바로 스폰하고 기존 부하에도 강화를 적용하므로 결과는 같다
		ApplyMetaProgression(GetPawn());
		SetupHUD();
		BindToPawn(GetPawn());
		SetupSquadWidget(GetPawn());
	}

	// 다음 틱: HUD가 뷰포트에 올라가고 분대가 시작 부하를 스폰한 뒤 진입 비행 → 카드를 띄우고 멈춘다
	GetWorldTimerManager().SetTimerForNextTick(this, &AUSPlayerController::BeginStageArrival);
}

void AUSPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 분대가 BeginPlay에서 시작 부하를 스폰하기 전에 넣어야 하므로 HUD·바인딩보다 먼저
	ApplyMetaProgression(InPawn);
	SetupHUD();
	BindToPawn(InPawn);
	SetupSquadWidget(InPawn);
}

void AUSPlayerController::SetupHUD()
{
	if (!IsLocalController() || HUDWidget != nullptr)
	{
		return;
	}

	if (HUDWidgetClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] HUDWidgetClass is not set. HUD will not be shown."), *GetName());
		return;
	}

	SetupDamageNumbers();
	SetupDamageVignette();
	SetupSupplyShipUI();
	SetupHordeNotice();
	SetupRadar();
	SetupBossHealthBar();
	SetupStageBanner();

	HUDWidget = CreateWidget<UUSPlayerHUD>(this, HUDWidgetClass);
	if (HUDWidget)
	{
		HUDWidget->AddToViewport();

		if (APawn* CurrentPawn = GetPawn())
		{
			HUDWidget->InitializeForPawn(CurrentPawn);
			bHUDInitialized = true;
		}

		// 런 시계 타이머는 1초 뒤 첫 갱신이므로, 그동안 WBP 기본 글자가 보이지 않게 바로 한 번 채운다
		InitializeRunDuration();
		if (ClearTimeSeconds > 0.0f || HasRunTimeLimit())
		{
			RefreshRunClock();
		}
	}
}

void AUSPlayerController::BindToPawn(APawn* InPawn)
{
	if (InPawn == nullptr)
	{
		return;
	}

	UUSAttributeComponent* Attribute = InPawn->FindComponentByClass<UUSAttributeComponent>();
	UUSExperienceComponent* Experience = InPawn->FindComponentByClass<UUSExperienceComponent>();
	PawnAttribute = Attribute;
	PawnCurrency = InPawn->FindComponentByClass<UUSCurrencyComponent>();
	PawnExperience = Experience;
	
	// HUD 초기화는 SetupHUD()에서 일괄 처리(중복 바인딩 방지).
	// SetupHUD가 폰보다 먼저 불려 초기화를 못 했다면 여기서 한 번만 보강.
	if (HUDWidget && !bHUDInitialized)
	{
		HUDWidget->InitializeForPawn(InPawn);
		bHUDInitialized = true;
	}

	// OnLevelUp은 한 번만 바인딩 (OnPossess/BeginPlay 중복 호출 대비)
	if (Experience && !bLevelUpBound)
	{
		Experience->OnLevelUp.AddUObject(this, &AUSPlayerController::HandlePlayerLevelUp);
		bLevelUpBound = true;
	}

	// OnDeath도 한 번만 바인딩
	if (Attribute && !bDeathBound)
	{
		Attribute->OnDeath.AddUObject(this, &AUSPlayerController::HandlePlayerDeath);
		bDeathBound = true;
	}

	// 대장이 맞으면 화면을 흔든다 (한 번만 바인딩)
	if (Attribute && !bDamagedBound)
	{
		Attribute->OnDamaged.AddUObject(this, &AUSPlayerController::HandlePawnDamaged);
		bDamagedBound = true;
	}
}

void AUSPlayerController::HandlePlayerLevelUp()
{
	// 마지막 처치로 경험치가 차서 죽는 순간 레벨업하면 카드가 게임오버 창과 겹치므로 받지 않는다
	if (bGameOver)
	{
		return;
	}

	++PendingCardSelections;

	// 이미 선택/모달 중이면 현재 것이 끝날 때 다음이 표시된다.
	if (!bModalOpen)
	{
		TryShowNextLevelUpCard();
	}
}

void AUSPlayerController::TryShowNextLevelUpCard()
{
	if (PendingCardSelections <= 0)
	{
		return;
	}

	TArray<FUSLevelUpCard> Choices;
	if (UpgradeComponent)
	{
		Choices = UpgradeComponent->GenerateChoices(3, GetPawnSquad());
	}

	if (Choices.Num() == 0)
	{
		PendingCardSelections = FMath::Max(0, PendingCardSelections - 1);
		TryShowNextLevelUpCard();
		return;
	}

	if (CVarAutoPickCards.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogUS, Log, TEXT("[Test] auto-picked level-up card: %s"), *Choices[0].Title.ToString());
		HandleCardSelected(Choices[0]);
		return;
	}

	// 위젯이 없으면 고를 방법이 없으므로 쌓인 선택을 버리고 알린다 (보이지 않는 선택 대기를 만들지 않는다)
	if (LevelUpWidgetClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] LevelUpWidgetClass is not set. Level-up choices are skipped."), *GetName());
		PendingCardSelections = 0;
		return;
	}

	LevelUpWidget = CreateWidget<UUSLevelUpWidget>(this, LevelUpWidgetClass);
	if (LevelUpWidget == nullptr)
	{
		return;
	}

	LevelUpWidget->OnCardSelected.BindUObject(this, &AUSPlayerController::HandleCardSelected);
	LevelUpWidget->OnRerollRequested.BindUObject(this, &AUSPlayerController::HandleCardRerollRequested);
	LevelUpWidget->AddToViewport();
	LevelUpWidget->SetupCards(Choices);
	LevelUpWidget->SetRerollsLeft(CardRerollsLeft);
	if (UpgradeComponent)
	{
		LevelUpWidget->SetupBuildProgress(UpgradeComponent->GetBuildProgress());
	}

	EnterModalUI();

	// UIOnly는 포커스를 뷰포트에 두므로, 1·2·3 키가 카드 창에 가도록 포커스를 옮긴다
	LevelUpWidget->SetKeyboardFocus();
}

void AUSPlayerController::HandleCardSelected(const FUSLevelUpCard& Card)
{
	if (UpgradeComponent)
	{
		UpgradeComponent->ApplyCard(Card, GetPawn(), GetPawnSquad());
	}

	if (LevelUpWidget)
	{
		LevelUpWidget->RemoveFromParent();
		LevelUpWidget = nullptr;
	}

	PendingCardSelections = FMath::Max(0, PendingCardSelections - 1);

	if (PendingCardSelections > 0)
	{
		TryShowNextLevelUpCard();
	}
	else
	{
		ExitModalUI();
	}
}

void AUSPlayerController::HandleCardRerollRequested()
{
	if (CardRerollsLeft <= 0 || LevelUpWidget == nullptr || UpgradeComponent == nullptr)
	{
		return;
	}

	// 다시 뽑아도 후보가 없으면(보급 카드도 없음) 지금 카드를 그대로 두고 횟수도 쓰지 않는다
	const TArray<FUSLevelUpCard> Choices = UpgradeComponent->GenerateChoices(3, GetPawnSquad());
	if (Choices.Num() == 0)
	{
		return;
	}

	--CardRerollsLeft;
	LevelUpWidget->SetupCards(Choices);
	LevelUpWidget->SetRerollsLeft(CardRerollsLeft);
}

// ---------------- 상점 (보급선) ----------------

void AUSPlayerController::RequestOpenShop()
{
	// 카드 선택 중이거나 이미 상점이 열려 있으면 겹쳐 열지 않는다
	if (bModalOpen || bGameOver)
	{
		return;
	}

	OpenShop();
}

void AUSPlayerController::OpenShop()
{
	// 위젯이 없으면 상점을 열지 않는다 (보이지 않는 상점 대기를 만들지 않는다)
	if (ShopWidgetClass == nullptr || ShopComponent == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] ShopWidgetClass or ShopComponent is not set. Shop is skipped."), *GetName());
		return;
	}

	ShopWidget = CreateWidget<UUSShopWidget>(this, ShopWidgetClass);
	if (ShopWidget == nullptr)
	{
		return;
	}

	ShopWidget->OnOfferSelected.BindUObject(this, &AUSPlayerController::HandleShopOfferSelected);
	ShopWidget->OnShopClosed.BindUObject(this, &AUSPlayerController::HandleShopClosed);
	ShopWidget->AddToViewport();
	RefreshShopWidget();

	EnterModalUI();
}

void AUSPlayerController::HandleShopOfferSelected(int32 Index)
{
	if (ShopComponent == nullptr)
	{
		return;
	}

	if (ShopComponent->TryPurchase(Index, GetPawnSquad(), PawnCurrency.Get()))
	{
		// 부활한 줄은 빠지고 가격·골드가 바뀌므로 목록을 새로 만든다
		RefreshShopWidget();
	}
}

void AUSPlayerController::RefreshShopWidget()
{
	if (ShopComponent == nullptr || ShopWidget == nullptr)
	{
		return;
	}

	ShopComponent->RefreshOffers(GetPawnSquad(), CanExtendRunTime());

	const int32 Gold = PawnCurrency.IsValid() ? PawnCurrency->GetGold() : 0;
	ShopWidget->SetupOffers(ShopComponent->GetOffers(), Gold);
}

void AUSPlayerController::HandleShopClosed()
{
	if (ShopWidget)
	{
		ShopWidget->RemoveFromParent();
		ShopWidget = nullptr;
	}

	ExitModalUI();
}

// ---------------- 핫키 ----------------

void AUSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// 키는 IMC_UI에 있다 (Esc·P 일시정지, B 보스 호출, 게임오버 R·Enter). 멈춘 중에도 받을 키는 입력 애셋의 bTriggerWhenPaused로 켠다.
	// 에디터 PIE에서는 Esc가 PIE 종료 키라 게임에 오지 않으므로 일시정지에 P도 묶여 있다
	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (EnhancedInput == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: InputComponent is not an EnhancedInputComponent; UI keys are not bound"), *GetName());
		return;
	}

	if (PauseAction)
	{
		EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &AUSPlayerController::Input_Pause);
	}
	if (CallBossAction)
	{
		// 신호가 떠 있을 때만 받는다. 멈춘 동안(카드·상점)에는 부르지 않는다
		EnhancedInput->BindAction(CallBossAction, ETriggerEvent::Started, this, &AUSPlayerController::OnCallBossKey);
	}
	if (RestartAction)
	{
		EnhancedInput->BindAction(RestartAction, ETriggerEvent::Started, this, &AUSPlayerController::OnRestartKey);
	}
	if (ToHangarAction)
	{
		EnhancedInput->BindAction(ToHangarAction, ETriggerEvent::Started, this, &AUSPlayerController::OnHangarKey);
	}
}

void AUSPlayerController::AddUIMappingContext()
{
	if (UIMappingContext == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: UIMappingContext is not set; pause and game over keys will not work"), *GetName());
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (Subsystem)
	{
		// 기체의 IMC_Default(0)보다 높게 — 키가 겹치지 않지만 판 진행 키가 먼저 처리되게
		Subsystem->AddMappingContext(UIMappingContext, 1);
	}
}

void AUSPlayerController::HandlePlayerDeath()
{
	if (bGameOver)
	{
		return;
	}

	EndRun(EUSRunEndReason::Destroyed);
	bShowMouseCursor = true;

	// 바로 멈추면 대장 폭발이 한 프레임에서 굳으므로, 폭발을 보여 준 뒤 창을 띄우고 멈춘다
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	World->GetTimerManager().SetTimer(GameOverTimerHandle, this, &AUSPlayerController::ShowGameOver,
		FMath::Max(GameOverDelay, KINDA_SMALL_NUMBER), false);
}

void AUSPlayerController::ShowGameOver()
{
	if (!IsLocalController() || GameOverWidget != nullptr)
	{
		return;
	}

	// 스크랩 적립은 위젯과 상관없이 해야 하므로 위젯 검사보다 먼저 한다
	const FUSRunResult Result = BuildRunResult();
	FUSRunRecords Records = RecordRunOnce();

	// 위젯이 없어도 R/Enter 재시작은 된다. 보이는 안내만 빠지므로 알린다
	if (GameOverWidgetClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] GameOverWidgetClass is not set. Only R/Enter restarts."), *GetName());
		return;
	}

	GameOverWidget = CreateWidget<UUSGameOverWidget>(this, GameOverWidgetClass);
	if (GameOverWidget == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] Failed to create game over widget (%s)."), *GetName(), *GetNameSafe(GameOverWidgetClass));
		return;
	}

	GameOverWidget->OnHangarRequested.BindUObject(this, &AUSPlayerController::HandleHangarRequested);
	GameOverWidget->OnTitleRequested.BindUObject(this, &AUSPlayerController::HandleTitleRequested);
	GameOverWidget->OnRetryRequested.BindUObject(this, &AUSPlayerController::HandleRetryRequested);
	GameOverWidget->AddToViewport(1);
	GameOverWidget->SetupResult(Result, Records);
	if (RunEndReason == EUSRunEndReason::Returned && StagesClearedThisRun > 0 && !bPlanetCleared && CurrentStage)
	{
		GameOverWidget->SetStageClearTitle(CurrentStage->GetStageNumber());
	}

	// UIOnly는 컨트롤러 키 입력(R/Enter)을 막으므로 게임+UI 모드를 유지한 채 멈춘다
	ApplyGameplayInputMode();
	SetPause(true);
}

FUSRunResult AUSPlayerController::BuildRunResult() const
{
	FUSRunResult Result;
	Result.EndReason = RunEndReason;
	// 스테이지를 하나라도 깼으면 클리어 (다음 스테이지로 이어 가다 격추돼도)
	Result.bCleared = bCleared || StagesClearedThisRun > 0;
	Result.StagesCleared = StagesClearedThisRun;
	Result.StageNumber = CurrentStage ? CurrentStage->GetStageNumber() : 0;
	Result.bPlanetCleared = bPlanetCleared;
	// 클리어 창은 판이 끝나기 전에 "지금까지"를 보여 주므로 끝난 시각 대신 지금 시각을 쓴다
	const float EndTime = bGameOver ? RunEndTime : (GetWorld() ? GetWorld()->GetTimeSeconds() : RunEndTime);
	Result.SurvivedSeconds = FMath::Max(0.0f, EndTime - RunStartTime);
	Result.ContinueSeconds = bContinuing ? FMath::Max(0.0f, EndTime - ContinueStartTime) : 0.0f;
	Result.Level = PawnExperience.IsValid() ? PawnExperience->GetCurrentLevel() : 0;

	if (const UUSRunStatsSubsystem* RunStats = GetWorld() ? GetWorld()->GetSubsystem<UUSRunStatsSubsystem>() : nullptr)
	{
		Result.KillCount = RunStats->GetKillCount();
	}

	if (ScrapRewardData)
	{
		const UUSMetaProgressionSubsystem* Meta = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr;
		const float ScrapMultiplier = Meta ? Meta->GetMultiplier(MetaTree, EUSMetaEffect::ScrapGain) : 1.0f;
		ScrapRewardData->CalculateScrap(Result, ScrapMultiplier);
	}
	else
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] ScrapRewardData is not set. Run summary shows 0 scrap."), *GetName());
	}

	return Result;
}

FUSRunRecords AUSPlayerController::ReadPreviousRecords() const
{
	FUSRunRecords Records;
	if (const UUSMetaProgressionSubsystem* Meta = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr)
	{
		Records.bValid = true;
		Records.PreviousBestSeconds = Meta->GetBestSurvivedSeconds();
		Records.PreviousBestKills = Meta->GetBestKillCount();
	}
	return Records;
}

void AUSPlayerController::ApplyMetaProgression(APawn* InPawn)
{
	if (bMetaApplied || InPawn == nullptr)
	{
		return;
	}

	if (MetaTree == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] MetaTree is not set. Permanent upgrades are not applied."), *GetName());
		return;
	}

	UUSMetaProgressionSubsystem* Meta = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr;
	if (Meta == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] No meta progression subsystem. Permanent upgrades are not applied."), *GetName());
		return;
	}

	bMetaApplied = true;
	Meta->ApplyToRun(MetaTree, InPawn);
}

FUSRunRecords AUSPlayerController::RecordRunOnce()
{
	const FUSRunResult Result = BuildRunResult();
	// 판마다 한 줄이라 결산 요약만 Log로 둔다. 30초 간격·출격 시간 줄은 Verbose (`log LogUS Verbose`로 켠다)
	UE_LOG(LogUS, Log, TEXT("[Balance] run end: %s%s, %.0fs, kills %d, Lv %d, scrap kill %d + time %d + clear %d + continue %d = %d"),
		*StaticEnum<EUSRunEndReason>()->GetNameStringByValue(static_cast<int64>(Result.EndReason)), Result.bCleared ? TEXT(" (cleared)") : TEXT(""),
		Result.SurvivedSeconds, Result.KillCount, Result.Level,
		Result.KillScrap, Result.TimeScrap, Result.ClearScrap, Result.ContinueScrap, Result.TotalScrap);

	// 최고 기록은 RecordRun이 이번 판으로 덮어쓰기 전에 읽어야 "갱신"을 가릴 수 있다
	FUSRunRecords Records = ReadPreviousRecords();
	if (UUSMetaProgressionSubsystem* Meta = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr)
	{
		if (!bRunRecorded)
		{
			bRunRecorded = true;
			Meta->RecordRun(Result);
		}

		Records.RunCount = Meta->GetRunCount();
		Records.ClearCount = Meta->GetClearCount();
	}
	return Records;
}

void AUSPlayerController::EndRun(EUSRunEndReason Reason)
{
	bGameOver = true;
	RunEndReason = Reason;

	if (UWorld* World = GetWorld())
	{
		RunEndTime = World->GetTimeSeconds();
		World->GetTimerManager().ClearTimer(RunClockTimerHandle);
	}
}

// ---------------- 런 시계·클리어 ----------------

void AUSPlayerController::RefreshRunClock()
{
	const UWorld* World = GetWorld();
	if (bGameOver || World == nullptr)
	{
		return;
	}

	InitializeRunDuration();

	const float Now = World->GetTimeSeconds();
	// 시계·클리어 시각은 스테이지 기준 ("다음 스테이지"마다 0부터)
	const float Elapsed = Now - StageStartTime;

	if (BalanceLogInterval > 0.0f && Elapsed >= NextBalanceLogSeconds)
	{
		LogBalanceSnapshot(Elapsed);
		NextBalanceLogSeconds += BalanceLogInterval;
	}

	if (bContinuing)
	{
		if (HUDWidget)
		{
			HUDWidget->SetRunTime(Now - ContinueStartTime, true);
		}
		return;
	}

	if (HUDWidget)
	{
		// 보스전 중에는 시계가 0:00에 멈춰 있어 "시간이 다 됐다"로 읽힌다 → 보스 등장 문구로 바꾼다
		if (bBossFight)
		{
			HUDWidget->ShowBossFightTime();
		}
		else
		{
			HUDWidget->SetRunTime(FMath::Max(0.0f, GetClockTargetSeconds() - Elapsed), false);
		}
	}

	// 월드 타이머라 카드·상점 창이 떠 있는 동안에는 이 함수가 불리지 않지만, 혹시 겹치면 다음 초에 처리한다.
	// 보스전 중(호출 경고 포함)에는 시계가 멈추고 격파(HandleBossDeath)가 클리어를 연다 — 출격 시간이 끝나도 귀환시키지 않는다
	if (bModalOpen || bCleared || bBossFight)
	{
		return;
	}

	RefreshBossApproach(Elapsed);

	// 출격 시간과 클리어 시각이 같으면 클리어(보스 신호)가 먼저다 (끝까지 버틴 판을 귀환으로 끝내지 않는다)
	if (!bBossSignal && ClearTimeSeconds > 0.0f && Elapsed >= ClearTimeSeconds)
	{
		// 보스 스테이지는 1:30에 바로 경고 → 보스 (B로 부르기를 기다리면 "보스가 너무 늦다", 사용자 피드백 2026-10-26)
		const AUSEnemySpawner* Spawner = EnemySpawner.Get();
		if (Spawner && Spawner->HasBoss())
		{
			BeginBossSummon();
		}
		else
		{
			ShowClear();
		}
		return;
	}

	if (HasRunTimeLimit() && Elapsed >= RunDurationSeconds)
	{
		HandleRunTimeUp();
	}
}

void AUSPlayerController::InitializeRunDuration()
{
	if (bRunDurationInitialized)
	{
		return;
	}
	bRunDurationInitialized = true;

	// 판 시작 순간(t=0)은 스포너가 돌기 전이라 찍을 것이 없으므로 첫 로그는 한 간격 뒤
	NextBalanceLogSeconds = BalanceLogInterval;

	RunDurationSeconds = BaseRunDurationSeconds;
	if (BaseRunDurationSeconds <= 0.0f)
	{
		return;
	}

	if (const UUSMetaProgressionSubsystem* Meta = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr)
	{
		RunDurationSeconds += Meta->GetAdditiveTotal(MetaTree, EUSMetaEffect::RunDuration);
	}

	UE_LOG(LogUS, Verbose, TEXT("[Balance] run duration %.0fs (base %.0fs), clear at %.0fs"),
		RunDurationSeconds, BaseRunDurationSeconds, ClearTimeSeconds);
}

void AUSPlayerController::InitializeChainExplosion()
{
	UUSChainExplosionSubsystem* Chain = GetWorld() ? GetWorld()->GetSubsystem<UUSChainExplosionSubsystem>() : nullptr;
	if (Chain == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: no ChainExplosionSubsystem; chain explosions disabled"), *GetName());
		return;
	}

	// 격납고 연쇄 확률·폭발 범위 칸은 트리에서 뺐다 (2026-11-02, 폭발탄 카드와 구분이 안 됨) → 기본값 그대로
	const FUSChainExplosionSettings& Settings = ChainExplosion;
	Chain->Configure(Settings);

	UE_LOG(LogUS, Verbose, TEXT("[Balance] chain explosion chance %.0f%%, radius %.0f, all explosions radius x%.2f"),
		Settings.Chance * 100.0f, Settings.Radius, Settings.RadiusMultiplier);
}

bool AUSPlayerController::HasRunTimeLimit() const
{
	return RunDurationSeconds > 0.0f;
}

bool AUSPlayerController::CanExtendRunTime() const
{
	if (!HasRunTimeLimit() || bCleared || bContinuing)
	{
		return false;
	}

	return ClearTimeSeconds <= 0.0f || RunDurationSeconds < ClearTimeSeconds;
}

float AUSPlayerController::GetClockTargetSeconds() const
{
	// 보스 신호 뒤에는 귀환까지 남은 시간을 보여 준다 (그 안에 B로 부를지 고른다)
	if (HasRunTimeLimit() && (bBossSignal || ClearTimeSeconds <= 0.0f || RunDurationSeconds < ClearTimeSeconds))
	{
		return RunDurationSeconds;
	}

	return ClearTimeSeconds;
}

void AUSPlayerController::HandleRunTimeExtended(float Seconds)
{
	if (!HasRunTimeLimit())
	{
		return;
	}

	RunDurationSeconds += Seconds;
	UE_LOG(LogUS, Verbose, TEXT("[Balance] run duration extended +%.0fs -> %.0fs"), Seconds, RunDurationSeconds);

	// 상점이 열린 채(일시정지)여도 HUD 남은 시간이 바로 늘어 보이게 한다
	RefreshRunClock();
}

void AUSPlayerController::HandleRunTimeUp()
{
	// 보스를 부르지 않고 출격 시간이 끝남 → 신호 문구를 거두고 보스 없이 귀환
	bBossSignal = false;
	if (BossHealthBarWidget)
	{
		BossHealthBarWidget->HideApproach();
	}

	// 살아서 돌아오므로 폭발 대기 없이 바로 결산을 띄운다 (클리어 창 "나가기"와 같은 귀환)
	EndRun(EUSRunEndReason::Returned);
	ShowGameOver();
}

void AUSPlayerController::LogBalanceSnapshot(float ElapsedSeconds) const
{
	const UWorld* World = GetWorld();
	const UUSRunStatsSubsystem* RunStats = World ? World->GetSubsystem<UUSRunStatsSubsystem>() : nullptr;
	const UUSTeamSubsystem* Team = World ? World->GetSubsystem<UUSTeamSubsystem>() : nullptr;
	const AUSEnemySpawner* Spawner = EnemySpawner.Get();

	UE_LOG(LogUS, Verbose, TEXT("[Balance] t=%.0fs kills %d, squad DPS %.0f (allies %.0f), enemy HP x%.2f, enemies %d, gold %d, Lv %d"),
		ElapsedSeconds,
		RunStats ? RunStats->GetKillCount() : 0,
		RunStats ? RunStats->GetSquadDps() : 0.0f,
		RunStats ? RunStats->GetAllyDps() : 0.0f,
		Spawner ? Spawner->GetHealthMultiplier() : 0.0f,
		Team ? Team->GetEnemies().Num() : 0,
		PawnCurrency.IsValid() ? PawnCurrency->GetGold() : 0,
		PawnExperience.IsValid() ? PawnExperience->GetCurrentLevel() : 0);
}

void AUSPlayerController::ShowClear()
{
	bCleared = true;

	// 깬 스테이지는 바로 저장한다 (이어 가다 격추되거나 게임을 꺼도 지도에 남게)
	UUSMetaProgressionSubsystem* Meta = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr;
	if (CurrentStage)
	{
		++StagesClearedThisRun;
		bPlanetCleared = bPlanetCleared || CurrentStage->IsBossStage();
		if (Meta)
		{
			Meta->MarkStageCleared(CurrentStage);
		}
	}

	// 보통 스테이지: 고를 것이 없다 (판마다 처음부터) → 연출 뒤 바로 결산
	if (CurrentStage && !CurrentStage->IsBossStage())
	{
		BeginStageOutro();
		return;
	}

	// 위젯이 없으면 고를 방법이 없으므로 계속 가기로 넘어간다 (클리어 보너스는 결산에 들어간다)
	if (ClearWidgetClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] ClearWidgetClass is not set. Continuing automatically."), *GetName());
		HandleContinueRequested();
		return;
	}

	ClearWidget = CreateWidget<UUSClearWidget>(this, ClearWidgetClass);
	if (ClearWidget == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] Failed to create clear widget (%s). Continuing automatically."),
			*GetName(), *GetNameSafe(ClearWidgetClass));
		HandleContinueRequested();
		return;
	}

	ClearWidget->OnLeaveRequested.BindUObject(this, &AUSPlayerController::HandleLeaveRequested);
	ClearWidget->OnContinueRequested.BindUObject(this, &AUSPlayerController::HandleContinueRequested);
	ClearWidget->AddToViewport(1);

	// 이 결과는 보여 주기만 한다. 기록(RecordRun)은 판이 정말 끝난 결산 때 한 번만 한다
	const FUSRunResult Result = BuildRunResult();
	const FUSRunRecords Records = ReadPreviousRecords();
	if (ScrapRewardData)
	{
		ClearWidget->SetupClear(Result, Records, ScrapRewardData->GetContinueBonusPerStep(), ScrapRewardData->GetContinueStepSeconds());
	}
	else
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] ScrapRewardData is not set. Clear rewards show 0."), *GetName());
		ClearWidget->SetupClear(Result, Records, 0, 0.0f);
	}

	EnterModalUI();
}

void AUSPlayerController::DebugStartBossFight()
{
	if (bBossFight || bCleared || bGameOver)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] US.Test.Boss: boss fight already started or run is over."), *GetName());
		return;
	}

	const AUSEnemySpawner* Spawner = EnemySpawner.Get();
	if (Spawner == nullptr || !Spawner->HasBoss())
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] US.Test.Boss: no boss on the spawner."), *GetName());
		return;
	}

	// 처음엔 5:00을 건너뛰고 신호부터 (B로 부르는 흐름 확인), 신호가 떠 있으면 B 대신 바로 호출
	if (bBossSignal)
	{
		BeginBossSummon();
	}
	else
	{
		ShowBossSignal();
	}
}

void AUSPlayerController::ShowBossSignal()
{
	bBossSignal = true;

	// 신호 직후 귀환하면 고를 틈이 없다 → 최소 BossSignalGraceSeconds는 남긴다
	if (HasRunTimeLimit() && ClearTimeSeconds > 0.0f)
	{
		RunDurationSeconds = FMath::Max(RunDurationSeconds, ClearTimeSeconds + BossSignalGraceSeconds);
	}

	const AUSEnemySpawner* Spawner = EnemySpawner.Get();
	const UUSEnemyTypeData* BossType = Spawner ? Spawner->GetBossEnemyType() : nullptr;
	if (BossHealthBarWidget)
	{
		BossHealthBarWidget->ShowSignal(BossType ? BossType->GetDisplayName() : FText::GetEmpty());
	}

	// 시계가 바로 "귀환까지 남은 시간"으로 바뀌게
	RefreshRunClock();
}

void AUSPlayerController::OnCallBossKey()
{
	if (!bBossSignal || bBossFight || bModalOpen || bCleared || bGameOver)
	{
		return;
	}

	BeginBossSummon();
}

void AUSPlayerController::BeginBossSummon()
{
	bBossSignal = false;
	bBossFight = true;

	if (HUDWidget)
	{
		HUDWidget->ShowBossFightTime();
	}

	// 경고(띠·경고음·테두리)를 먼저 보여 주고 끝날 때 보스가 들어온다 (다라이어스식 WARNING → 보스).
	// 아직 보스가 없어 방향 화살표는 없다 — 스폰 뒤에는 보스 표시기가 가리킨다
	if (BossHealthBarWidget)
	{
		BossHealthBarWidget->HideApproach();
		BossHealthBarWidget->ShowWarning(nullptr);
	}

	if (BossSummonWarningSeconds <= 0.0f)
	{
		FinishBossSummon();
		return;
	}
	GetWorldTimerManager().SetTimer(BossSummonTimerHandle, this, &AUSPlayerController::FinishBossSummon, BossSummonWarningSeconds, false);
}

void AUSPlayerController::FinishBossSummon()
{
	if (bGameOver)
	{
		return;
	}

	if (!StartBossFight())
	{
		bBossFight = false;
		ShowClear();
	}
}

bool AUSPlayerController::StartBossFight()
{
	AUSEnemySpawner* Spawner = EnemySpawner.Get();
	if (Spawner == nullptr || !Spawner->HasBoss())
	{
		return false;
	}

	AUSBossEnemyCharacter* Boss = Spawner->StartBossPhase();
	if (Boss == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] Boss failed to spawn. Clearing without a boss."), *GetName());
		return false;
	}

	bBossFight = true;
	ActiveBoss = Boss;
	Boss->OnEnemyDeath.AddUObject(this, &AUSPlayerController::HandleBossDeath);

	// 체력을 지금 편대 화력에 맞춘다 (체력바·이름표가 읽기 전에)
	const UUSRunStatsSubsystem* RunStats = GetWorld() ? GetWorld()->GetSubsystem<UUSRunStatsSubsystem>() : nullptr;
	if (BossFightSeconds > 0.0f && RunStats && Boss->GetAttributeComponent())
	{
		const float SquadDps = RunStats->GetSquadDps();
		const float BossHealth = FMath::Max(BossMinHealth, SquadDps * BossFightSeconds);
		Boss->GetAttributeComponent()->SetMaxHealth(BossHealth);
		UE_LOG(LogUS, Log, TEXT("[%s] Boss health %.0f (squad DPS %.0f x %.0fs, min %.0f)"), *GetName(), BossHealth, SquadDps, BossFightSeconds, BossMinHealth);
	}

	if (RadarWidget)
	{
		RadarWidget->SetBoss(Boss);
	}

	if (BossIndicatorWidget == nullptr && BossIndicatorWidgetClass)
	{
		BossIndicatorWidget = CreateWidget<UUSOffscreenIndicatorWidget>(this, BossIndicatorWidgetClass);
		if (BossIndicatorWidget)
		{
			// 보급선 표시기와 같은 층(HUD 아래)
			BossIndicatorWidget->AddToViewport(-1);
		}
	}
	if (BossIndicatorWidget)
	{
		BossIndicatorWidget->SetTarget(Boss);
	}

	if (BossHealthBarWidget)
	{
		const UUSEnemyTypeData* BossType = Spawner->GetBossEnemyType();
		BossHealthBarWidget->HideApproach();
		BossHealthBarWidget->SetBoss(Boss, BossType ? BossType->GetDisplayName() : FText::GetEmpty());
	}
	return true;
}

void AUSPlayerController::RefreshBossApproach(float Elapsed)
{
	const AUSEnemySpawner* Spawner = EnemySpawner.Get();
	if (BossHealthBarWidget == nullptr || Spawner == nullptr || !Spawner->HasBoss() || ClearTimeSeconds <= 0.0f || BossApproachLeadSeconds <= 0.0f)
	{
		return;
	}

	if (bBossSignal)
	{
		return;
	}

	// 출격 시간이 먼저 끝나 귀환할 판이면 보스는 안 나오므로 예고하지 않는다 (보급선에서 시간을 사면 그때부터 뜬다)
	const float SecondsLeft = ClearTimeSeconds - Elapsed;
	const bool bBossComing = !HasRunTimeLimit() || RunDurationSeconds >= ClearTimeSeconds;
	if (bBossComing && SecondsLeft > 0.0f && SecondsLeft <= BossApproachLeadSeconds)
	{
		const UUSEnemyTypeData* BossType = Spawner->GetBossEnemyType();
		BossHealthBarWidget->ShowApproach(BossType ? BossType->GetDisplayName() : FText::GetEmpty(), SecondsLeft);
	}
	else
	{
		BossHealthBarWidget->HideApproach();
	}
}

void AUSPlayerController::HandleBossDeath(AUSEnemyCharacter* Enemy)
{
	if (AUSBossEnemyCharacter* Boss = ActiveBoss.Get())
	{
		Boss->OnEnemyDeath.RemoveAll(this);
	}
	ActiveBoss.Reset();

	if (BossHealthBarWidget)
	{
		BossHealthBarWidget->HideAll();
	}
	if (RadarWidget)
	{
		RadarWidget->SetBoss(nullptr);
	}
	if (BossIndicatorWidget)
	{
		BossIndicatorWidget->SetTarget(nullptr);
	}

	// 대장이 같은 순간 격추됐으면 결산이 먼저다
	UWorld* World = GetWorld();
	if (bGameOver || World == nullptr)
	{
		bBossFight = false;
		return;
	}

	if (AUSEnemySpawner* Spawner = EnemySpawner.Get())
	{
		Spawner->EndBossPhase();
	}

	// 막타의 히트스톱이 곧 시간 배율을 1로 되돌리므로 먼저 끄고 슬로모션을 건다.
	// 타이머는 늦춰진 게임 시간으로 흐르므로 실제 길이 × 배율로 건다 (히트스톱과 같은 방식)
	World->GetTimerManager().ClearTimer(HitStopTimerHandle);
	bBossDefeatSlowMo = true;
	UGameplayStatics::SetGlobalTimeDilation(this, BossDefeatDilation);
	World->GetTimerManager().SetTimer(BossDefeatTimerHandle, this, &AUSPlayerController::FinishBossDefeat,
		FMath::Max(BossDefeatSlowSeconds * BossDefeatDilation, KINDA_SMALL_NUMBER), false);
}

void AUSPlayerController::FinishBossDefeat()
{
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
	bBossDefeatSlowMo = false;
	bBossFight = false;

	if (bGameOver)
	{
		return;
	}

	// 슬로모션 사이 레벨업 카드가 떴으면 다 고른 뒤에 연다 (창 두 개를 겹치지 않는다)
	if (bModalOpen)
	{
		GetWorldTimerManager().SetTimer(BossDefeatTimerHandle, this, &AUSPlayerController::FinishBossDefeat, 0.25f, false);
		return;
	}

	ShowClear();

	const AUSEnemySpawner* Spawner = EnemySpawner.Get();
	const UUSEnemyTypeData* BossType = Spawner ? Spawner->GetBossEnemyType() : nullptr;
	if (ClearWidget && BossType)
	{
		ClearWidget->ShowDefeatedBoss(BossType->GetIcon(), BossType->GetDisplayName());
	}
}

void AUSPlayerController::SetupBossHealthBar()
{
	if (BossHealthBarWidget != nullptr)
	{
		return;
	}

	if (BossHealthBarWidgetClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] BossHealthBarWidgetClass is not set. Boss warning and health bar will not be shown."), *GetName());
		return;
	}

	BossHealthBarWidget = CreateWidget<UUSBossHealthBarWidget>(this, BossHealthBarWidgetClass);
	if (BossHealthBarWidget)
	{
		BossHealthBarWidget->AddToViewport();
	}
}

void AUSPlayerController::HandleLeaveRequested()
{
	RemoveClearWidget();
	bModalOpen = false;

	// 클리어 창이 이미 판 요약을 보여 줬으므로 결산 창을 또 띄우지 않고, 기록(스크랩 적립·저장)만 하고 격납고로 간다
	EndRun(EUSRunEndReason::Returned);
	RecordRunOnce();
	HandleHangarRequested();
}

void AUSPlayerController::HandleContinueRequested()
{
	RemoveClearWidget();

	bContinuing = true;
	if (const UWorld* World = GetWorld())
	{
		ContinueStartTime = World->GetTimeSeconds();
	}

	// 스포너는 맵에 배치된 한 대. 계속 가기는 한 번뿐이라 그때 찾는다
	if (AUSEnemySpawner* Spawner = Cast<AUSEnemySpawner>(UGameplayStatics::GetActorOfClass(this, AUSEnemySpawner::StaticClass())))
	{
		// 구간 길이의 출처는 스크랩 데이터 하나: 클리어 창 문구("30초마다")와 적이 강해지는 간격이 어긋나지 않게 한다
		Spawner->EnterContinueMode(ScrapRewardData ? ScrapRewardData->GetContinueStepSeconds() : 0.0f);
	}
	else
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] No enemy spawner in the level. Continue mode will not raise enemy health."), *GetName());
	}

	ExitModalUI();
	RefreshRunClock();
}

void AUSPlayerController::RemoveClearWidget()
{
	if (ClearWidget)
	{
		ClearWidget->RemoveFromParent();
		ClearWidget = nullptr;
	}
}

// ---------------- 일시정지 ----------------

void AUSPlayerController::Input_Pause()
{
	if (bGameOver)
	{
		return;
	}

	if (PauseWidget)
	{
		PauseWidget->HandleBackRequested();
		return;
	}

	// 레벨업 카드·상점·클리어 창은 각자 골라야 닫히는 창이라 그 위에 일시정지 창을 겹치지 않는다
	if (bModalOpen)
	{
		return;
	}

	OpenPauseMenu();
}

void AUSPlayerController::OpenPauseMenu()
{
	if (!IsLocalController())
	{
		return;
	}

	if (PauseWidgetClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] PauseWidgetClass is not set. Esc/P does nothing."), *GetName());
		return;
	}

	PauseWidget = CreateWidget<UUSPauseWidget>(this, PauseWidgetClass);
	if (PauseWidget == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] Failed to create pause widget (%s)."), *GetName(), *GetNameSafe(PauseWidgetClass));
		return;
	}

	PauseWidget->OnResumeRequested.BindUObject(this, &AUSPlayerController::HandleResumeRequested);
	PauseWidget->OnAbandonRequested.BindUObject(this, &AUSPlayerController::HandleAbandonRequested);
	PauseWidget->OnQuitRequested.BindUObject(this, &AUSPlayerController::HandleQuitRequested);
	PauseWidget->AddToViewport(2);

	// "이번 출격" 칸: 멈춘 동안 값이 바뀌지 않으므로 열 때 한 번만 채운다
	FUSPauseSnapshot Snapshot;
	if (const UWorld* World = GetWorld())
	{
		const float Now = World->GetTimeSeconds();
		Snapshot.bContinuing = bContinuing;
		Snapshot.ClockSeconds = bContinuing ? Now - ContinueStartTime : FMath::Max(0.0f, GetClockTargetSeconds() - (Now - StageStartTime));
		if (const UUSRunStatsSubsystem* RunStats = World->GetSubsystem<UUSRunStatsSubsystem>())
		{
			Snapshot.KillCount = RunStats->GetKillCount();
		}
	}
	Snapshot.Level = PawnExperience.IsValid() ? PawnExperience->GetCurrentLevel() : 0;
	Snapshot.Gold = PawnCurrency.IsValid() ? PawnCurrency->GetGold() : 0;
	if (const UUSSquadComponent* Squad = GetPawnSquad())
	{
		Snapshot.AllyCount = Squad->GetMemberCount();
		Snapshot.MaxAllies = Squad->GetMaxMembers();
	}
	if (const AUSEnemySpawner* Spawner = EnemySpawner.Get())
	{
		Snapshot.MetEnemies = Spawner->GetSpawnedEnemyTypes();
	}
	PauseWidget->ShowSnapshot(Snapshot);

	// EnterModalUI의 UIOnly는 컨트롤러 키(Esc/P로 닫기)를 막으므로, 결산 창처럼 게임+UI 모드를 유지한 채 멈춘다
	bModalOpen = true;
	FlushPressedKeys(); // 메뉴에 포커스가 가면 키 뗌이 메뉴로 갈 수 있다 (EnterModalUI와 같은 이유)
	ApplyGameplayInputMode();
	SetPause(true);

	// ↑/↓·Enter가 메뉴에 가도록 포커스를 준다. 메뉴가 안 쓰는 키(Esc/P)는 그대로 컨트롤러 입력으로 넘어간다
	PauseWidget->SetKeyboardFocus();
}

void AUSPlayerController::HandleApplicationActivationChanged(bool bIsActive)
{
	// 다른 모달은 이미 멈춰 있고 각자 골라야 닫히므로 겹치지 않는다 (Input_Pause와 같은 조건)
	if (bIsActive || bGameOver || PauseWidget != nullptr || bModalOpen)
	{
		return;
	}

	OpenPauseMenu();
}

void AUSPlayerController::RemovePauseWidget()
{
	if (PauseWidget)
	{
		PauseWidget->RemoveFromParent();
		PauseWidget = nullptr;
	}
}

void AUSPlayerController::HandleResumeRequested()
{
	RemovePauseWidget();
	ExitModalUI();
}

void AUSPlayerController::HandleAbandonRequested()
{
	RemovePauseWidget();
	bModalOpen = false;

	// 이미 멈춘 상태라 월드 시간도 멈춰 있어 생존 시간은 창을 연 순간까지다. 결산 창이 계속 멈춰 둔다
	EndRun(EUSRunEndReason::Abandoned);
	ShowGameOver();
}

void AUSPlayerController::HandleQuitRequested()
{
	RemovePauseWidget();
	bModalOpen = false;

	// 출격 포기와 같은 결산 기록(스크랩 적립·저장)을 하고, 결산 창 대신 바로 끈다. 저장은 SaveGameToSlot이라 끄기 전에 끝난다
	EndRun(EUSRunEndReason::Abandoned);
	RecordRunOnce();
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void AUSPlayerController::OnRestartKey()
{
	if (!bGameOver)
	{
		return;
	}

	HandleRetryRequested();
}

void AUSPlayerController::OnHangarKey()
{
	if (!bGameOver)
	{
		return;
	}

	HandleHangarRequested();
}

void AUSPlayerController::HandleHangarRequested()
{
	if (HangarMap.IsNull())
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] HangarMap is not set. Restarting the current level instead."), *GetName());
		HandleRestartRequested();
		return;
	}

	// 행성을 깨는 중이면 격납고 "출격"이 그 행성 지역 지도로 바로 간다 (강화 먼저, 다시 출발은 짧게)
	if (UUSMetaProgressionSubsystem* Meta = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr)
	{
		Meta->SetReturnToStageMap(StagesClearedThisRun > 0 && !bPlanetCleared && CurrentStage != nullptr);
	}

	UUSLevelTravelSubsystem::TravelFrom(this, HangarMap);
}

void AUSPlayerController::HandleRetryRequested()
{
	// 재도전 = 끝난 그 스테이지부터. "다음 스테이지"로 이어 가면 CurrentStage만 바뀌고 메타의 출격 스테이지는 처음 고른 곳이라,
	// 맵만 다시 열면 첫 스테이지로 돌아간다 → 출격 스테이지를 지금 스테이지로 바꾼 뒤 연다 (편대 편성은 메타에 그대로 남는다)
	if (UUSMetaProgressionSubsystem* Meta = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr)
	{
		if (CurrentZone && CurrentStage)
		{
			Meta->SetSortie(CurrentZone, CurrentStage);
		}
	}

	HandleRestartRequested();
}

void AUSPlayerController::HandleRestartRequested()
{
	const FName LevelName(*UGameplayStatics::GetCurrentLevelName(this, true));
	UUSLevelTravelSubsystem::TravelByNameFrom(this, LevelName);
}

void AUSPlayerController::HandleTitleRequested()
{
	if (TitleMap.IsNull())
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] TitleMap is not set. Restarting the current level instead."), *GetName());
		HandleRestartRequested();
		return;
	}

	UUSLevelTravelSubsystem::TravelFrom(this, TitleMap);
}

// ---------------- 조작 안내 ----------------

void AUSPlayerController::BeginRunIntro()
{
	ShowControlsGuideIfFirstRun();

	// 조작 안내가 떴으면 닫을 때(HandleControlsGuideDismissed) 카드를 준다
	if (ControlsGuideWidget == nullptr)
	{
		GrantStartingCards();
	}
}

void AUSPlayerController::GrantStartingCards()
{
	if (bStartingCardsGranted || bGameOver)
	{
		return;
	}
	bStartingCardsGranted = true;

	const UUSMetaProgressionSubsystem* Meta = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr;
	const int32 Cards = Meta && MetaTree ? FMath::RoundToInt(Meta->GetAdditiveTotal(MetaTree, EUSMetaEffect::StartingCard)) : 0;
	if (Cards <= 0)
	{
		return;
	}

	UE_LOG(LogUS, Log, TEXT("[%s] Starting cards: %d"), *GetName(), Cards);
	PendingCardSelections += Cards;
	if (!bModalOpen)
	{
		TryShowNextLevelUpCard();
	}
}

void AUSPlayerController::ApplyStageToSpawner()
{
	AUSEnemySpawner* Spawner = EnemySpawner.Get();
	if (CurrentStage == nullptr || CurrentZone == nullptr)
	{
		return;
	}
	if (Spawner == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] No enemy spawner in the level. Stage %s settings are not applied."), *GetName(), *GetNameSafe(CurrentStage));
		return;
	}

	// 체력 = 스테이지 번호 공식 × 벽 덤 (4·7번 계단)
	const int32 Number = CurrentStage->GetStageNumber();
	const float HealthMultiplier = CurrentZone->GetStageHealthMultiplier(Number) * CurrentStage->GetHealthBonus();
	Spawner->ApplyStage(CurrentStage->GetSpawnTable(), HealthMultiplier, CurrentZone->GetStageAmountMultiplier(Number), CurrentStage->IsBossStage());

	UE_LOG(LogUS, Log, TEXT("[%s] Stage %d %s: HP x%.2f (wall x%.2f)"), *GetName(), Number, *CurrentStage->GetDisplayName().ToString(), HealthMultiplier, CurrentStage->GetHealthBonus());
}

void AUSPlayerController::BeginStageOutro()
{
	// 살아서 돌아가는 판 끝: 시계·생존 시간을 여기서 멈춘다 (연출 시간은 생존 시간에 넣지 않는다). 게임오버 상태라 피격·레벨업 카드·상점도 막힌다
	EndRun(EUSRunEndReason::Returned);

	// 화면에 남은 적은 한꺼번에 터뜨린다 (처치 수·스크랩은 평소 처치처럼 들어간다). 보상은 대장에게 빨려 온다 (Brotato 웨이브 끝)
	if (AUSEnemySpawner* Spawner = EnemySpawner.Get())
	{
		Spawner->StopSpawning();
		Spawner->ClearAllEnemies();
	}
	AttractAllPickups();

	if (StageBannerWidget && CurrentStage)
	{
		StageBannerWidget->ShowStageClear(CurrentStage->GetStageNumber(), StageClearBannerSeconds);
	}

	GetWorldTimerManager().SetTimer(StageOutroTimerHandle, this, &AUSPlayerController::StartStageDeparture,
		FMath::Max(StageClearBannerSeconds, KINDA_SMALL_NUMBER), false);
}

void AUSPlayerController::StartStageDeparture()
{
	// 격추 폭발로 띠가 떠 있는 동안 새로 떨어진 보상도 끌어온다
	AttractAllPickups();

	AUSPlayerCharacter* Leader = Cast<AUSPlayerCharacter>(GetPawn());
	if (StageTransitionComponent == nullptr || Leader == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] No transition component or leader. Showing the result without departure."), *GetName());
		HandleDepartureFinished();
		return;
	}

	StageTransitionComponent->PlayDeparture(Leader, FDelegateOnTransitionFinished::CreateUObject(this, &AUSPlayerController::HandleDepartureFinished));
}

void AUSPlayerController::HandleDepartureFinished()
{
	ShowGameOver();
}

void AUSPlayerController::BeginStageArrival()
{
	AUSPlayerCharacter* Leader = Cast<AUSPlayerCharacter>(GetPawn());
	if (CurrentStage == nullptr || StageTransitionComponent == nullptr || Leader == nullptr || bGameOver)
	{
		BeginRunIntro();
		return;
	}

	if (StageBannerWidget)
	{
		StageBannerWidget->ShowStageTitle(CurrentStage->GetStageNumber(), CurrentStage->GetDisplayName(), CurrentStage->IsWallStage(),
			StageTransitionComponent->GetArrivalTotalSeconds() + StageTitleSeconds);
	}

	StageTransitionComponent->PlayArrival(Leader, FDelegateOnTransitionFinished::CreateUObject(this, &AUSPlayerController::BeginRunIntro));
}

void AUSPlayerController::AttractAllPickups() const
{
	// 클리어 순간 두 번만 도는 검색이라 매 프레임 검색 금지 규칙과 부딪히지 않는다
	for (TActorIterator<AUSPickup> It(GetWorld()); It; ++It)
	{
		It->StartAutoCollect();
	}
}

void AUSPlayerController::SetupStageBanner()
{
	if (StageBannerWidget != nullptr)
	{
		return;
	}

	StageBannerWidget = CreateWidget<UUSStageBannerWidget>(this, UUSStageBannerWidget::StaticClass());
	if (StageBannerWidget)
	{
		// 결산 창(1)보다 아래, HUD(0)와 같은 층
		StageBannerWidget->AddToViewport();
	}
	if (StageTransitionComponent)
	{
		StageTransitionComponent->SetBanner(StageBannerWidget);
	}
}

void AUSPlayerController::DebugClearStage()
{
	const UWorld* World = GetWorld();
	if (World == nullptr || bCleared || bBossFight || bGameOver || ClearTimeSeconds <= 0.0f)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] US.Test.ClearStage: stage already cleared, boss fight or run over."), *GetName());
		return;
	}

	StageStartTime = World->GetTimeSeconds() - ClearTimeSeconds;
	RefreshRunClock();
}

void AUSPlayerController::ShowControlsGuideIfFirstRun()
{
	if (!IsLocalController() || ControlsGuideWidget != nullptr || bModalOpen || bGameOver)
	{
		return;
	}

	// 기록된 판(사망·클리어·포기 모두 결산 때 1 증가)이 없을 때만 = 정말 처음 해 보는 사람
	const UUSMetaProgressionSubsystem* Meta = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr;
	const bool bFirstRun = Meta == nullptr || Meta->GetRunCount() == 0;
	if (!bFirstRun && !bAlwaysShowControlsGuide)
	{
		return;
	}

	if (ControlsGuideWidgetClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] ControlsGuideWidgetClass is not set. First-run controls guide is skipped."), *GetName());
		return;
	}

	ControlsGuideWidget = CreateWidget<UUSControlsGuideWidget>(this, ControlsGuideWidgetClass);
	if (ControlsGuideWidget == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] Failed to create controls guide widget (%s)."), *GetName(), *GetNameSafe(ControlsGuideWidgetClass));
		return;
	}

	ControlsGuideWidget->OnDismissed.BindUObject(this, &AUSPlayerController::HandleControlsGuideDismissed);
	ControlsGuideWidget->AddToViewport(2);

	// UIOnly: 아무 키(Esc 포함)가 컨트롤러가 아니라 카드로 가게
	EnterModalUI();
	ControlsGuideWidget->SetKeyboardFocus();
}

void AUSPlayerController::HandleControlsGuideDismissed()
{
	if (ControlsGuideWidget)
	{
		ControlsGuideWidget->RemoveFromParent();
		ControlsGuideWidget = nullptr;
	}

	// 판 시계는 월드 시간이라 멈춘 동안은 흐르지 않는다 → 카드를 읽는 시간은 생존 시간에서 빠진다
	ExitModalUI();
	GrantStartingCards();
}

UUSSquadComponent* AUSPlayerController::GetPawnSquad() const
{
	const AUSPlayerCharacter* PlayerCharacter = Cast<AUSPlayerCharacter>(GetPawn());
	return PlayerCharacter ? PlayerCharacter->GetSquadComponent() : nullptr;
}

// ---------------- 모달 UI ----------------

void AUSPlayerController::EnterModalUI()
{
	bModalOpen = true;

	// UIOnly 동안 키를 떼면 그 신호는 UI가 가져가 게임 입력은 "눌린 채"로 남는다 → 창을 닫은 뒤 이동이 계속된다.
	// 창을 열 때 눌린 키를 비워 둔다 (Enhanced Input도 Completed를 받는다)
	FlushPressedKeys();

	bShowMouseCursor = true;
	SetInputMode(FInputModeUIOnly());
	SetPause(true);
}

void AUSPlayerController::ExitModalUI()
{
	bModalOpen = false;

	ApplyGameplayInputMode();
	SetPause(false);
}

void AUSPlayerController::ApplyGameplayInputMode()
{
	bShowMouseCursor = true;

	// GameOnly는 클릭 시 마우스를 캡처하며 커서를 숨기므로 GameAndUI를 쓰고, 캡처 중에도 커서를 유지한다.
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}

void AUSPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ActivationChangedHandle.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().OnApplicationActivationStateChanged().Remove(ActivationChangedHandle);
		ActivationChangedHandle.Reset();
	}

	if (UUSCombatFeedbackSubsystem* Feedback = GetWorld() ? GetWorld()->GetSubsystem<UUSCombatFeedbackSubsystem>() : nullptr)
	{
		Feedback->OnEnemyDamaged.RemoveAll(this);
		Feedback->OnPlayerKilledEnemy.RemoveAll(this);
	}

	if (AUSSupplyShip* Ship = SupplyShip.Get())
	{
		Ship->OnSupplyStateChanged.RemoveAll(this);
	}

	if (AUSEnemySpawner* Spawner = EnemySpawner.Get())
	{
		Spawner->OnHordeWarning.RemoveAll(this);
	}

	if (ShopComponent)
	{
		ShopComponent->OnRunTimeExtended.RemoveAll(this);
	}

	if (AUSBossEnemyCharacter* Boss = ActiveBoss.Get())
	{
		Boss->OnEnemyDeath.RemoveAll(this);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GameOverTimerHandle);
		World->GetTimerManager().ClearTimer(RunClockTimerHandle);
		World->GetTimerManager().ClearTimer(BossDefeatTimerHandle);
		World->GetTimerManager().ClearTimer(BossSummonTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AUSPlayerController::SetupSquadWidget(APawn* InPawn)
{
	if (!IsLocalController() || SquadWidget != nullptr || InPawn == nullptr)
	{
		return;
	}

	if (SquadWidgetClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] SquadWidgetClass is not set. Squad panel will not be shown."), *GetName());
		return;
	}

	const AUSPlayerCharacter* PlayerCharacter = Cast<AUSPlayerCharacter>(InPawn);
	if (PlayerCharacter == nullptr)
	{
		return;
	}

	SquadWidget = CreateWidget<UUSSquadWidget>(this, SquadWidgetClass);
	if (SquadWidget)
	{
		SquadWidget->AddToViewport();
		SquadWidget->InitializeForSquad(PlayerCharacter->GetSquadComponent(), PlayerCharacter->GetCurrencyComponent());
	}
}

void AUSPlayerController::SetupDamageNumbers()
{
	if (DamageNumberWidget != nullptr)
	{
		return;
	}

	if (DamageNumberWidgetClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] DamageNumberWidgetClass is not set. Damage numbers will not be shown."), *GetName());
		return;
	}

	DamageNumberWidget = CreateWidget<UUSDamageNumberWidget>(this, DamageNumberWidgetClass);
	if (DamageNumberWidget)
	{
		// HUD(Z 0)보다 아래에 깔아 체력 바·탄약 표시를 가리지 않게 한다
		DamageNumberWidget->AddToViewport(-1);
	}
}

void AUSPlayerController::SetupDamageVignette()
{
	if (DamageVignetteWidget != nullptr)
	{
		return;
	}

	if (DamageVignetteWidgetClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] DamageVignetteWidgetClass is not set. No red edge flash on player hit."), *GetName());
		return;
	}

	DamageVignetteWidget = CreateWidget<UUSDamageVignetteWidget>(this, DamageVignetteWidgetClass);
	if (DamageVignetteWidget)
	{
		// HUD 글자 위를 붉게 덮지 않도록 HUD(Z 0) 아래에 깐다
		DamageVignetteWidget->AddToViewport(-1);
	}
}

void AUSPlayerController::SetupSupplyShipUI()
{
	if (SupplyShip.IsValid())
	{
		return;
	}

	// 보급선은 맵에 배치된 한 대가 파괴되지 않고 숨었다 나타났다 하므로 한 번 찾아 두면 된다
	AUSSupplyShip* Ship = Cast<AUSSupplyShip>(UGameplayStatics::GetActorOfClass(this, AUSSupplyShip::StaticClass()));
	if (Ship == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] No supply ship in the level. Shop cannot be opened."), *GetName());
		return;
	}
	SupplyShip = Ship;

	if (SupplyIndicatorWidgetClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] SupplyIndicatorWidgetClass is not set. Supply ship direction will not be shown."), *GetName());
	}
	else
	{
		SupplyIndicatorWidget = CreateWidget<UUSOffscreenIndicatorWidget>(this, SupplyIndicatorWidgetClass);
		if (SupplyIndicatorWidget)
		{
			// 데미지 숫자와 같은 층(HUD 아래)에 둔다
			SupplyIndicatorWidget->AddToViewport(-1);
		}
	}

	if (SupplyNoticeWidgetClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] SupplyNoticeWidgetClass is not set. Supply ship arrival notices will not be shown."), *GetName());
	}
	else
	{
		SupplyNoticeWidget = CreateWidget<UUSSupplyNoticeWidget>(this, SupplyNoticeWidgetClass);
		if (SupplyNoticeWidget)
		{
			SupplyNoticeWidget->AddToViewport();
		}
	}

	Ship->OnSupplyStateChanged.AddUObject(this, &AUSPlayerController::HandleSupplyStateChanged);

	// 구독 전에 이미 상태가 정해졌을 수 있으니 지금 상태로 표시기를 한 번 맞춘다
	if (SupplyIndicatorWidget)
	{
		SupplyIndicatorWidget->SetTarget(Ship->IsAvailable() ? Ship : nullptr);
	}
}

void AUSPlayerController::SetupHordeNotice()
{
	if (HordeNoticeWidget != nullptr)
	{
		return;
	}

	// 스포너는 BeginPlay에서 찾아 둔다. SetupHUD가 BeginPlay보다 먼저(OnPossess) 불릴 수 있어 비었으면 여기서 찾는다
	if (!EnemySpawner.IsValid())
	{
		EnemySpawner = Cast<AUSEnemySpawner>(UGameplayStatics::GetActorOfClass(this, AUSEnemySpawner::StaticClass()));
	}

	AUSEnemySpawner* Spawner = EnemySpawner.Get();
	if (Spawner == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] No enemy spawner in the level. Horde warnings will not be shown."), *GetName());
		return;
	}

	if (HordeNoticeWidgetClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] HordeNoticeWidgetClass is not set. Horde warnings will not be shown."), *GetName());
		return;
	}

	HordeNoticeWidget = CreateWidget<UUSHordeNoticeWidget>(this, HordeNoticeWidgetClass);
	if (HordeNoticeWidget)
	{
		HordeNoticeWidget->AddToViewport();
		Spawner->OnHordeWarning.AddUObject(this, &AUSPlayerController::HandleHordeWarning);
	}
}

void AUSPlayerController::HandleHordeWarning(EUSHordeShape Shape, const FVector& HordeCenter, float SecondsUntilSpawn)
{
	if (HordeNoticeWidget)
	{
		HordeNoticeWidget->HandleHordeWarning(Shape, HordeCenter, SecondsUntilSpawn);
	}
}

void AUSPlayerController::SetupRadar()
{
	if (RadarWidget != nullptr)
	{
		return;
	}

	if (RadarWidgetClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] RadarWidgetClass is not set. Minimap will not be shown."), *GetName());
		return;
	}

	RadarWidget = CreateWidget<UUSRadarWidget>(this, RadarWidgetClass);
	if (RadarWidget)
	{
		RadarWidget->AddToViewport();
		// 보급선이 없는 맵이면 null이 들어가 보급선 점만 빠진다
		RadarWidget->SetSupplyShip(SupplyShip.Get());
	}
}

void AUSPlayerController::HandleSupplyStateChanged(EUSSupplyShipState NewState, float SecondsRemaining)
{
	// 떠나 있는 동안에는 가리킬 곳이 없으므로 화살표를 숨긴다
	if (SupplyIndicatorWidget)
	{
		AUSSupplyShip* Ship = SupplyShip.Get();
		SupplyIndicatorWidget->SetTarget(Ship && Ship->IsAvailable() ? Ship : nullptr);
	}

	if (SupplyNoticeWidget)
	{
		SupplyNoticeWidget->HandleSupplyStateChanged(NewState, SecondsRemaining);
	}
}

void AUSPlayerController::BindCombatFeedback()
{
	if (!IsLocalController())
	{
		return;
	}

	UUSCombatFeedbackSubsystem* Feedback = GetWorld() ? GetWorld()->GetSubsystem<UUSCombatFeedbackSubsystem>() : nullptr;
	if (Feedback == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] CombatFeedbackSubsystem not found. Hit feedback is disabled."), *GetName());
		return;
	}

	Feedback->OnEnemyDamaged.AddUObject(this, &AUSPlayerController::HandleEnemyDamaged);
	Feedback->OnPlayerKilledEnemy.AddUObject(this, &AUSPlayerController::HandlePlayerKilledEnemy);
}

void AUSPlayerController::HandleEnemyDamaged(const FVector& Location, float Damage, bool /*bByPlayer*/)
{
	const UUSGameUserSettings* Settings = UUSGameUserSettings::Get();
	if (DamageNumberWidget && (Settings == nullptr || Settings->IsDamageNumbersEnabled()))
	{
		DamageNumberWidget->ShowDamageNumber(Location, Damage);
	}
}

void AUSPlayerController::HandlePlayerKilledEnemy(const FVector& Location)
{
	PlayHitShake(KillShakeScale);
	PlayHitStop();
}

void AUSPlayerController::PlayHitStop()
{
	UWorld* World = GetWorld();
	if (World == nullptr || HitStopDuration <= 0.0f || IsPaused() || bBossDefeatSlowMo)
	{
		return;
	}

	// 실제 시간으로 잰다 — 게임 시간은 히트스톱 자체가 늦추므로 쿨다운 기준으로 못 쓴다
	const double Now = FPlatformTime::Seconds();
	if (Now - LastHitStopRealTime < HitStopCooldown)
	{
		return;
	}
	LastHitStopRealTime = Now;

	UGameplayStatics::SetGlobalTimeDilation(this, HitStopDilation);

	// 타이머는 늦춰진 게임 시간으로 흐르므로, 실제 HitStopDuration 뒤에 끝나게 배율을 곱해 건다
	World->GetTimerManager().SetTimer(HitStopTimerHandle, this, &AUSPlayerController::EndHitStop, HitStopDuration * HitStopDilation, false);
}

void AUSPlayerController::EndHitStop()
{
	UGameplayStatics::SetGlobalTimeDilation(this, 1.0f);
}

void AUSPlayerController::HandlePawnDamaged(float Damage, AController* InstigatedBy)
{
	PlayHitShake(HitShakeScale);

	if (DamageVignetteWidget)
	{
		DamageVignetteWidget->PlayFlash();
	}

	const double Now = FPlatformTime::Seconds();
	if (PlayerHitSound && Now - LastPlayerHitSoundRealTime >= PlayerHitSoundCooldown)
	{
		LastPlayerHitSoundRealTime = Now;
		UGameplayStatics::PlaySound2D(this, PlayerHitSound);
	}
}

void AUSPlayerController::PlayHitShake(float Scale)
{
	if (HitShakeClass == nullptr || Scale <= 0.0f)
	{
		return;
	}

	// 설정 창 "화면 흔들림" (멀미 방지)
	const UUSGameUserSettings* Settings = UUSGameUserSettings::Get();
	if (Settings && !Settings->IsScreenShakeEnabled())
	{
		return;
	}

	ClientStartCameraShake(HitShakeClass, Scale);
}
