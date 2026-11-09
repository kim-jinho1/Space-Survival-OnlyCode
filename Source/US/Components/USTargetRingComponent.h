#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "USTargetRingComponent.generated.h"

class AActor;
class UMaterialInterface;
class UUSAimComponent;

/**
 * 표적 링 (D6, 조준 화면 표시): 대장이 클릭으로 고정한 적의 발밑에 링을 깐다.
 * bShowOnAutoTarget을 켜면 자동으로 고른 적에도 깐다 (Archero식). 기본은 꺼짐 — 사용자 결정 2026-10-21: 링은 "내가 찍은 표적" 표시.
 * 둘 다 켤 때는 클릭 표적을 ManualRadiusMultiplier만큼 크게, ManualMaterial이 있으면 그 머티리얼로 구별한다.
 * 대장이 가진 링 하나를 표적의 루트에 붙였다 뗀다 — 표적을 따라다니는 데 Tick이 필요 없고, 적마다 링을 두지 않는다.
 * 같은 주인의 UUSAimComponent::OnAimTargetChanged를 받아 켜고 끈다. 판 메시·도는 모양은 BP의 메시·머티리얼이 정한다.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class US_API UUSTargetRingComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UUSTargetRingComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	/** 조준 표적 변경 알림 (UUSAimComponent::OnAimTargetChanged) */
	void HandleAimTargetChanged(AActor* NewTarget, bool bManual);

	/** Target 발밑에 링을 붙이고 보인다 */
	void ShowOn(AActor* Target, bool bManual);

	/** 링을 숨기고 주인에게 되돌려 붙인다 */
	void HideRing();

	/**
	 * 링 반지름 = 표적 크기 × 이 배율 (적 종류마다 크기가 달라서 고정 크기 대신 비율).
	 * 표적 크기는 충돌 반지름과 기체 메시의 수평 반폭 중 큰 쪽 — 기체가 캡슐보다 커서 링이 기체에 가려지던 문제
	 */
	UPROPERTY(EditDefaultsOnly, Category = "TargetRing", meta = (ClampMin = "0.1"))
	float RadiusScale = 1.2f;

	/** 자동으로 고른 표적에도 링을 깐다 */
	UPROPERTY(EditDefaultsOnly, Category = "TargetRing")
	bool bShowOnAutoTarget = false;

	/** 클릭으로 고정한 표적의 링 크기 배율 (자동 표적과 구별) */
	UPROPERTY(EditDefaultsOnly, Category = "TargetRing", meta = (ClampMin = "0.1"))
	float ManualRadiusMultiplier = 1.0f;

	/** 클릭으로 고정한 표적의 링 머티리얼. 비우면 자동 표적과 같은 머티리얼 */
	UPROPERTY(EditDefaultsOnly, Category = "TargetRing")
	TObjectPtr<UMaterialInterface> ManualMaterial;

	/** 표적 발밑에서 띄우는 높이 (바닥과 같은 높이면 깜빡인다) */
	UPROPERTY(EditDefaultsOnly, Category = "TargetRing")
	float GroundOffset = 3.0f;

private:
	/** 구독 중인 조준 컴포넌트 */
	TWeakObjectPtr<UUSAimComponent> BoundAim;

	FDelegateHandle AimTargetChangedHandle;

	/** BP에 지정된 기본(자동 표적) 머티리얼 — 수동 머티리얼에서 되돌릴 때 쓴다 */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> AutoMaterial;
};
