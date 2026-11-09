#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USPlayerHUD.generated.h"

/**
 * 플레이어 HUD를 표시하는 UI
 */
UCLASS()
class US_API UUSPlayerHUD : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UUSPlayerHUD(const FObjectInitializer& ObjectInitializer);

	/**
	 * 플레이어 폰의 속성/경험치/통화 컴포넌트에 바인딩한다.
	 * NativeConstruct 시점에는 아직 폰이 없을 수 있으므로(위젯 생성이 OnPossess보다 앞설 수 있음),
	 * 폰을 확보한 AUSPlayerController가 명시적으로 호출한다.
	 */
	void InitializeForPawn(class APawn* InPawn);

public:
	void OnHealthChanged(float CurrentValue, float MaxValue) const;
	
	void OnExperienceChanged(float CurrentExperience, float TargetExperience) const;
	
	void OnLevelChanged(int Value) const;
	
	void OnQuickInfoChanged(int32 Value) const;

	/** 처치 수 갱신 (UUSRunStatsSubsystem::OnKillCountChanged) */
	void HandleKillCountChanged(int32 KillCount) const;

	/** 편대 DPS 갱신 (UUSRunStatsSubsystem::OnSquadDpsChanged). 부하 비율은 여기서 GetSafeRatio로 계산한다 */
	void HandleSquadDpsChanged(float SquadDps, float AllyDps) const;

	/**
	 * 런 시계 갱신 (컨트롤러가 1초마다 호출).
	 * bContinuing이 false면 Seconds = 귀환(출격 시간 끝) 또는 클리어까지 남은 시간 중 먼저 오는 쪽, true면 계속 가기로 더 버틴 시간
	 */
	void SetRunTime(float Seconds, bool bContinuing) const;

	/** 보스전 중 시계 자리 문구 (0:00에 멈춘 시계 대신, R-41 14) */
	void ShowBossFightTime() const;

	/** 0 나눗셈을 피한 비율(0.0 ~ 1.0) 계산. 바 위젯과 분대 칸 색이 전부 이 한 곳을 거친다 (그래서 public static) */
	static float GetSafeRatio(float CurrentValue, float MaxValue);

protected:
	/** 남은 시간 문구. {0} = 분, {1} = 초(두 자리) */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Text")
	FText RemainingTimeFormat;

	/** 계속 가기 시간 문구. {0} = 분, {1} = 초(두 자리) */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Text")
	FText ContinueTimeFormat;

	/** 보스전 중 시계 자리 문구 */
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Text")
	FText BossFightTimeText;

	/** 화면 위 가운데 런 시계 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> RunTimeText;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UUSHealthBar> HealthBarUI;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UUSExperienceBar> ExperienceBarUI;

	/** 레벨 배지 숫자 (경험치 바와 떨어진 왼쪽 위 배지). 없으면 경험치 바 안의 레벨 글자를 쓴다 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> LevelText;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UUSQuickInfoUI> QuickInfoUI;
};
