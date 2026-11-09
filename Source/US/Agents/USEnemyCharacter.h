#pragma once

#include "CoreMinimal.h"
#include "USAgent.h"
#include "USEnemyCharacter.generated.h"

class UUSAttributeComponent;
class UUSHoverVisualComponent;
class UUSHitFlashComponent;
class UUSWorldHealthBarComponent;
class UUSDeathExplosionComponent;
class UUSCombatFeedbackSubsystem;
class UUSEnemyTypeData;
class USceneComponent;
class UStaticMeshComponent;
class UMaterialInterface;
class UAnimMontage;
class UParticleSystemComponent;

/** 적 사망 알림 (사망한 적 자신) — 스포너가 생존 수를 관리하는 데 사용 */
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateOnEnemyDeath, class AUSEnemyCharacter*);

/**
 * 플레이어를 직선으로 추적하고, 접촉 시 주기적으로 데미지를 가한다.
 * 체력/피격/사망은 플레이어와 동일한 UUSAttributeComponent를 재사용한다.
 */
UCLASS()
class US_API AUSEnemyCharacter : public AUSAgent
{
	GENERATED_BODY()

public:
	AUSEnemyCharacter();

protected:
	virtual void PostInitializeComponents() override;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void Tick(float DeltaTime) override;

public:
	/** 생존 여부 */
	bool IsAlive() const;

	/**
	 * 날아오는 중인 피해를 더하거나(+) 뺀다(-). 무기가 볼트 도착 때 피해를 주므로(AUSWeapon::FireShot),
	 * 그동안 같은 적을 계속 노려 탄을 낭비하지 않도록 조준·표적 검색이 IsDoomed를 본다
	 */
	void AddPendingDamage(float Amount);

	/** 날아오는 피해만으로 죽을 적인가 (살아 있지만 새로 노릴 필요 없음) */
	bool IsDoomed() const;

	/** 클릭 판정용 외형 범위(월드 AABB). 캡슐은 기체보다 훨씬 작아서 외형 메시 범위를 쓴다. 메시가 없으면 캡슐 범위 */
	FBox GetPickBounds() const;

	/**
	 * 외형 범위(GetPickBounds)에서 From에 가장 가까운 점 (높이는 범위 가운데). 사거리·화면 판정을 중심 대신 가장자리로 하려고 쓴다 —
	 * 보스처럼 큰 적은 몸이 화면·사거리 안에 있어도 중심이 밖이라 표적이 자꾸 풀렸다 (플레이테스트 2026-10-27)
	 */
	FVector GetClosestPickPoint(const FVector& From) const;

	/** 이동 속도를 Multiplier(0~1)배로 Duration초 동안 늦춘다. 겹치면 더 강한 감속을 쓰고 시간은 새로 건다 */
	void ApplySlow(float Multiplier, float Duration);

	/**
	 * 표적 지정: Duration초 동안 부하(플레이어가 아닌 아군)에게 받는 피해를 ×(1 + DamageBonus)로 키운다.
	 * 겹치면 더 큰 보너스를 쓰고 시간은 새로 건다 (감속과 같은 규칙)
	 */
	void ApplyMark(float DamageBonus, float Duration);

	/** 지금 표적 지정 상태인가 */
	FORCEINLINE bool IsMarked() const { return MarkDamageBonus > 0.0f; }

	/** 표적 지정 보너스를 피해에 반영한 뒤 기본 처리(OnTakeAnyDamage → 체력)로 넘긴다 */
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** 스폰 직후(FinishSpawning 전) 스포너가 호출. BeginPlay에서 종류의 스탯·외형과 체력 배율(경과 시간 성장)을 적용한다 */
	void SetEnemyType(const UUSEnemyTypeData* InEnemyType, float InHealthMultiplier);

	/** 이 적의 종류 (스포너가 넣은 데이터, 없으면 nullptr) */
	FORCEINLINE const UUSEnemyTypeData* GetEnemyType() const { return EnemyType; }

	/** 사망 시 호출되는 델리게이트 (스포너가 바인딩) */
	FDelegateOnEnemyDeath OnEnemyDeath;

protected:
	/** 현재 추적 대상 (가장 가까운 아군, RefreshTarget이 주기적으로 갱신) */
	FORCEINLINE AActor* GetTargetActor() const { return CachedTarget; }

	/** 팀 등록부에서 가장 가까운 아군을 골라 CachedTarget에 저장 (타이머로 주기 호출) */
	void RefreshTarget();

	/** 감속 종료 (타이머) */
	void ClearSlow();

	/** 표적 지정 종료 (타이머, 사망) */
	void ClearMark();

	/** 스폰 직후 외형을 키운다 (Tick, 커지는 동안만) */
	void UpdateSpawnScale(float DeltaTime);

	/** VisualMesh 덧씌우기를 지금 상태에 맞춘다. 덧씌우기는 한 장뿐이라 표적 지정(부하 피해 +)이 감속보다 앞선다 */
	void RefreshOverlay();

	/** MoveSpeed × 감속 배율을 이동 컴포넌트에 반영 */
	void RefreshMoveSpeed();

	/** 대상 방향으로 직선 이동 */
	void ChaseTarget(float DeltaTime);

	/** 접촉 범위 안에 대상이 있으면 데미지를 가한다 (타이머로 주기 호출). 기뢰는 자폭으로 바꾼다 */
	virtual void ApplyContactDamage();

	/** 대상이 접촉 범위(3D 거리) 안에 있는가 */
	bool IsTargetInContactRange(const AActor* Target) const;

	/** 사망 처리 (AttributeComponent::OnDeath에 바인딩) */
	virtual void HandleDeath() override;

	/** 사망 위치에 경험치/골드 젬을 스폰 */
	void SpawnRewards() const;

	/** 이번 죽음이 처치인가 (보상·처치 수 판단). 기뢰의 자폭은 처치가 아니다 */
	virtual bool ShouldCountAsKill() const;

	/** 사망 시 연쇄 폭발을 요청한다. 기본은 확률을 굴리고, 기뢰는 반드시 크게 터진다 */
	virtual void RequestChainExplosion();

	/** EnemyType의 스탯·보상·외형을 적용한다 (BeginPlay, 컴포넌트 초기화 뒤) */
	void ApplyEnemyType();

	/** EnemyType에 트레일이 있으면 VisualRoot 꼬리에 붙인다 (BeginPlay 한 번). 움직인 거리만큼 그려지므로 Tick 없음 */
	void SpawnTypeTrails();

	/** 피격 시 호출 (AttributeComponent::OnDamaged에 바인딩): 발광·피격음·체력 바, 데미지 숫자 보고, 막타 주인 기록 */
	void HandleDamaged(float Damage, AController* InstigatedBy);

	/** 피격·사망 연출 위치. 둥실 연출이 적용된 실제 기체 위치(VisualRoot) */
	FVector GetFeedbackLocation() const;

protected:
	/** 이동 속도 */
	UPROPERTY(EditAnywhere, Category = "Enemy|Move")
	float MoveSpeed = 150.0f;

	/** 접촉 시 가하는 데미지 */
	UPROPERTY(EditAnywhere, Category = "Enemy|Combat")
	float ContactDamage = 10.0f;

	/** 접촉 데미지 간격(초) */
	UPROPERTY(EditAnywhere, Category = "Enemy|Combat")
	float ContactDamageInterval = 1.0f;

	/** 접촉으로 판정하는 거리 (평면 기준) */
	UPROPERTY(EditAnywhere, Category = "Enemy|Combat")
	float ContactRange = 120.0f;

	/** 타겟 재탐색 주기(초). 매 프레임 찾지 않고 이 간격으로만 등록부를 훑는다 */
	UPROPERTY(EditAnywhere, Category = "Enemy|Combat")
	float TargetRefreshInterval = 0.25f;

	/** 사망 후 액터가 제거되기까지의 시간 */
	UPROPERTY(EditAnywhere, Category = "Enemy|Death")
	float DeathLifeSpan = 2.0f;

	/** 처치 시 지급할 경험치 */
	UPROPERTY(EditAnywhere, Category = "Enemy|Reward")
	float ExperienceReward = 10.0f;

	/** 처치 시 지급할 골드 */
	UPROPERTY(EditAnywhere, Category = "Enemy|Reward")
	int32 GoldReward = 5;

	/** 경험치 젬 클래스 (미지정 시 기본 AUSPickup 사용) */
	UPROPERTY(EditAnywhere, Category = "Enemy|Reward")
	TSubclassOf<class AUSPickup> ExperiencePickupClass;

	/** 골드 젬 클래스 (미지정 시 기본 AUSPickup 사용) */
	UPROPERTY(EditAnywhere, Category = "Enemy|Reward")
	TSubclassOf<class AUSPickup> GoldPickupClass;

	/** 사망 몽타주 (선택) */
	UPROPERTY(EditAnywhere, Category = "Enemy|Death")
	TObjectPtr<UAnimMontage> DeathMontage;

	/** 피격음 */
	UPROPERTY(EditAnywhere, Category = "Enemy|Sound")
	TObjectPtr<class USoundBase> HitSound;

	/** 피격음 음 높이 흔들기 폭 (0.1 = 0.9~1.1배). 같은 소리 연타가 기계적으로 들리지 않게 */
	UPROPERTY(EditAnywhere, Category = "Enemy|Sound", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float HitSoundPitchVariation = 0.1f;

	/** 표적 지정 중 VisualMesh 위에 덧씌울 머티리얼 (OverlayMaterial, 주황 깜빡임은 머티리얼이 그린다) */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Mark")
	TObjectPtr<UMaterialInterface> MarkOverlayMaterial;

	/** 감속 중 VisualMesh 위에 덧씌울 머티리얼 (파랑). 비우면 감속은 걸리지만 보이지 않는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Mark")
	TObjectPtr<UMaterialInterface> SlowOverlayMaterial;

	/** 스폰 직후 외형(VisualRoot)이 0에서 제 크기로 커지는 시간(초). 0이면 바로 제 크기 — 무리가 "뿅" 나타나지 않게 */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Visual", meta = (ClampMin = "0.0"))
	float SpawnScaleInTime = 0.3f;

	/** 격추 섬광·충격파 고리 반경(uu). 적 종류 VisualScale을 곱해 큰 적일수록 크게 터진다 */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy|Death", meta = (ClampMin = "0.0"))
	float DeathBurstRadius = 60.0f;

protected:
	/** 드론 외형의 회전·둥실 중심. HoverVisual이 이 컴포넌트를 움직인다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> VisualRoot;

	/** 드론 외형 메시 (적 종류 데이터의 메시). 충돌·피격은 캡슐이 담당 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	/** 둥실·기울기 연출 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSHoverVisualComponent> HoverVisual;

	/** 피격 발광 (VisualMesh) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSHitFlashComponent> HitFlash;

	/** 머리 위 체력 바 (맞았을 때만 잠시 표시) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSWorldHealthBarComponent> HealthBar;

	/** 사망 폭발 연출 (대장·부하와 같은 컴포넌트). 이펙트·사망음은 BP에서 지정한다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSDeathExplosionComponent> DeathExplosion;

	/** 격추 순간 섬광 + 퍼지는 고리 (판 1장, 재질은 BP에서). 재질이 없으면 그리지 않는다 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UUSRingPulsePoolComponent> DeathBurstPool;
private:
	/** 접촉 데미지 주기 타이머 */
	FTimerHandle ContactDamageTimerHandle;

	/** 타겟 재탐색 타이머 */
	FTimerHandle TargetRefreshTimerHandle;

	/** 감속 종료 타이머 */
	FTimerHandle SlowTimerHandle;

	/** 표적 지정 종료 타이머 */
	FTimerHandle MarkTimerHandle;

	/** 이 적의 종류 (스포너가 스폰할 때 지정. 없으면 BP 기본값) */
	UPROPERTY()
	TObjectPtr<const UUSEnemyTypeData> EnemyType;

	/** 종류 체력에 곱하는 배율 (스포너의 경과 시간 성장) */
	float HealthMultiplier = 1.0f;

	/** 현재 감속 배율 (1 = 감속 없음) */
	float SlowMultiplier = 1.0f;

	/** 표적 지정으로 부하 피해에 더하는 비율 (0 = 표적 아님, 0.5 = +50%) */
	float MarkDamageBonus = 0.0f;

	/** 날아오는 중인 볼트 피해 합 (AddPendingDamage). 표적 지정 보너스는 빼고 센 추정치 */
	float PendingDamage = 0.0f;

	/** 현재 추적 대상 (가장 가까운 아군) */
	UPROPERTY()
	TObjectPtr<AActor> CachedTarget;

	/** 사망 상태 플래그 */
	bool bIsDead = false;

	/** 스폰 뒤 흐른 시간(초). SpawnScaleInTime에 닿으면 커지기를 멈춘다 */
	float SpawnScaleElapsed = 0.0f;

	/** 커지는 중인가 (끝나면 Tick에서 바로 빠진다) */
	bool bScalingIn = false;

	/** 마지막 타격이 대장(플레이어 컨트롤러)의 것인가. 처치 연출 판단에 쓴다 */
	bool bLastHitByPlayer = false;

	/** 전투 연출 중계소 (BeginPlay에서 캐싱) */
	UPROPERTY()
	TObjectPtr<UUSCombatFeedbackSubsystem> CombatFeedback;

	/** 종류 트레일 (없으면 비어 있음). 사망 때 끈다 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UParticleSystemComponent>> TrailComponents;
};
