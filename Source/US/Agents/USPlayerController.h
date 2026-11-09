#pragma once

#include "CoreMinimal.h"
#include "Core/USChainExplosionSubsystem.h"
#include "Datas/USScrapRewardData.h"
#include "Progression/USLevelUpCard.h"
#include "GameFramework/PlayerController.h"
#include "USPlayerController.generated.h"

class AUSEnemySpawner;
class UUSClearWidget;
class AUSEnemyCharacter;
class AUSBossEnemyCharacter;
class UUSBossHealthBarWidget;
class UUSEnemyTypeData;
struct FUSRunRecords;
class UUSGameOverWidget;
class UUSPauseWidget;
class UUSMetaTreeData;
class UUSStageData;
class UUSZoneData;
class UWorld;
enum class EUSSupplyShipState : uint8;
enum class EUSHordeShape : uint8;

/**
 * 플레이어 컨트롤러.
 */
UCLASS()
class US_API AUSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AUSPlayerController();

	/** 보급선이 상점 열기를 요청 (모달·게임오버 중이면 무시) */
	void RequestOpenShop();

	/** 게임오버 상태 */
	bool IsGameOver() const { return bGameOver; }

	/** 시험용(콘솔 US.Test.Boss): 클리어 시각을 기다리지 않고 보스전을 바로 시작한다 */
	void DebugStartBossFight();

	/** 시험용(콘솔 US.Test.ClearStage): 스테이지 시계를 클리어 시각으로 넘긴다 (보스 스테이지면 보스가 나온다) */
	void DebugClearStage();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

	/** 조종 중인 대장의 분대 (없으면 nullptr) */
	class UUSSquadComponent* GetPawnSquad() const;

	/** 플레이어 사망 → 게임오버 진입, GameOverDelay 뒤 게임오버 창 (폰 AttributeComponent::OnDeath에 바인딩) */
	void HandlePlayerDeath();

	/** 런을 끝낸다: 게임오버 상태·종료 시각·이유를 기록하고 런 시계를 멈춘다 (격추, 나가기, 출격 시간 끝) */
	void EndRun(EUSRunEndReason Reason);

	/** 결산: 결과를 만들어 메타 진행에 한 번 기록하고, 결산 창을 띄우고 일시정지한다 (사망 타이머, 나가기) */
	void ShowGameOver();

	/** 런의 결과(시간·처치·레벨·스크랩)를 만든다. 아직 끝나지 않았으면(클리어 창) 지금 시각까지로 계산한다 */
	FUSRunResult BuildRunResult() const;

	/** 이번 판을 기록하기 "전"의 최고 기록 (메타 진행이 없으면 bValid = false). 누적 수는 채우지 않는다 */
	FUSRunRecords ReadPreviousRecords() const;

	/** 이번 판 결과를 메타 진행에 한 번만 기록(스크랩 적립·저장)하고, 기록 전 최고 기록 + 누적 수를 돌려준다 (결산 창, 게임 종료) */
	FUSRunRecords RecordRunOnce();

	/** 판 시작: 영구 업그레이드를 대장·분대에 적용한다 (OnPossess, 폰 BeginPlay 전) */
	void ApplyMetaProgression(APawn* InPawn);

	// ---- 런 시계·클리어 ----
	/** 1초마다: HUD 남은 시간(계속 가기 중이면 더 버틴 시간) 갱신, 클리어 시각이면 클리어 창, 출격 시간이 끝나면 귀환 */
	void RefreshRunClock();

	/** 이번 판 출격 시간 = 기본 + 영구 트리 출격 시간 칸. 한 번만 계산한다 (OnPossess가 BeginPlay보다 먼저 불릴 수 있어 둘 다 부른다) */
	void InitializeRunDuration();

	/** 연쇄 폭발 = 기본 설정 + 영구 트리 연쇄 칸(확률·반경). 판 시작 때 서브시스템에 넘긴다 */
	void InitializeChainExplosion();

	/** 출격 시간 제한이 있는 판인가 */
	bool HasRunTimeLimit() const;

	/** 보급선에서 출격 시간을 더 살 의미가 있는가 (클리어 전이고, 출격 시간이 아직 클리어 시각보다 짧음) */
	bool CanExtendRunTime() const;

	/** HUD 남은 시간이 가리키는 시각(판 시작 뒤 초): 출격 시간이 클리어 시각보다 짧으면 출격 시간, 아니면 클리어 시각 */
	float GetClockTargetSeconds() const;

	/** 보급선 출격 시간 상품 → 출격 시간을 늘리고 HUD를 바로 갱신 */
	void HandleRunTimeExtended(float Seconds);

	/** 출격 시간 끝 → 살아서 귀환, 바로 결산 */
	void HandleRunTimeUp();

	/** 밸런스 확인용 한 줄 로그 (BalanceLogInterval마다): 시각·처치·DPS·적 체력 배율·적 수·골드·레벨 */
	void LogBalanceSnapshot(float ElapsedSeconds) const;

	/** 클리어: 깬 스테이지를 저장하고, 보스(행성) 스테이지면 클리어 창(일시정지 모달), 보통 스테이지면 클리어 연출 → 결산 */
	void ShowClear();

	// ---- 스테이지 (행성 지역 지도) ----
	/** 이번 출격 스테이지(격납고가 정함)를 스포너에 넣는다: 스폰 테이블·체력·물량 배율·보스 여부. 스테이지가 없으면 맵 설정 그대로 */
	void ApplyStageToSpawner();

	/**
	 * 보통 스테이지 클리어 연출 (2026-10-27, 판마다 처음부터): 판을 끝내고(귀환) 남은 적 격추·보상 끌어오기·클리어 띠 →
	 * StageClearBannerSeconds 뒤 이탈 비행 → 결산 창. 클리어 창(이어 가기 선택)은 없다
	 */
	void BeginStageOutro();

	/** 클리어 띠가 끝남 → 이탈 비행 */
	void StartStageDeparture();

	/** 이탈 비행 끝 → 결산 */
	void HandleDepartureFinished();

	/** 판 시작: 스테이지 출격이면 진입 자막 + 진입 비행 뒤 BeginRunIntro, 아니면 바로 BeginRunIntro (BeginPlay 다음 틱) */
	void BeginStageArrival();

	/** 맵에 떨어진 보상을 전부 대장에게 끌어온다 (클리어 연출) */
	void AttractAllPickups() const;

	/** 스테이지 띠·자막 위젯 생성 후 표시 (SetupHUD) */
	void SetupStageBanner();

	/** 판 시작 순서: 첫 판이면 조작 안내, 그다음 시작 카드 (BeginPlay 다음 틱) */
	void BeginRunIntro();

	/** 영구 트리 "시작 카드" 칸만큼 레벨업 카드를 고르게 한다 (출격당 한 번) */
	void GrantStartingCards();

	/** 클리어 창 "격납고로" → 기록 후 격납고 (결산 창 생략) */
	void HandleLeaveRequested();

	/** 클리어 창 "계속 가기"(행성 클리어 뒤) → 적이 구간마다 강해지는 상태로 게임 재개 */
	void HandleContinueRequested();

	/** 클리어 창을 닫는다 */
	void RemoveClearWidget();

	// ---- 일시정지 ----
	/** Esc / P: 일시정지 창을 열거나, 열려 있으면 한 단계 뒤로 (설정 창 → 일시정지 창 → 게임). 다른 모달·게임오버 중엔 무시 */
	void Input_Pause();

	/** 일시정지 창을 띄우고 멈춘다 */
	void OpenPauseMenu();

	/**
	 * 게임 창이 포커스를 잃으면(Alt+Tab 등) 일시정지 창을 연다 (출시 전 체크 R-17).
	 * 이미 멈춘 창(레벨업·상점·클리어·조작 안내)이나 게임오버 중이면 그대로 둔다. 돌아와도 자동으로 풀지 않는다 — 준비되면 플레이어가 "계속"
	 */
	void HandleApplicationActivationChanged(bool bIsActive);

	/** 일시정지 창을 닫는다 (멈춤 해제는 부르는 쪽) */
	void RemovePauseWidget();

	/** 일시정지 창 "계속" → 게임 재개 */
	void HandleResumeRequested();

	/** 일시정지 창 "출격 포기" → 그때까지 번 스크랩으로 결산 */
	void HandleAbandonRequested();

	/** 일시정지 창 "게임 종료" → 출격 포기처럼 그때까지 번 스크랩을 기록하고 게임을 끈다 (R-29) */
	void HandleQuitRequested();

	/** 게임오버 상태에서 R → 격납고를 건너뛰고 바로 재출격 (현재 레벨 리로드) */
	void OnRestartKey();

	/** 보스 신호가 떠 있을 때 B → 보스 호출 (R-41 2) */
	void OnCallBossKey();

	/** 게임오버 상태에서 Enter → 격납고 */
	void OnHangarKey();

	/** 결산 창 "다시 출격"·R → 끝난 스테이지를 출격 스테이지로 정하고 리로드 (재도전) */
	void HandleRetryRequested();

	/** 현재 레벨 리로드 */
	void HandleRestartRequested();

	/** 결산 창 "격납고로" → 격납고 맵 열기 (비어 있으면 리로드) */
	void HandleHangarRequested();

	/** 게임오버 창 "타이틀로" → 타이틀 맵 열기 */
	void HandleTitleRequested();

	// ---- 조작 안내 ----
	/** 첫 출격(기록된 판 0)이면 조작 안내 카드를 띄우고 멈춘다 (BeginPlay 다음 틱 — HUD·폰이 준비된 뒤) */
	void ShowControlsGuideIfFirstRun();

	/** 조작 안내 카드를 닫음 → 게임 시작 */
	void HandleControlsGuideDismissed();

protected:
	/** HUD 위젯 생성 후 표시 */
	void SetupHUD();

	/** 데미지 숫자 위젯 생성 후 표시 (HUD 아래 층) */
	void SetupDamageNumbers();

	/** 피격 테두리 위젯 생성 후 표시 (데미지 숫자와 같은 HUD 아래 층) */
	void SetupDamageVignette();

	/** 보급선을 한 번 찾아 방향 표시기·안내 문구를 만들고 상태 변화를 구독한다 (보급선은 맵에 한 대) */
	void SetupSupplyShipUI();

	/** 레이더 미니맵 생성 후 화면 오른쪽 위에 표시 (보급선이 있으면 함께 그린다) */
	void SetupRadar();

	/** 보급선 상태 변화 → 표시기 대상(떠나 있으면 숨김)과 안내 문구 갱신 */
	void HandleSupplyStateChanged(EUSSupplyShipState NewState, float SecondsRemaining);

	/** 무리 예고 위젯을 만들고 스포너 예고에 연결한다 */
	void SetupHordeNotice();

	// ---- 보스 (출시 전 체크 R-36) ----
	/** 보스 체력바 위젯을 숨긴 채 뷰포트에 올린다 (SetupHUD) */
	void SetupBossHealthBar();

	/**
	 * 클리어 시각(R-41 2): 보스를 바로 내보내지 않고 "보스 신호 포착 · B 호출"을 띄운다. 플레이어가 B로 부를 때까지 판은 이어지고,
	 * 출격 시간이 끝나면 보스 없이 귀환한다 (Risk of Rain 2 텔레포터처럼 보스전 시작은 플레이어가 고른다).
	 * 출격 시간이 신호 직후 끝나면 고를 틈이 없으므로 최소 BossSignalGraceSeconds는 남겨 준다
	 */
	void ShowBossSignal();

	/** B 호출 → 시계를 멈추고 경고 띠·경고음을 BossSummonWarningSeconds 동안 먼저 띄운 뒤 FinishBossSummon */
	void BeginBossSummon();

	/** 경고가 끝남 → 보스 스폰. 실패하면 예전처럼 바로 클리어 */
	void FinishBossSummon();

	/** 보스 스폰 → 체력바 (경고는 BeginBossSummon이 이미 띄웠다). 보스가 없거나 스폰에 실패하면 false */
	bool StartBossFight();

	/** 보스 신호가 뜨기 BossApproachLeadSeconds 전부터 "신호 포착까지" 카운트다운을 띄운다 (매초 RefreshRunClock) */
	void RefreshBossApproach(float Elapsed);

	/** 보스 격파 → 체력바를 내리고 슬로모션을 건 뒤 BossDefeatSlowSeconds(실제 초) 뒤 FinishBossDefeat */
	void HandleBossDeath(AUSEnemyCharacter* Enemy);

	/** 슬로모션을 풀고 클리어 창 + 격파한 보스 칸 */
	void FinishBossDefeat();

	/** 스포너 무리 예고 → 예고 위젯에 전달 */
	void HandleHordeWarning(EUSHordeShape Shape, const FVector& HordeCenter, float SecondsUntilSpawn);

	/** 분대 표시 위젯 생성 후 표시 (폰이 있어야 분대·골드를 연결할 수 있다) */
	void SetupSquadWidget(APawn* InPawn);

	// ---- 타격감 ----
	/** 전투 연출 중계소 구독 (BeginPlay) */
	void BindCombatFeedback();

	/** 적이 받은 데미지(대장·부하 모두) → 데미지 숫자 */
	void HandleEnemyDamaged(const FVector& Location, float Damage, bool bByPlayer);

	/** 대장이 처치 → 약한 화면 흔들림 */
	void HandlePlayerKilledEnemy(const FVector& Location);

	/** 대장이 맞음 → 화면 흔들림 + 붉은 테두리 + 피격음 */
	void HandlePawnDamaged(float Damage, AController* InstigatedBy);

	/** HitShakeClass를 배율 Scale로 재생 */
	void PlayHitShake(float Scale);

	/** 처치 히트스톱: 전역 시간을 HitStopDilation으로 HitStopDuration(실제 초)만큼 늦춘다. 쿨다운 안이면 건너뛴다 */
	void PlayHitStop();

	/** 히트스톱 끝: 전역 시간 배율을 1로 되돌린다 */
	void EndHitStop();

	/** 폰의 속성 컴포넌트에 레벨업 이벤트 바인딩 */
	void BindToPawn(APawn* InPawn);
	
	// ---- 진행 조정자 ----
	/** 레벨업 발생 → 카드 선택을 큐에 적재 */
	void HandlePlayerLevelUp();

	/** 큐에 남은 카드 선택을 하나 표시 */
	void TryShowNextLevelUpCard();

	/** 카드 선택 처리 → 적용 후 다음 카드 또는 모달 종료 */
	void HandleCardSelected(const FUSLevelUpCard& Card);

	/** 다시 뽑기: 같은 창에서 카드 3장을 새로 뽑고 남은 횟수를 하나 쓴다 */
	void HandleCardRerollRequested();

	// ---- 상점 (보급선) ----
	/** 상품 목록을 만들고 상점 창을 연다 (일시정지) */
	void OpenShop();

	/** 상품 선택 → 구매 시도 후 창을 다시 그린다 */
	void HandleShopOfferSelected(int32 Index);

	/** 상점 창 닫기 → 모달 종료 */
	void HandleShopClosed();

	/** 상품 목록을 다시 만들고 상점 창을 다시 그린다 */
	void RefreshShopWidget();

	// ---- 모달 UI ----
	void EnterModalUI();
	void ExitModalUI();

	/** 게임플레이 입력 모드: 쿼터뷰 커서 조준용으로 커서를 항상 보이게 하고 게임+UI 입력을 받는다 (BeginPlay, 모달 종료 시) */
	void ApplyGameplayInputMode();

	/** 판 진행·게임오버 키(IMC_UI)를 로컬 플레이어에 등록한다. 기체가 아니라 컨트롤러가 등록해야 기체가 죽은 게임오버에서도 키가 남는다 */
	void AddUIMappingContext();

protected:
	// ---- 입력 (Enhanced Input) ----
	/** 판 진행·게임오버 키 묶음. 기체의 IMC_Default보다 우선순위를 높게 등록한다 */
	UPROPERTY(EditAnywhere, Category = "US|Input")
	TObjectPtr<class UInputMappingContext> UIMappingContext;

	/** Esc / P: 일시정지 (입력 애셋의 bTriggerWhenPaused로 멈춘 중에도 받는다) */
	UPROPERTY(EditAnywhere, Category = "US|Input")
	TObjectPtr<class UInputAction> PauseAction;

	/** B: 보스 호출 */
	UPROPERTY(EditAnywhere, Category = "US|Input")
	TObjectPtr<class UInputAction> CallBossAction;

	/** R: 게임오버에서 바로 재출격 (bTriggerWhenPaused) */
	UPROPERTY(EditAnywhere, Category = "US|Input")
	TObjectPtr<class UInputAction> RestartAction;

	/** Enter: 게임오버에서 격납고로 (bTriggerWhenPaused) */
	UPROPERTY(EditAnywhere, Category = "US|Input")
	TObjectPtr<class UInputAction> ToHangarAction;

	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSubclassOf<class UUSPlayerHUD> HUDWidgetClass;

	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSubclassOf<class UUSLevelUpWidget> LevelUpWidgetClass;

	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSubclassOf<class UUSShopWidget> ShopWidgetClass;

	/** 분대 표시 위젯 (WBP, 텍스트 이름 SquadText) */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSubclassOf<class UUSSquadWidget> SquadWidgetClass;

	/** 데미지 숫자 위젯 (WBP, 캔버스 이름 NumberCanvas) */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSubclassOf<class UUSDamageNumberWidget> DamageNumberWidgetClass;

	/** 대장 피격 때 화면 가장자리를 붉게 물들이는 위젯 (WBP, 이미지 이름 VignetteImage) */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSubclassOf<class UUSDamageVignetteWidget> DamageVignetteWidgetClass;

	/** 보급선이 화면 밖일 때 방향을 가리키는 위젯 (WBP, 이미지 이름 ArrowImage) */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSubclassOf<class UUSOffscreenIndicatorWidget> SupplyIndicatorWidgetClass;

	/** 보급선 도착·출발 안내 문구 위젯 (WBP, 텍스트 이름 NoticeText) */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSubclassOf<class UUSSupplyNoticeWidget> SupplyNoticeWidgetClass;

	/** 적 무리 예고 위젯 (WBP, 텍스트 NoticeText·이미지 EdgeBand) */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSubclassOf<class UUSHordeNoticeWidget> HordeNoticeWidgetClass;

	/** 레이더 미니맵 위젯 (WBP, 배경 이미지만) */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSubclassOf<class UUSRadarWidget> RadarWidgetClass;

	/** 일시정지 창 위젯 (WBP, 이름은 UUSPauseWidget 주석 참고) */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSubclassOf<UUSPauseWidget> PauseWidgetClass;

	/** 결산 창 위젯 (WBP, 이름은 UUSGameOverWidget 주석 참고) */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSubclassOf<UUSGameOverWidget> GameOverWidgetClass;

	/** 보스 등장 경고 띠 + 체력바 (WBP_USBossHealthBar) */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSubclassOf<UUSBossHealthBarWidget> BossHealthBarWidgetClass;

	/** 화면 밖 보스 방향 표시기 (WBP_USBossIndicator, 보급선 표시기와 같은 클래스) */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSubclassOf<class UUSOffscreenIndicatorWidget> BossIndicatorWidgetClass;

	/**
	 * 보스 체력 = 보스가 나올 때의 편대 DPS × 이 시간(초). 판마다 강해진 정도와 상관없이 보스전 길이를 비슷하게 맞춘다
	 * (사용자: 고정 체력 60000은 너무 커서 못 잡음). 0 이하면 종류 데이터 체력 그대로
	 */
	UPROPERTY(EditAnywhere, Category = "US|Boss", meta = (ClampMin = "0.0"))
	float BossFightSeconds = 25.0f;

	/** 위 계산의 하한 (DPS가 아직 0이거나 아주 낮을 때) */
	UPROPERTY(EditAnywhere, Category = "US|Boss", meta = (ClampMin = "1.0"))
	float BossMinHealth = 3000.0f;

	/**
	 * 보스 등장 몇 초 전부터 "접근 중" 카운트다운을 띄울지. 0이면 예고 없음.
	 * 처음 하는 사람은 5:00에 보스가 나온다는 걸 모른다 (사용자 플레이 2026-10-21: "너무 뜬금없음")
	 */
	UPROPERTY(EditAnywhere, Category = "US|Boss", meta = (ClampMin = "0.0"))
	float BossApproachLeadSeconds = 30.0f;

	/** 보스 신호가 뜬 뒤 출격 시간이 이보다 적게 남았으면 이만큼으로 늘린다 (B를 누를지 고를 시간) */
	UPROPERTY(EditAnywhere, Category = "US|Boss", meta = (ClampMin = "0.0"))
	float BossSignalGraceSeconds = 60.0f;

	/** B로 부른 뒤 보스가 나오기까지 경고 띠·경고음 시간(초). 경고 위젯 WarningSeconds와 맞춘다 */
	UPROPERTY(EditAnywhere, Category = "US|Boss", meta = (ClampMin = "0.0"))
	float BossSummonWarningSeconds = 3.0f;

	/** 보스 격파 슬로모션 배율 (전역 시간) */
	UPROPERTY(EditAnywhere, Category = "US|Boss", meta = (ClampMin = "0.05", ClampMax = "1.0"))
	float BossDefeatDilation = 0.25f;

	/** 보스 격파 슬로모션 길이 (실제 초). 끝나면 클리어 창 */
	UPROPERTY(EditAnywhere, Category = "US|Boss", meta = (ClampMin = "0.0"))
	float BossDefeatSlowSeconds = 1.6f;

	/** 클리어 창 위젯 (WBP, DescriptionText·LeaveButton·ContinueButton) */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSubclassOf<UUSClearWidget> ClearWidgetClass;

	/** 첫 판 조작 안내 카드 (WBP_USControlsGuide) */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSubclassOf<class UUSControlsGuideWidget> ControlsGuideWidgetClass;

	/** 판 수와 상관없이 매 판 조작 안내를 띄운다 (PIE 확인용, 출시 값은 false) */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	bool bAlwaysShowControlsGuide = false;

	/** 결산 스크랩 공식 */
	UPROPERTY(EditAnywhere, Category = "US|Run")
	TObjectPtr<UUSScrapRewardData> ScrapRewardData;

	/** 영구 업그레이드 트리. 판 시작 때 효과를 적용하고, 결산 스크랩 배율을 계산한다 (격납고 컨트롤러도 같은 애셋을 가리킨다) */
	UPROPERTY(EditAnywhere, Category = "US|Run")
	TObjectPtr<UUSMetaTreeData> MetaTree;

	/**
	 * 클리어 시각(스테이지 시작 뒤 초). 버티면 스테이지 클리어, 보스 스테이지는 이때 보스가 나온다
	 * (사용자 피드백 2026-10-26: "보스는 아무리 길어도 1분 30초쯤"). 0 이하면 클리어 없음
	 */
	UPROPERTY(EditAnywhere, Category = "US|Run", meta = (ClampMin = "0.0"))
	float ClearTimeSeconds = 90.0f;

	/**
	 * 출격 시간 기본값(초). 끝나면 귀환한다. 0 이하면 시간 제한 없음 — 스테이지 판(1:30 고정)부터 0 (예전 D6 타이머 판: 90초 + 트리·상점 연장)
	 */
	UPROPERTY(EditAnywhere, Category = "US|Run", meta = (ClampMin = "0.0"))
	float BaseRunDurationSeconds = 0.0f;

	/** 보통 스테이지 클리어 띠를 보여 주는 시간(초). 남은 적 폭발·보상 끌어오기를 이 동안 보여 준 뒤 이탈 비행 */
	UPROPERTY(EditAnywhere, Category = "US|Run", meta = (ClampMin = "0.0"))
	float StageClearBannerSeconds = 1.2f;

	/** 진입 자막이 진입 비행 뒤에 더 남는 시간(초) */
	UPROPERTY(EditAnywhere, Category = "US|Run", meta = (ClampMin = "0.0"))
	float StageTitleSeconds = 2.0f;

	/** 연쇄 폭발 기본 설정. 확률·반경에는 판 시작 때 영구 트리 칸이 더해진다 */
	UPROPERTY(EditAnywhere, Category = "US|Run")
	FUSChainExplosionSettings ChainExplosion;

	/** [Balance] 로그 간격(초). 0 이하면 끈다 */
	UPROPERTY(EditAnywhere, Category = "US|Run", meta = (ClampMin = "0.0"))
	float BalanceLogInterval = 30.0f;

	/** 대장 사망 뒤 게임오버 창까지 기다리는 시간(초). 사망 폭발을 보여 준다 */
	UPROPERTY(EditAnywhere, Category = "US|UI", meta = (ClampMin = "0.0"))
	float GameOverDelay = 1.5f;

	/** "타이틀로" 버튼이 여는 맵 */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSoftObjectPtr<UWorld> TitleMap;

	/** "격납고로" 버튼(Enter)이 여는 맵 */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSoftObjectPtr<UWorld> HangarMap;

	/** 처치·피격 화면 흔들림 클래스 (세기는 아래 배율로 나눈다) */
	UPROPERTY(EditAnywhere, Category = "US|Feedback")
	TSubclassOf<class UCameraShakeBase> HitShakeClass;

	/** 대장이 적을 처치했을 때 흔들림 배율 (약하게) */
	UPROPERTY(EditAnywhere, Category = "US|Feedback", meta = (ClampMin = "0.0"))
	float KillShakeScale = 0.4f;

	/** 대장이 맞았을 때 흔들림 배율 (처치보다 확실히 세게 — 맞는 느낌이 약하다는 피드백, 2026-11-01 1.0 → 1.5) */
	UPROPERTY(EditAnywhere, Category = "US|Feedback", meta = (ClampMin = "0.0"))
	float HitShakeScale = 1.5f;

	/** 대장 피격음 (2D, 거리 감쇠 없음). 없으면 소리 없이 넘어간다 */
	UPROPERTY(EditAnywhere, Category = "US|Feedback")
	TObjectPtr<class USoundBase> PlayerHitSound;

	/** 피격음 최소 간격(실제 초). 여러 적이 한꺼번에 박아도 소리가 겹쳐 터지지 않게 */
	UPROPERTY(EditAnywhere, Category = "US|Feedback", meta = (ClampMin = "0.0"))
	float PlayerHitSoundCooldown = 0.12f;

	/** 대장 처치 히트스톱 길이 (실제 초). 0 = 끔 */
	UPROPERTY(EditAnywhere, Category = "US|Feedback", meta = (ClampMin = "0.0"))
	float HitStopDuration = 0.03f;

	/** 히트스톱 동안 전역 시간 배율 (0에 가까울수록 멈춤) */
	UPROPERTY(EditAnywhere, Category = "US|Feedback", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float HitStopDilation = 0.05f;

	/** 히트스톱 최소 간격 (실제 초). 떼로 죽을 때 화면이 계속 끊기지 않게 */
	UPROPERTY(EditAnywhere, Category = "US|Feedback", meta = (ClampMin = "0.0"))
	float HitStopCooldown = 0.25f;

	/** 업그레이드(레벨업 카드) 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, Category = "US|Progression")
	TObjectPtr<class UUSUpgradeComponent> UpgradeComponent;

	/** 보급선 상점 로직 (상품 목록·구매) */
	UPROPERTY(VisibleAnywhere, Category = "US|Shop")
	TObjectPtr<class UUSShopComponent> ShopComponent;

	/** 스테이지 이탈·진입 워프 점프 */
	UPROPERTY(VisibleAnywhere, Category = "US|Run")
	TObjectPtr<class UUSStageTransitionComponent> StageTransitionComponent;

private:
	UPROPERTY()
	TObjectPtr<class UUSPlayerHUD> HUDWidget;

	UPROPERTY()
	TObjectPtr<class UUSLevelUpWidget> LevelUpWidget;

	UPROPERTY()
	TObjectPtr<class UUSShopWidget> ShopWidget;

	UPROPERTY()
	TObjectPtr<class UUSDamageNumberWidget> DamageNumberWidget;

	UPROPERTY()
	TObjectPtr<class UUSDamageVignetteWidget> DamageVignetteWidget;

	/** 마지막 피격음 시각(실제 초) — 히트스톱이 게임 시간을 늦추므로 실제 시간으로 잰다 */
	double LastPlayerHitSoundRealTime = -1.0e9;

	UPROPERTY()
	TObjectPtr<class UUSSquadWidget> SquadWidget;

	UPROPERTY()
	TObjectPtr<class UUSOffscreenIndicatorWidget> SupplyIndicatorWidget;

	UPROPERTY()
	TObjectPtr<class UUSSupplyNoticeWidget> SupplyNoticeWidget;

	UPROPERTY()
	TObjectPtr<class UUSHordeNoticeWidget> HordeNoticeWidget;

	UPROPERTY()
	TObjectPtr<class UUSRadarWidget> RadarWidget;

	UPROPERTY()
	TObjectPtr<UUSGameOverWidget> GameOverWidget;

	UPROPERTY()
	TObjectPtr<UUSClearWidget> ClearWidget;

	UPROPERTY()
	TObjectPtr<UUSBossHealthBarWidget> BossHealthBarWidget;

	/** 스테이지 클리어 띠·진입 자막 (C++만으로 만든 위젯이라 클래스 지정이 없다) */
	UPROPERTY()
	TObjectPtr<class UUSStageBannerWidget> StageBannerWidget;

	/** 클리어 띠 → 이탈 비행 */
	FTimerHandle StageOutroTimerHandle;

	UPROPERTY()
	TObjectPtr<class UUSOffscreenIndicatorWidget> BossIndicatorWidget;

	/** 지금 싸우는 보스 (격파·맵 이동 때 구독 해제) */
	TWeakObjectPtr<AUSBossEnemyCharacter> ActiveBoss;

	/** 보스전 중 — B로 부른 뒤 경고 동안도 포함 (시계·출격 시간 끝 판정 멈춤) */
	bool bBossFight = false;

	/** 보스 신호가 떠서 B 호출을 기다리는 중 */
	bool bBossSignal = false;

	/** 호출 경고 → 보스 스폰 */
	FTimerHandle BossSummonTimerHandle;

	/** 격파 슬로모션 중 (히트스톱이 시간 배율을 1로 되돌리지 못하게) */
	bool bBossDefeatSlowMo = false;

	/** 격파 슬로모션 → 클리어 창 */
	FTimerHandle BossDefeatTimerHandle;

	UPROPERTY()
	TObjectPtr<UUSPauseWidget> PauseWidget;

	UPROPERTY()
	TObjectPtr<class UUSControlsGuideWidget> ControlsGuideWidget;

	/** 사망 → 게임오버 창 지연 */
	FTimerHandle GameOverTimerHandle;

	/** 히트스톱 해제 타이머 */
	FTimerHandle HitStopTimerHandle;

	/** 마지막 히트스톱 시작 실제 시각 (FPlatformTime::Seconds) */
	double LastHitStopRealTime = -1000.0;

	/** 런 시계 (1초 반복, 월드 타이머라 일시정지 중 멈춤) */
	FTimerHandle RunClockTimerHandle;

	/** 런 시작 시각 (월드 시간). 월드 시간은 일시정지 중 멈추므로 카드·상점을 연 시간은 생존 시간에서 빠진다 */
	float RunStartTime = 0.0f;

	/** 런이 끝난 시각 (월드 시간). 사망 폭발 대기 시간이 생존 시간에 섞이지 않게 끝난 순간을 따로 기록한다 */
	float RunEndTime = 0.0f;

	/** 계속 가기를 고른 시각 (월드 시간) */
	float ContinueStartTime = 0.0f;

	/** 지금 스테이지를 시작한 시각 (월드 시간). 시계·클리어 시각은 이것 기준, 생존 시간은 RunStartTime 기준 */
	float StageStartTime = 0.0f;

	/** 이번 출격 구역·스테이지 (메타 진행 SetSortie에서 받는다. 스테이지가 없는 판이면 nullptr) */
	UPROPERTY()
	TObjectPtr<const UUSZoneData> CurrentZone;

	UPROPERTY()
	TObjectPtr<const UUSStageData> CurrentStage;

	/** 이번 출격에서 깬 스테이지 수 (결산 "클리어" 판정·스테이지 보너스). 판마다 처음부터라 0 또는 1 */
	int32 StagesClearedThisRun = 0;

	/** 이번 출격에서 보스 스테이지를 깼는가 (행성 클리어 보너스) */
	bool bPlanetCleared = false;

	/** 시작 카드를 줬는가 */
	bool bStartingCardsGranted = false;

	/** 이번 판 출격 시간(판 시작 뒤 초). InitializeRunDuration에서 정하고 보급선 상품으로 늘어난다 */
	float RunDurationSeconds = 0.0f;

	/** 다음 [Balance] 로그를 남길 시각(판 시작 뒤 초) */
	float NextBalanceLogSeconds = 0.0f;

	/** 출격 시간을 계산했는가 */
	bool bRunDurationInitialized = false;

	/** 런이 끝난 이유 */
	EUSRunEndReason RunEndReason = EUSRunEndReason::Destroyed;

	/** 목표 시간을 버텼는가 */
	bool bCleared = false;

	/** 계속 가기 중인가 */
	bool bContinuing = false;

	/** 영구 업그레이드를 이번 판에 적용했는가 (빙의가 다시 일어나도 두 번 곱하지 않게) */
	bool bMetaApplied = false;

	/** 이번 판 결과를 메타 진행에 기록했는가 (스크랩 이중 적립 방지) */
	bool bRunRecorded = false;

	/** 맵의 보급선 (SetupSupplyShipUI에서 한 번 찾음). 레벨이 바뀌면 사라지므로 약한 참조 */
	TWeakObjectPtr<class AUSSupplyShip> SupplyShip;

	/** 맵의 적 스포너 ([Balance] 로그의 적 체력 배율, BeginPlay에서 한 번 찾음) */
	TWeakObjectPtr<AUSEnemySpawner> EnemySpawner;

	/** 읽기 전용 (Pawn의 컴포넌트에 대한 약한 참조) UPROPERTY 사용 불가 */
	TWeakObjectPtr<class UUSAttributeComponent> PawnAttribute;
	TWeakObjectPtr<class UUSExperienceComponent> PawnExperience;
	TWeakObjectPtr<class UUSCurrencyComponent> PawnCurrency;

	/** 보류된 카드 선택 수 (다중 레벨업 대응) */
	int32 PendingCardSelections = 0;

	/** 이번 판에 남은 카드 다시 뽑기 횟수 (격납고 CardReroll 칸, BeginPlay에서 정한다) */
	int32 CardRerollsLeft = 0;

	/** 모달 UI 열림 상태 */
	bool bModalOpen = false;

	/** 앱 포커스 변경 구독 (FSlateApplication, EndPlay에서 해제) */
	FDelegateHandle ActivationChangedHandle;

	/** OnLevelUp 바인딩 완료 여부 (중복 바인딩 방지) */
	bool bLevelUpBound = false;

	/** OnDeath 바인딩 완료 여부 (중복 바인딩 방지) */
	bool bDeathBound = false;

	/** OnDamaged 바인딩 완료 여부 (중복 바인딩 방지) */
	bool bDamagedBound = false;

	/** 게임오버 상태 */
	bool bGameOver = false;

	/** HUD 초기화 완료 여부 (중복 초기화 방지) */
	bool bHUDInitialized = false;
};
