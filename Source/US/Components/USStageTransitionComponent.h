#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USStageTransitionComponent.generated.h"

class AUSPlayerCharacter;
class AUSSpaceDust;
class UUSStageBannerWidget;
class USoundBase;

/** 이탈·진입 워프가 끝남 */
DECLARE_DELEGATE(FDelegateOnTransitionFinished);

/**
 * 스테이지 넘어가는 워프 점프 연출 (사용자 요청 2026-10-27, FTL·Everspace식 워프, 스토리보드 5컷 확정).
 * 이탈: 충전(편대 정지·기수 위로·화면 떨림) → 별이 선으로(주변 먼지 점 늘임·줌아웃·살짝 뒤로 웅크림)
 *       → 점프(기체가 빛줄기처럼 길어지며 화면 위로 순간 가속 + 섬광) → 섬광이 걷힐 때까지 → 결산.
 * 진입: 화면 밖에서 대기(맵 전환 검은 화면 동안) → 빛줄기 도착(길게 늘어난 기체가 아래에서 날아와 섬광과 함께 조금 지나침)
 *       → 급정지(제자리로 되밀리며 원래 모양, 별이 점으로).
 * 카메라는 출발·도착 자리에 세워 둔다(AUSPlayerCharacter::SetCameraFrozen). 기체 위치·크기를 직접 바꾸고 이동 입력은 막는다.
 * 연출하는 동안만 컴포넌트 Tick을 켠다 — 고정 간격 타이머는 화면 주사율과 어긋나 끊겨 보였다(2026-10-27 PIE). 플레이어 컨트롤러에 붙는다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class US_API UUSStageTransitionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUSStageTransitionComponent();

	/** 섬광을 띄울 위젯 (컨트롤러가 만든 뒤 넘긴다). 없으면 섬광만 빠진다 */
	void SetBanner(UUSStageBannerWidget* InBanner);

	/** 이탈 워프를 시작한다. 끝나면 OnFinished (대장이 없으면 바로) */
	void PlayDeparture(AUSPlayerCharacter* Leader, FDelegateOnTransitionFinished OnFinished);

	/** 진입 워프: 편대를 화면 밖으로 옮겨 두고 ArrivalDelay 뒤 들어온다. 끝나면 OnFinished (대장이 없으면 바로) */
	void PlayArrival(AUSPlayerCharacter* Leader, FDelegateOnTransitionFinished OnFinished);

	/** 진입 워프가 끝날 때까지의 시간(초). 진입 자막을 맞추는 데 쓴다 */
	float GetArrivalTotalSeconds() const;

	/** 연출 중인가 */
	FORCEINLINE bool IsPlaying() const { return Phase != EPhase::None; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	enum class EPhase : uint8
	{
		None,
		Charge,
		Streak,
		Jump,
		Tail,
		ArrivalWait,
		ArrivalBeam,
		ArrivalSettle
	};

	/** 날 기체 목록(대장 + 살아 있는 부하)과 출발 자리·방향·크기를 모으고 입력·카메라를 잠근다 */
	bool BeginSequence(AUSPlayerCharacter* Leader, FDelegateOnTransitionFinished OnFinished);

	/** Phase로 넘어가고 시작 시각을 적는다. 단계 시작 효과(소리·섬광)도 여기서 */
	void EnterPhase(EPhase NewPhase);

	/** 연출 한 걸음 (연출 중 매 프레임) */
	void StepSequence();

	/** 기체 하나의 자리: 기준 자리 + 방향 × Offset, 진행 방향 길이 × Stretch */
	void PlaceFlyer(int32 Index, float Offset, float Stretch, float Yaw) const;

	/** 먼지 늘임·시야각 */
	void ApplyWarpLook(float DustStretch, float FovOffset) const;

	/** 끝: (진입이면) 크기·카메라·입력을 돌려놓고 알린다 */
	void FinishSequence();

	/** 맵의 먼지 층 (연출마다 한 번 찾는다) */
	AUSSpaceDust* FindDust();

	float GetPhaseSeconds(EPhase InPhase) const;

	// ---- 이탈 ----
	/** 충전: 편대가 멈추고 기수를 화면 위로 돌리며 화면이 떨린다 */
	UPROPERTY(EditDefaultsOnly, Category = "Warp|Departure", meta = (ClampMin = "0.05"))
	float ChargeSeconds = 0.8f;

	/** 별이 선으로 늘어나는 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Warp|Departure", meta = (ClampMin = "0.05"))
	float StreakSeconds = 0.5f;

	/** 점프(화면 밖으로 순간 가속) 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Warp|Departure", meta = (ClampMin = "0.05"))
	float JumpSeconds = 0.2f;

	/** 점프 뒤 섬광이 걷힐 때까지 기다리는 시간. 결산 창이 게임을 멈추므로 그 전에 섬광이 끝나야 한다 */
	UPROPERTY(EditDefaultsOnly, Category = "Warp|Departure", meta = (ClampMin = "0.0"))
	float TailSeconds = 0.4f;

	/** 점프 거리(cm). 화면 위 끝을 넉넉히 넘게 */
	UPROPERTY(EditDefaultsOnly, Category = "Warp|Departure", meta = (ClampMin = "0.0"))
	float DepartureDistance = 4000.0f;

	/** 별이 늘어나는 동안 뒤로 웅크리는 거리(cm) */
	UPROPERTY(EditDefaultsOnly, Category = "Warp|Departure", meta = (ClampMin = "0.0"))
	float PullBackDistance = 60.0f;

	/** 충전 떨림 간격(초)과 세기 (처음 → 끝, 처치 흔들림 배율) */
	UPROPERTY(EditDefaultsOnly, Category = "Warp|Departure", meta = (ClampMin = "0.05"))
	float ChargeShakeInterval = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Warp|Departure")
	FVector2D ChargeShakeScale = FVector2D(0.2f, 0.9f);

	// ---- 진입 ----
	/**
	 * 진입 전 대기(초). 맵 전환 검은 화면 유지(UUSLoadingScreenSettings::HoldSeconds 0.6)가 끝나고 밝아지기 시작할 무렵 들어오게.
	 * 맵을 바로 연 PIE에는 페이드가 없어 그만큼 빈 화면이 보인다
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Warp|Arrival", meta = (ClampMin = "0.0"))
	float ArrivalDelay = 0.7f;

	/** 빛줄기가 날아와 조금 지나치는 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Warp|Arrival", meta = (ClampMin = "0.05"))
	float ArrivalBeamSeconds = 0.3f;

	/** 제자리로 되밀리며 멈추는 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Warp|Arrival", meta = (ClampMin = "0.05"))
	float ArrivalSettleSeconds = 0.9f;

	/** 진입 출발 거리(cm, 화면 아래 밖) */
	UPROPERTY(EditDefaultsOnly, Category = "Warp|Arrival", meta = (ClampMin = "0.0"))
	float ArrivalDistance = 3000.0f;

	/** 제자리를 지나치는 거리(cm) */
	UPROPERTY(EditDefaultsOnly, Category = "Warp|Arrival", meta = (ClampMin = "0.0"))
	float ArrivalOvershoot = 90.0f;

	// ---- 모양 ----
	/** 점프·도착 순간 기체가 진행 방향으로 늘어나는 배율 (빛줄기) */
	UPROPERTY(EditDefaultsOnly, Category = "Warp|Look", meta = (ClampMin = "1.0"))
	float ShipStreakStretch = 7.0f;

	/** 먼지 점 늘임: 충전 끝 / 별이 선 끝 */
	UPROPERTY(EditDefaultsOnly, Category = "Warp|Look", meta = (ClampMin = "1.0"))
	float DustChargeStretch = 1.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Warp|Look", meta = (ClampMin = "1.0"))
	float DustStreakStretch = 16.0f;

	/** 줌아웃 (시야각 + 도) */
	UPROPERTY(EditDefaultsOnly, Category = "Warp|Look", meta = (ClampMin = "0.0"))
	float FovOffset = 8.0f;

	/** 섬광 세기: 점프 / 도착 */
	UPROPERTY(EditDefaultsOnly, Category = "Warp|Look", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float JumpFlashOpacity = 0.85f;

	UPROPERTY(EditDefaultsOnly, Category = "Warp|Look", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ArrivalFlashOpacity = 0.6f;

	/** 점프음 / 도착음 (비우면 소리 없음) */
	UPROPERTY(EditDefaultsOnly, Category = "Warp|Sound")
	TObjectPtr<USoundBase> JumpSound;

	UPROPERTY(EditDefaultsOnly, Category = "Warp|Sound")
	TObjectPtr<USoundBase> ArrivalSound;

private:
	TWeakObjectPtr<AUSPlayerCharacter> FlyingLeader;
	TWeakObjectPtr<UUSStageBannerWidget> Banner;
	TWeakObjectPtr<AUSSpaceDust> Dust;

	/** 나는 기체와 기준 자리(이탈 = 출발 자리, 진입 = 도착 자리)·처음 기수·처음 크기 */
	TArray<TWeakObjectPtr<AActor>> Flyers;
	TArray<FVector> BaseLocations;
	TArray<float> StartYaws;
	TArray<FVector> BaseScales;

	/** 화면 위쪽 (바닥 평면) */
	FVector Direction = FVector::ForwardVector;
	float DirectionYaw = 0.0f;

	EPhase Phase = EPhase::None;

	/** 지금 단계 시작 시각 (월드 시간) */
	float PhaseStartTime = 0.0f;

	/** 다음 충전 떨림 시각 (단계 시작 뒤 초) */
	float NextShakeTime = 0.0f;

	FDelegateOnTransitionFinished PendingFinished;
};
