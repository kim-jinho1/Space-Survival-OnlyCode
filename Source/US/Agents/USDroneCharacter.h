#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Agents/USAgent.h"
#include "USDroneCharacter.generated.h"

class UUSStateComponent;
class UUSCombatComponent;
class AUSWeapon;
class USceneComponent;
class UStaticMeshComponent;
class UUSHoverVisualComponent;
class UUSThrusterFXComponent;
class UUSDeathExplosionComponent;
class UUSHitFlashComponent;
class AController;
class UMaterialInterface;
class UStaticMesh;

/**
 * 드론 공통 캐릭터. 대장(플레이어)과 부하가 같이 쓴다.
 * 상태, 전투(무기 장착·발사), 사망 폭발을 담당한다. 비주얼은 드론 스태틱 메시(VisualMesh) + HoverVisual.
 * 입력·카메라·조준 방식은 자식이 정한다 — 이 클래스는 "어디를 쏠지"를 받기만 한다.
 */
UCLASS()
class US_API AUSDroneCharacter : public AUSAgent
{
	GENERATED_BODY()

public:
	AUSDroneCharacter();

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	FORCEINLINE UUSStateComponent* GetStateComponent() const { return StateComponent; }
	FORCEINLINE UUSCombatComponent* GetCombatComponent() const { return CombatComponent; }

	/** 사망 상태인가 (StateComponent의 Death 태그) */
	bool IsDeath() const;

	/** 공격 가능 상태인가 (사망·구르기·일반행동·피격 중이 아닐 때) */
	bool CanAttack() const;

	/** 조준 포즈 여부. 드론은 항상 무기를 겨누므로 무기를 들고 있으면 true */
	bool IsAiming() const;

	/** 무기는 드론 코끝 WeaponMount에 붙는다 */
	virtual USceneComponent* GetEquipmentAttachParent() const override;

	/** MuzzleOffsets를 순환해 WeaponMount 기준 포구를 월드 좌표로 돌려준다 (좌우 번갈아 발사) */
	virtual bool GetMuzzleWorldLocation(int32 ShotIndex, FVector& OutLocation) const override;

	/** 기본 연사 간격 (공속 배율 반영 전. 격납고 사양표가 CDO에서 읽는다) */
	FORCEINLINE float GetRapidFireInterval() const { return RapidFireInterval; }

	/** 외형 메시 (격납고 3D 무대가 CDO에서 메시·보정 위치·재질을 읽는다) */
	FORCEINLINE const UStaticMeshComponent* GetVisualMesh() const { return VisualMesh; }

	/** 외형 메시 슬롯 이름 → 덮어쓸 재질 */
	FORCEINLINE const TMap<FName, TObjectPtr<UMaterialInterface>>& GetSlotMaterials() const { return SlotMaterials; }

	/** 둥실·기울기 / 추진 불꽃 연출 (타이틀 기체가 CDO의 설정을 그대로 복사해 쓴다) */
	FORCEINLINE const UUSHoverVisualComponent* GetHoverVisual() const { return HoverVisual; }
	FORCEINLINE const UUSThrusterFXComponent* GetThruster() const { return Thruster; }

	/** 공속 배율을 반영한 실제 연사 간격 (간격 / 공속배율) */
	float GetEffectiveRapidFireInterval() const;

	/**
	 * 조준점을 향해 발사한다. 연사 간격·공격 가능 상태를 검사하고 무기 Fire를 호출한다.
	 * 대장은 커서 조준점을, 부하는 타겟 위치를 넘긴다.
	 */
	void TryFire(const FVector& AimLocation, const FGameplayTag& AttackType);

	/**
	 * 기체 외형 메시를 바꾼다 (업그레이드 진화). 모델마다 슬롯 순서가 달라 덮어쓴 재질을 비우고 SlotMaterials를 슬롯 이름으로 다시 입힌다.
	 * ExtraSlotMaterials는 그 위에 이 모델에만 덮어쓸 재질. RelativeTransform은 VisualRoot 기준 (모델 피벗 보정·방향·크기).
	 */
	void SetVisualMesh(UStaticMesh* NewMesh, const FTransform& RelativeTransform, const TMap<FName, TObjectPtr<UMaterialInterface>>& ExtraSlotMaterials);

protected:
	/** 시작 시 기본 무기를 스폰하여 자동 장착 */
	void EquipDefaultWeapon();

	/** 사망 처리: 상태 Death, 기체를 숨기고 연출(둥실·추진 불꽃)을 멈춘 뒤 폭발 */
	virtual void HandleDeath() override;

	/** 스탯(배율) 변경 시 호출 (AttributeComponent::OnStatsChanged에 바인딩). 자식이 이속 등을 갱신 */
	virtual void HandleStatsChanged();

	/** 피격 시 기체를 잠깐 하얗게 번쩍인다 (AttributeComponent::OnDamaged에 바인딩) */
	void HandleDamaged(float Damage, AController* InstigatedBy);

	/** SlotMaterials에 지정한 VisualMesh 슬롯들의 머티리얼을 바꾼다 */
	void ApplySlotMaterials();

	/** Materials(슬롯 이름 → 재질)를 VisualMesh에 입힌다. 없는 슬롯은 Warning */
	void ApplySlotMaterialMap(const TMap<FName, TObjectPtr<UMaterialInterface>>& Materials);

	/** 시작 시 자동 장착할 기본 무기 클래스 */
	UPROPERTY(EditAnywhere, Category = "Combat")
	TSubclassOf<AUSWeapon> DefaultWeaponClass;

	/** 연사 발사 간격 */
	UPROPERTY(EditAnywhere, Category = "Combat")
	float RapidFireInterval = 0.12f;

	/** 포구 위치 목록 (WeaponMount 기준). 발사마다 순서대로 돌아가며 쏜다. 비어 있으면 WeaponMount 한 점 */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TArray<FVector> MuzzleOffsets;

	/** VisualMesh 슬롯 이름 → 덮어쓸 머티리얼 (금속 몸체, 편 구분 포인트 등). 모델마다 슬롯 순서가 달라 번호 대신 이름으로 찾는다. 없는 슬롯은 모델 원래 머티리얼 */
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TMap<FName, TObjectPtr<UMaterialInterface>> SlotMaterials;

	/** 격추 섬광·충격파 고리 반경(uu). 적(AUSEnemyCharacter::DeathBurstRadius)과 같은 연출, 색은 재질이 정한다 */
	UPROPERTY(EditDefaultsOnly, Category = "Visual", meta = (ClampMin = "0.0"))
	float DeathBurstRadius = 110.0f;

	/** 캐릭터 상태 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSStateComponent> StateComponent;

	/** 전투 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSCombatComponent> CombatComponent;

	/** 드론 외형의 회전·둥실 중심. HoverVisual이 이 컴포넌트를 움직인다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> VisualRoot;

	/** 드론 외형 메시. 모델 피벗이 중심이 아니면 BP에서 상대 위치로 보정해 VisualRoot가 중심에 오게 한다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	/** 둥실·기울기 연출 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSHoverVisualComponent> HoverVisual;

	/** 추진 불꽃 연출. VisualRoot에 붙어 둥실·기울기를 따라간다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSThrusterFXComponent> Thruster;

	/** 사망 폭발 연출. 이펙트·사망음은 BP에서 지정한다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSDeathExplosionComponent> DeathExplosion;

	/** 격추 순간 섬광 + 퍼지는 고리 (판 1장, 재질은 BP에서). 재질이 없으면 그리지 않는다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UUSRingPulsePoolComponent> DeathBurstPool;

	/** 피격 발광. VisualMesh 재질이 Custom Primitive Data의 맞은 시각으로 번쩍임을 줄여 간다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSHitFlashComponent> HitFlash;

	/** 무기가 붙는 지점이자 발사점. VisualRoot 아래라 둥실·기울기를 따라간다. BP에서 기체 코끝에 둔다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> WeaponMount;

private:
	/** 마지막 발사 시각 */
	float LastFireTime = 0.0f;
};
