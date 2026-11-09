#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Agents/USDroneCharacter.h"
#include "USPlayerCharacter.generated.h"

class UUSCurrencyComponent;
class UUSAimComponent;
class UUSPlayerInputComponent;
class UUSExperienceComponent;
class UUSSquadComponent;
class UUSMissileBarrageComponent;
class UUSTargetMarkComponent;
class UUSTargetRingComponent;
class USpringArmComponent;
class UCameraComponent;
class UAnimMontage;
class USoundBase;
struct FInputActionValue;

/**
 * 대장 드론(플레이어). 드론 공통 기능은 AUSDroneCharacter에 있고,
 * 여기는 입력·쿼터뷰 카메라·커서 조준·골드/경험치·상호작용만 담당한다.
 */
UCLASS()
class US_API AUSPlayerCharacter : public AUSDroneCharacter
{
	GENERATED_BODY()

public:
	AUSPlayerCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	FORCEINLINE UUSCurrencyComponent* GetCurrencyComponent() const { return CurrencyComponent; }
	FORCEINLINE UUSAimComponent* GetAimComponent() const { return AimComponent; }
	FORCEINLINE UUSPlayerInputComponent* GetPlayerInputComponent() const { return PlayerInputComponent; }
	FORCEINLINE UUSExperienceComponent* GetExperienceComponent() const { return ExperienceComponent; }
	FORCEINLINE UUSSquadComponent* GetSquadComponent() const { return SquadComponent; }

	/** 픽업 자석 획득반경 */
	FORCEINLINE float GetMagnetRadius() const { return MagnetRadius; }

	/** 보상 획득 반경에 배율을 곱한다 (보급선 자석 상품) */
	void ScaleMagnetRadius(float Multiplier) { MagnetRadius *= FMath::Max(0.0f, Multiplier); }

	/** 걷기 이속 (달리기 전. 격납고 사양표가 CDO에서 읽는다) */
	FORCEINLINE float GetNormalSpeed() const { return NormalSpeed; }

	bool CanToggleCombat() const;

	/**
	 * 카메라를 지금 자리에 세운다(true) / 스프링암에 다시 붙인다(false). 스테이지 이탈·진입 비행 동안 기체가 화면 밖으로 나가고 들어오는 게 보이게.
	 * 뷰 타깃을 다른 카메라로 바꾸지 않고 같은 카메라 컴포넌트를 떼어 두므로 카메라 설정(시야각·후처리)이 그대로다
	 */
	void SetCameraFrozen(bool bFrozen);

	/** 화면 위쪽 방향 (카메라 앞 방향을 바닥에 눕힌 단위 벡터). 카메라가 없으면 월드 +X */
	FVector GetScreenUpDirection() const;

	/** 카메라 시야각 = 원래 값 + Degrees (워프 줌아웃). 0이면 원래대로 */
	void SetCameraFovOffset(float Degrees);

protected:
	/** 사망 처리: 공통 래그돌에 더해 커서 조준 갱신을 멈춘다 */
	virtual void HandleDeath() override;

	/** 스탯(배율) 변경 시 이속 재계산 */
	virtual void HandleStatsChanged() override;

	/** 조준점(자동 조준이면 표적, 아니면 커서)을 구해 공통 TryFire로 넘긴다 */
	void TryFireAtAim(const FGameplayTag& AttackType);

	/** 현재 달리기 상태와 이속 배율을 반영해 MaxWalkSpeed를 갱신 */
	void RefreshMoveSpeed();

	/** 이동 속도에 비례해 스프링암 끝(TargetOffset, 월드 기준)을 이동 방향으로 옮긴다 */
	void UpdateCameraLead(float DeltaTime);

	/** 레벨업 시 호출 (ExperienceComponent::OnLevelUp에 바인딩) */
	void HandleLevelUp();

#pragma region InputSystem

public:
	/** Movement — 카메라가 고정이므로 카메라 yaw 기준(사실상 월드 축)으로 이동 */
	void Input_Move(const FInputActionValue& InputValue);

	/** 마우스 휠 줌 (카메라 거리 조절) */
	void Input_Zoom(const FInputActionValue& InputValue);

	void Input_Run();
	void StopRunning();

	/** Combat: 조준점(자동 조준이면 표적, 아니면 커서)으로 연사 한 번 시도 (간격은 TryFire가 지킨다) */
	void RapidAttack();

	/** 발사 버튼을 누르고 있는 동안 (자동 사격이 꺼져 있을 때만 쏜다) */
	void Input_Fire();

	/** 발사 버튼을 누른 순간: 커서 아래 적을 수동 표적으로 고정, 빈 곳이면 해제 (자동 조준일 때) */
	void Input_SelectTarget();

	/** 근처 보급선과 상호작용 (상점 열기) */
	void Input_Interact();

#pragma endregion

protected:
	UPROPERTY(EditAnywhere, Category = Sprinting)
	float RunSpeed = 600.0f;

	UPROPERTY(EditAnywhere, Category = Sprinting)
	float NormalSpeed = 300.0f;

	/** 이속이 늘어난 비율의 몇 배만큼 추진 불꽃을 키우는가 (3 = 이속 +5% → 불꽃 ×1.15, +25% → ×1.75). 1이면 이속 배율 그대로라 눈에 안 띈다 */
	UPROPERTY(EditAnywhere, Category = Sprinting, meta = (ClampMin = "0.0"))
	float ThrusterSizePerMoveSpeed = 3.0f;

	UPROPERTY(EditAnywhere, Category = Camera)
	float RunFieldOfVision = 120.0f;

	UPROPERTY(EditAnywhere, Category = Camera)
	float NormalFieldOfVision = 90.0f;

	/** 자동 사격: 켜면 입력 없이 조준점으로 계속 연사한다 (뱀서식 최소 조작, D6). 자동 조준이면 표적이 있을 때만. 끄면 왼쪽 버튼을 누르는 동안만 쏜다 */
	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAutoFire = true;

	/** 표적이 없을 때 기수가 이동 방향으로 도는 속도 (도/초). 이동 자체는 입력 방향으로 바로 한다. 0 이하면 즉시 돈다 */
	UPROPERTY(EditAnywhere, Category = Movement, meta = (ClampMin = "0.0"))
	float MoveTurnRate = 540.0f;

	/** 몸(기수)이 조준 방향에서 이 각도(도) 안으로 돌아왔을 때만 쏜다. 고정 전방 화기라 옆·뒤로 탄이 나가지 않게 (부하 FireAtTarget과 같은 규칙) */
	UPROPERTY(EditAnywhere, Category = Combat, meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float FireConeHalfAngle = 25.0f;

	/** 쿼터뷰 카메라 거리 (시작값). BeginPlay에서 SpringArm에 적용 — BP의 SpringArm 값 대신 이걸 튜닝한다 */
	UPROPERTY(EditAnywhere, Category = Camera)
	float NormalArmLength = 600.0f;

	/** 쿼터뷰 카메라 내려다보는 각도 (음수 = 아래로). BeginPlay에서 SpringArm에 적용 */
	UPROPERTY(EditAnywhere, Category = Camera)
	float CameraPitch = -55.0f;

	/** 카메라 앞당김: 달리기 최고 속도일 때 화면 중심을 이동 방향으로 이만큼(uu) 옮긴다. 0 = 끔 */
	UPROPERTY(EditAnywhere, Category = Camera, meta = (ClampMin = "0.0"))
	float CameraLeadDistance = 250.0f;

	/** 카메라 앞당김이 따라가는 빠르기 (VInterpTo 속도). 작을수록 늦게·부드럽게 */
	UPROPERTY(EditAnywhere, Category = Camera, meta = (ClampMin = "0.0"))
	float CameraLeadInterpSpeed = 2.5f;

	/** 마우스 휠 줌 최소 거리 */
	UPROPERTY(EditAnywhere, Category = Camera)
	float MinArmLength = 400.0f;

	/** 마우스 휠 줌 최대 거리 */
	UPROPERTY(EditAnywhere, Category = Camera)
	float MaxArmLength = 900.0f;

	/** 휠 한 칸당 줌 거리 변화량 */
	UPROPERTY(EditAnywhere, Category = Camera)
	float ZoomStep = 50.0f;

	/** 픽업(젬) 자석 획득반경 */
	UPROPERTY(EditAnywhere, Category = Pickup)
	float MagnetRadius = 200.0f;

	/** 사망 몽타주 */
	UPROPERTY(EditAnywhere, Category = Combat)
	TObjectPtr<UAnimMontage> DeathMontage;

	/** 레벨업음 */
	UPROPERTY(EditAnywhere, Category = Sound)
	TObjectPtr<USoundBase> LevelUpSound;

	/** 배경 음악 (루프 사운드 권장) */
	UPROPERTY(EditAnywhere, Category = Sound)
	TObjectPtr<USoundBase> BackgroundMusic;

	/** 골드 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSCurrencyComponent> CurrencyComponent;

	/** 커서 조준 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSAimComponent> AimComponent;

	/** 플레이어 입력(매핑 컨텍스트/액션 바인딩) 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSPlayerInputComponent> PlayerInputComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSExperienceComponent> ExperienceComponent;

	/** 부하 드론 분대·진형 관리 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSSquadComponent> SquadComponent;

	/** 감속 미사일 능력 (레벨업 카드로 배우기 전엔 쉰다) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSMissileBarrageComponent> MissileBarrageComponent;

	/** 표적 지정 능력: 대장이 맞힌 적이 부하에게 더 아프게 맞는다 (배우기 전엔 무기를 구독하지 않는다) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSTargetMarkComponent> TargetMarkComponent;

	/** 클릭으로 고정한 수동 표적 발밑의 링 (메시·머티리얼은 BP에서) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSTargetRingComponent> TargetRing;

	/** 감속 미사일이 터진 자리에 감속 범위 고리를 그리는 판 풀 (머티리얼은 BP에서) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UUSRingPulsePoolComponent> SlowRing;

	/** 카메라 관련 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCameraComponent> Camera;

private:
	/** 달리기 중 여부 */
	bool bIsRunning = false;

	/** 지금 적용 중인 카메라 앞당김 (월드) */
	FVector CameraLeadOffset = FVector::ZeroVector;

	/** SetCameraFovOffset 기준 시야각 (처음 부를 때 기억) */
	float BaseFieldOfView = 90.0f;
	bool bBaseFieldOfViewCached = false;
};
