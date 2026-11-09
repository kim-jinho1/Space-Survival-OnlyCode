#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "USSupplyShip.generated.h"

class APawn;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UUSHoverVisualComponent;
class UUSRingPulsePoolComponent;
class UWidgetComponent;

/** 보급선 상태. 없음 → 도착 예고 → 머무름 → 출발 예고 → 없음 순서로 돈다 */
UENUM()
enum class EUSSupplyShipState : uint8
{
	/** 경기장에 없음 (숨김, 상호작용 불가) */
	Absent,
	/** 곧 도착 (아직 없음) */
	Arriving,
	/** 머무는 중 (상점 이용 가능) */
	Present,
	/** 곧 떠남 (아직 이용 가능) */
	Departing
};

/** 보급선 상태가 바뀜 (새 상태, 다음 상태까지 남은 초) */
DECLARE_MULTICAST_DELEGATE_TwoParams(FDelegateOnSupplyStateChanged, EUSSupplyShipState, float);

/**
 * 보급선. 경기장에 머물렀다 떠났다를 반복하고, 머무는 동안 대장이 가까이 와서 상호작용하면 상점을 연다.
 * 도착할 때마다 대장 주변 링 위의 새 위치에 나타난다. 상태 전환은 타이머로만 한다.
 * 도착·출발 때는 기체가 짧게 커지며 나타나고 줄며 사라지는 워프 연출과 바닥 고리 섬광을 낸다 — Tick은 연출하는 동안만 켠다.
 * 상점 UI와 판매 로직은 AUSPlayerController(UUSShopComponent)가 맡고, 이 클래스는 위치·상태·상호작용만 담당한다.
 */
UCLASS()
class US_API AUSSupplyShip : public AActor
{
	GENERATED_BODY()

public:
	AUSSupplyShip();

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** 플레이어가 상호작용 → 해당 플레이어의 컨트롤러로 상점 열기 요청 (머무는 중일 때만) */
	void Interact(APawn* InteractingPawn);

	/** 상점을 이용할 수 있고 Location이 상호작용 거리 안인가 (수평 거리) */
	bool IsInInteractRange(const FVector& Location) const;

	/** 경기장에 떠 있어 상점을 이용할 수 있는가 (머무름·출발 예고) */
	bool IsAvailable() const;

	FORCEINLINE EUSSupplyShipState GetState() const { return State; }

	/** 떠날 때까지 남은 초 (머무름·출발 예고 중). 떠나 있으면 0 */
	float GetStayTimeRemaining() const;

	/** 한 번 머무는 전체 시간 (초, 출발 예고 포함) — 남은 시간 막대의 분모 */
	FORCEINLINE float GetStayDuration() const { return PresentDuration; }

	/** 상태 변화 알림 (안내 문구·방향 표시기·미니맵) */
	FDelegateOnSupplyStateChanged OnSupplyStateChanged;

protected:
	/** 상태를 바꾸고 알린 뒤, 다음 상태로 넘어갈 타이머를 건다 */
	void EnterState(EUSSupplyShipState NewState, float Duration);

	/** 타이머 → 다음 상태로 */
	void AdvanceState();

	/** 대장 주변 링 위의 새 위치로 옮긴다. 자리를 못 찾으면 Warning 후 제자리 */
	void Relocate();

	/** 워프 연출 시작 (bIn = 나타남). 끝나면 Tick을 끄고, 사라짐이면 숨긴다 */
	void PlayWarp(bool bIn);

	/** 실제로 쓰는 예고 시간 (머무는·떠나 있는 시간을 넘지 않게 제한) */
	float GetNoticeLeadTime() const;

	/** 판 시작 뒤 첫 도착까지 걸리는 시간 (초, 도착 예고 포함). 0이면 맵에 배치된 자리에서 바로 머무는 상태로 시작한다 */
	UPROPERTY(EditAnywhere, Category = "Supply", meta = (ClampMin = "0.0"))
	float FirstArrivalDelay = 30.0f;

	/** 한 번 도착해서 머무는 시간 (초, 출발 예고 포함) */
	UPROPERTY(EditAnywhere, Category = "Supply", meta = (ClampMin = "1.0"))
	float PresentDuration = 45.0f;

	/** 떠나 있는 시간 (초, 도착 예고 포함) */
	UPROPERTY(EditAnywhere, Category = "Supply", meta = (ClampMin = "1.0"))
	float AbsentDuration = 45.0f;

	/** 도착·출발을 미리 알리는 시간 (초). 머무는·떠나 있는 시간보다 짧아야 한다 */
	UPROPERTY(EditAnywhere, Category = "Supply", meta = (ClampMin = "0.0"))
	float NoticeLeadTime = 10.0f;

	/** 새 위치의 대장으로부터 최소 거리 */
	UPROPERTY(EditAnywhere, Category = "Supply", meta = (ClampMin = "0.0"))
	float RelocateRadiusMin = 1500.0f;

	/** 새 위치의 대장으로부터 최대 거리 */
	UPROPERTY(EditAnywhere, Category = "Supply", meta = (ClampMin = "0.0"))
	float RelocateRadiusMax = 2500.0f;

	/** 바닥에서 띄우는 높이 (기체 중심) */
	UPROPERTY(EditAnywhere, Category = "Supply")
	float FloorHeightOffset = 90.0f;

	/** 이 거리(수평) 안에서 상호작용할 수 있다. 기체가 커서 대장 쪽 상호작용 거리 대신 보급선이 정한다 */
	UPROPERTY(EditAnywhere, Category = "Supply", meta = (ClampMin = "0.0"))
	float InteractRadius = 400.0f;

	/** 워프 연출 길이 (초). 0이면 연출 없이 바로 나타나고 사라진다 */
	UPROPERTY(EditAnywhere, Category = "Supply|Warp", meta = (ClampMin = "0.0"))
	float WarpDuration = 0.35f;

	/** 워프 고리 반경 (상호작용 거리 배수) */
	UPROPERTY(EditAnywhere, Category = "Supply|Warp", meta = (ClampMin = "0.0"))
	float WarpPulseRadiusScale = 1.5f;

	/** 머리 위 표시 높이 (기체 중심 기준) */
	UPROPERTY(EditAnywhere, Category = "Supply|Marker")
	float MarkerHeight = 220.0f;

	/** 액터 루트. 상호작용 범위를 눈으로 보기 위한 구 (충돌 없음) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionSphere;

	/** 둥실거림 회전 중심. 메시는 이 아래에 둔다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> VisualRoot;

	/** 표시용 메시 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	/** 바닥 고리. 지름 = 상호작용 거리라 "이 안에 들어오면 상점"을 바로 보여 준다. 둥실거림과 무관하게 바닥에 붙인다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> RangeRing;

	/** 머리 위 표시 (이름·떠날 때까지 남은 시간). 위젯 클래스는 BP에서 정한다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> MarkerWidget;

	/** 도착·출발 때 바닥 고리 섬광. 재질은 BP에서 정한다 (비면 고리 없이 크기 연출만) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSRingPulsePoolComponent> WarpPulse;

	/** 드론과 같은 둥실거림 연출 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSHoverVisualComponent> HoverVisual;

private:
	FTimerHandle StateTimerHandle;

	/** 워프 연출 경과 (초) */
	float WarpElapsed = 0.0f;

	/** 지금 연출이 나타남인가 (아니면 사라짐) */
	bool bWarpIn = true;

	/** 현재 상태. 시작은 맵에 배치된 자리에서 머무는 중 */
	EUSSupplyShipState State = EUSSupplyShipState::Present;
};
