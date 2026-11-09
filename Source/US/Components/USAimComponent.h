#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USAimComponent.generated.h"

class AActor;
class ACharacter;
class APlayerController;

/** 조준 표적이 바뀜 (새 표적, 없으면 nullptr / 클릭으로 고정한 표적인가) — 표적 링이 받는다 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDelegateOnAimTargetChanged, AActor*, bool);

/**
 * 쿼터뷰 조준 컴포넌트.
 * 자동 조준(기본, D6): 반경 안의 가장 가까운 적, 또는 플레이어가 클릭한 수동 표적을 조준한다.
 * 커서 조준(bAutoAim false): 매 프레임 마우스 커서를 캐릭터 발 높이의 수평면에 투영해 조준점을 구한다.
 * 어느 쪽이든 소유 캐릭터의 yaw와 컨트롤 회전을 조준 방향으로 맞춘다.
 * 조준점은 무기(AUSWeapon::Fire)가, 컨트롤 회전은 애님 Aim Offset(GetBaseAimRotation)이 읽는다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class US_API UUSAimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUSAimComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 이번 프레임에 유효한 조준점이 있는지 (자동 조준이면 표적이 없을 때, 커서 조준이면 커서가 뷰포트 밖일 때 false) */
	FORCEINLINE bool HasValidAim() const { return bHasValidAim; }

	/** 조준점 (자동 조준이면 표적 위치, 커서 조준이면 캐릭터 발 높이 수평면 위의 커서 위치) */
	FORCEINLINE const FVector& GetAimLocation() const { return AimLocation; }

	/** 캐릭터 → 조준점 수평 단위 벡터. 조준점이 캐릭터에 너무 가까우면 마지막 유효 방향 유지 */
	FORCEINLINE const FVector& GetAimDirection() const { return AimDirection; }

	/** 자동 조준 중인가 (false면 커서 조준) */
	FORCEINLINE bool IsAutoAim() const { return bAutoAim; }

	/** 지금 조준 중인 적: 수동 표적이 있으면 그것, 없으면 자동으로 고른 적 (없으면 nullptr) */
	AActor* GetTarget() const;

	/** 클릭으로 고정한 수동 표적 (없거나 죽었으면 nullptr). 부하 AI가 1순위로 쏜다 */
	FORCEINLINE AActor* GetManualTarget() const { return ManualTarget.Get(); }

	/**
	 * 커서 아래의 적을 수동 표적으로 고정한다 (클릭). 커서가 적 외형(+ClickPickRadius) 위에 없으면 수동 표적을 풀고 자동 조준으로 돌아간다.
	 * 표적은 죽을 때까지 고정한다. 화면 밖에 있거나 날아가는 탄으로 죽을 예정인 동안은 자동 표적을 쏘고, 다시 쓸 수 있으면 돌아온다.
	 */
	void SelectTargetUnderCursor();

	/**
	 * 실제로 노리는 표적(GetTarget)이 바뀔 때마다. 자동으로 고른 적도 알린다 —
	 * 클릭하지 않는 플레이어도 누구를 쏘는지 보여야 한다 (플레이테스트 2026-10-20)
	 */
	FDelegateOnAimTargetChanged OnAimTargetChanged;

protected:
	/** 커서를 수평면에 투영해 AimLocation / AimDirection을 갱신 */
	void UpdateAimFromCursor();

	/** 표적 위치로 AimLocation / AimDirection을 갱신. 표적이 없으면 조준 무효 */
	void UpdateAimFromTarget();

	/** 커서 레이를 캐릭터 몸 높이 수평면에 투영한다. 커서가 뷰포트 밖이면 false */
	bool TryGetCursorPlaneLocation(FVector& OutLocation) const;

	/** 자동 표적을 다시 고르고, 쓸 수 없게 된 수동 표적은 푼다 (RetargetInterval 타이머) */
	void RefreshAutoTarget();

	/**
	 * Target이 살아 있는 화면 안 적이고 AutoAimRange 안에 있는가. 거리·화면은 몸 가장자리(GetClosestPickPoint) 기준.
	 * 클릭 표적(또는 bIgnoreRange)은 거리를 보지 않는다 — 화면에 보이는데 클릭이 거절되면 안 된다 (플레이테스트 2026-10-21)
	 */
	bool IsTargetUsable(const AActor* Target, bool bIgnoreRange = false) const;

	/** 노리는 표적이나 수동 여부가 지난번 알림과 다르면 OnAimTargetChanged를 알린다 */
	void NotifyAimTargetIfChanged();

	/** AimLocation 기준으로 수평 방향을 갱신 (MinAimDistance보다 가까우면 마지막 방향 유지) */
	void UpdateAimDirection();

	/** 캐릭터 yaw와 컨트롤 회전을 AimDirection 쪽으로 보간 */
	void UpdateOwnerRotation(float DeltaTime);

	/** 조준점이 이 거리보다 가까우면 방향을 갱신하지 않는다 (커서가 캐릭터 위에 있을 때 방향이 튀는 것 방지) */
	UPROPERTY(EditAnywhere, Category = "Aim")
	float MinAimDistance = 30.0f;

	/** 조준 방향으로 캐릭터가 회전하는 속도 (도/초). 0 이하면 즉시 회전 */
	UPROPERTY(EditAnywhere, Category = "Aim")
	float TurnRate = 900.0f;

	/** 자동 조준: 가장 가까운 적(또는 클릭한 표적)을 조준한다. 끄면 커서 조준 */
	UPROPERTY(EditAnywhere, Category = "Aim|Auto")
	bool bAutoAim = true;

	/** 자동 조준 반경 (대장 기준). 카메라 2000·FOV 90에서 화면 좌우 끝이 약 2000이라 그 안쪽 대부분을 덮게. 화면 밖은 따로 거른다 */
	UPROPERTY(EditAnywhere, Category = "Aim|Auto", meta = (ClampMin = "0.0", EditCondition = "bAutoAim"))
	float AutoAimRange = 1600.0f;

	/** 클릭 지점이 적 외형 범위 안이거나 범위에서 이 거리 안이면 표적으로 고른다 (작고 빠른 적을 정확히 클릭하기 어려워서) */
	UPROPERTY(EditAnywhere, Category = "Aim|Auto", meta = (ClampMin = "0.0", EditCondition = "bAutoAim"))
	float ClickPickRadius = 150.0f;

	/** 자동 표적을 다시 고르는 간격(초). 적이 많아 매 프레임 검색하지 않는다 */
	UPROPERTY(EditAnywhere, Category = "Aim|Auto", meta = (ClampMin = "0.02", EditCondition = "bAutoAim"))
	float RetargetInterval = 0.1f;

private:
	/** 소유 캐릭터·컨트롤러 캐시 (BeginPlay에서 설정, 컨트롤러는 빙의가 늦을 수 있어 Tick에서 재확인) */
	TObjectPtr<ACharacter> OwnerCharacter;
	TObjectPtr<APlayerController> PlayerController;

	/** 클릭으로 고정한 표적. 적이 사라지면 자동으로 비도록 약한 참조 */
	TWeakObjectPtr<AActor> ManualTarget;

	/** 타이머가 고른 가장 가까운 적 */
	TWeakObjectPtr<AActor> AutoTarget;

	/** 마지막으로 알린 표적과 수동 여부 (NotifyAimTargetIfChanged) */
	TWeakObjectPtr<AActor> NotifiedTarget;
	bool bNotifiedManual = false;

	/** 자동 표적 재선택 타이머 */
	FTimerHandle RetargetTimerHandle;

	FVector AimLocation = FVector::ZeroVector;
	FVector AimDirection = FVector::ForwardVector;
	bool bHasValidAim = false;
};
