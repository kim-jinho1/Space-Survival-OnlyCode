#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Datas/USAllyTypeData.h"
#include "USSquadComponent.generated.h"

class AUSAllyDroneCharacter;
class AUSPlayerCharacter;
class UUSCurrencyComponent;
class USoundBase;
class UUSAbilityData;

/** 분대 구성·격추·가격이 바뀜 (분대 표시 위젯이 구독) */
DECLARE_MULTICAST_DELEGATE(FDelegateOnSquadChanged);

/** 부하 한 기가 격추됨 (분대 칸 인덱스). OnSquadChanged보다 먼저 온다 — 격추 알림·깜빡임용 */
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateOnMemberDown, int32 /*MemberIndex*/);

/** 진형 자리 하나: 대장 이동 방향 기준 오프셋과 이 자리를 우선 차지할 역할 */
USTRUCT(BlueprintType)
struct FUSSquadSlot
{
	GENERATED_BODY()

	/** 오프셋 (X = 조준 방향 앞(+)/뒤(-), Y = 오른쪽(+)/왼쪽(-), cm) */
	UPROPERTY(EditAnywhere, Category = "Squad")
	FVector2D Offset = FVector2D::ZeroVector;

	/** 이 자리를 우선 차지할 역할. 같은 역할 자리가 다 차면 빈 자리 아무 데나 */
	UPROPERTY(EditAnywhere, Category = "Squad")
	EUSAllyRole Role = EUSAllyRole::Ranged;
};

/** 슬롯 하나의 점유 상태. Type이 비면 빈자리, bDown이면 격추되어 부활을 기다리는 자리 */
USTRUCT()
struct FUSSquadMember
{
	GENERATED_BODY()

	/** 자리를 차지한 부하 (격추 뒤에도 부활 전까지 숨긴 채 남아 있다) */
	UPROPERTY()
	TObjectPtr<AUSAllyDroneCharacter> Ally;

	/** 부하 종류 (부활 시 같은 종류로 다시 스폰) */
	UPROPERTY()
	TObjectPtr<const UUSAllyTypeData> Type;

	/** 격추됨 */
	bool bDown = false;

	bool IsEmpty() const { return Type == nullptr; }
	bool IsActive() const { return Type != nullptr && !bDown; }
};

/** 편대 전체(또는 한 부하 종류)에 적용된 강화 하나. 나중에 스폰·부활하는 부하에도 다시 적용한다 */
USTRUCT()
struct FUSTypeUpgrade
{
	GENERATED_BODY()

	/** 강화를 받는 종류. nullptr이면 편대 전체 (보급선 반복 강화) */
	UPROPERTY()
	TObjectPtr<const UUSAllyTypeData> Type;

	UPROPERTY()
	EUSEffectType Effect = EUSEffectType::MaxHealth;

	UPROPERTY()
	float Value = 0.0f;
};

/**
 * 대장(플레이어)에 붙는 분대 컴포넌트.
 * 부하 드론 슬롯·진형, 합류(보급선)·편대 강화·기종 능력(레벨업 카드), 골드 부활을 관리한다. 진형은 대장의 이동 방향을 기준으로 한 슬롯 오프셋이며,
 * 진형은 FormationTurnRate로 천천히 따라 돌고, 대장이 멈추면 마지막 방향을 유지한다 (자동 조준이 표적 따라 튀어도 편대가 흔들리지 않게).
 * 탱커(방패기)만 예외로, 주변에 적이 있으면 이동 방향 대신 "적이 몰려오는 쪽"에 선다 — 도망치며 싸우면 이동 방향 앞은 적이 없는 쪽이라서.
 * 부활 가격 = ReviveBaseCost × ReviveCostGrowth^(부활 횟수), 상한 ReviveCostMax.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class US_API UUSSquadComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UUSSquadComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** SlotIndex의 격추된 부하를 골드로 같은 종류로 되살린다. 격추 자리가 아니거나 골드 부족이면 false (거절음) */
	bool TryRevive(int32 SlotIndex);

	/** AllyType 부하 한 대를 합류시킨다 (보급선이 골드를 받은 뒤 호출). 빈자리가 없으면 격추된 자리를 덮어쓴다. 자리가 없으면 false */
	bool TryAddFreeAlly(const UUSAllyTypeData* AllyType);

	/** 부하 기종 능력의 단계를 정하고 그 기종 전원에 적용한다. 이후 스폰·부활하는 같은 기종에도 적용된다 */
	void SetAbilityLevel(const UUSAbilityData* Ability, int32 Level);

	/** 종류와 상관없이 부하 전원에 강화를 적용하고 기록한다 (보급선 반복 강화). 이후 스폰·부활하는 부하에도 적용된다 */
	void ApplySquadUpgrade(EUSEffectType Effect, float Value);

	/** 부하 EMP 펄스의 감속 시간에 Bonus초를 더한다 (제압 빌드 완성 보너스). 이후 스폰·부활하는 부하에도 적용된다 */
	void AddSlowDurationBonus(float Bonus);

	/**
	 * 시작 부하를 격납고 출격 편성으로 바꾼다 (InitialAllyTypes 대신). 판 시작 때 컨트롤러가 OnPossess에서 부르면 BeginPlay가 이 목록을 스폰하고,
	 * BeginPlay 뒤에 불리면 그 자리에서 스폰한다 (호출 순서에 기대지 않게)
	 */
	void SetStartingAllies(const TArray<const UUSAllyTypeData*>& AllyTypes);

	/** 새 부하 후보(HireableTypes)에서 뺄 종류를 정한다 (영구 업그레이드로 아직 해금하지 않은 종류) */
	void SetLockedTypes(const TArray<const UUSAllyTypeData*>& Types);

	/** 부하를 더 받을 자리가 있는가 (빈자리 또는 격추 자리) */
	bool CanAddAlly() const;

	/** 살아 있는 부하의 종류 목록 (중복 없이) */
	void GetActiveTypes(TArray<const UUSAllyTypeData*>& OutTypes) const;

	/** 살아 있는 부하 전원을 최대 체력의 Fraction만큼 회복한다 (보급선 수리). 회복한 부하가 없으면 false */
	bool RepairAll(float Fraction);

	/** 살아 있는 부하 중 체력이 깎인 부하가 있는가 (수리할 필요가 있는가) */
	bool HasDamagedMember() const;

	/** 부하가 격추됨 (부하가 사망 처리에서 호출). 자리는 부활을 위해 남긴다 */
	void HandleMemberDown(AUSAllyDroneCharacter* Member);

	/** 다음 부활 가격 */
	int32 GetReviveCost() const;

	/** 슬롯의 월드 목표 위치 (대장 위치 + 진형 방향으로 회전한 오프셋). 잘못된 인덱스면 대장 위치 */
	FVector GetSlotWorldLocation(int32 SlotIndex) const;

	/** 현재 살아 있는 부하 수 */
	int32 GetMemberCount() const;

	FORCEINLINE int32 GetMaxMembers() const { return MaxMembers; }
	FORCEINLINE float GetFormationYaw() const { return FormationYaw; }
	FORCEINLINE const TArray<FUSSquadMember>& GetMembers() const { return Members; }
	/** 새 부하 후보 중 해금된 종류 (보급선 합류가 쓴다) */
	FORCEINLINE const TArray<TObjectPtr<UUSAllyTypeData>>& GetHireableTypes() const { return AvailableHireableTypes; }

	/** 분대 구성·격추·가격 변화 */
	FDelegateOnSquadChanged OnSquadChanged;

	/** 부하 격추 (OnSquadChanged 직전) */
	FDelegateOnMemberDown OnMemberDown;

protected:
	/** SlotIndex에 AllyType 부하를 스폰하고 그 종류에 기록된 강화를 적용한다. 실패하면 nullptr */
	AUSAllyDroneCharacter* SpawnAllyInSlot(const UUSAllyTypeData* AllyType, int32 SlotIndex);

	/** SlotIndex의 옛 부하(격추되어 숨겨 둔 몸)를 치우고 자리를 비운다 */
	void ClearSlot(int32 SlotIndex);

	/** 고용할 자리: Role 빈자리 → 아무 빈자리 → Role 격추 자리 → 아무 격추 자리. 없으면 INDEX_NONE */
	int32 FindHireSlot(EUSAllyRole Role) const;

	/** 골드를 낸다. 모자라면 false */
	bool TrySpendGold(int32 Cost) const;

	/** 2D 효과음 재생 (비어 있으면 무시) */
	void PlayFeedbackSound(USoundBase* Sound) const;

	/** 진형이 향할 yaw. 대장이 FormationMinMoveSpeed 이상으로 움직이면 이동 방향, 아니면 지금 진형 방향 그대로 */
	float GetFormationTargetYaw() const;

	/** 대장 주변 적의 가중 방향으로 탱커가 설 방향을 고른다 (GuardUpdateInterval 타이머). 적이 없으면 bHasGuardThreat = false */
	void UpdateGuardThreat();

	/** SlotIndex 부하가 위협 쪽에 설 탱커면 그 자리를 OutLocation에 넣고 true */
	bool TryGetGuardLocation(int32 SlotIndex, FVector& OutLocation) const;

	/** 비어 있는 자리에 AllyType 한 대를 무료로 스폰한다 (시작 부하). 자리가 없으면 false */
	bool SpawnStartingAlly(const UUSAllyTypeData* AllyType);

	/** HireableTypes에서 잠긴 종류를 뺀 목록을 다시 만든다 */
	void RefreshAvailableHireableTypes();

	/** 최대 부하 수 (슬롯 수) */
	UPROPERTY(EditDefaultsOnly, Category = "Squad")
	int32 MaxMembers = 8;

	/** 시작 시 무료로 스폰할 부하 종류 목록 (항목 하나당 한 대) */
	UPROPERTY(EditDefaultsOnly, Category = "Squad")
	TArray<TObjectPtr<UUSAllyTypeData>> InitialAllyTypes;

	/** 보급선에서 합류할 수 있는 종류 (이름은 BP에 저장된 값을 지키려고 예전 그대로 둔다) */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Hire")
	TArray<TObjectPtr<UUSAllyTypeData>> HireableTypes;

	/** 첫 부활 가격. 고용가와 따로 둔다 (예전 공식 첫 고용가 60 × 0.4와 같은 값) */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Revive", meta = (ClampMin = "0"))
	int32 ReviveBaseCost = 24;

	/** 부활할 때마다 부활 가격이 곱해지는 비율 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Revive", meta = (ClampMin = "1.0"))
	float ReviveCostGrowth = 1.3f;

	/** 부활 가격 상한 (예전 공식 첫 고용가 60 × 0.9와 같은 값) */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Revive", meta = (ClampMin = "0"))
	int32 ReviveCostMax = 54;

	/** 새 부하 합류음 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Sound")
	TObjectPtr<USoundBase> HireSound;

	/** 부활 성공음 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Sound")
	TObjectPtr<USoundBase> ReviveSound;

	/** 거절음 (골드 부족·만석) */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Sound")
	TObjectPtr<USoundBase> DeniedSound;

	/** 진형 자리 목록. 탱커는 앞, 원거리는 뒤, 슬로우는 옆. 최대 부하 수보다 적으면 나머지는 대장 뒤 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad")
	TArray<FUSSquadSlot> Slots;

	/** 진형 방향이 대장 이동 방향을 따라가는 속도 (도/초). 급하게 방향을 틀어도 진형이 휙 돌지 않게 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad")
	float FormationTurnRate = 120.0f;

	/** 대장이 이 속도(cm/s) 이상으로 움직일 때만 진형 방향을 바꾼다. 미끄러지듯 멈추는 동안 진형이 흔들리지 않게 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad", meta = (ClampMin = "0.0"))
	float FormationMinMoveSpeed = 100.0f;

	/** 탱커가 위협 방향을 볼 때 세는 적의 거리 (cm). 가까운 적일수록 무겁게 센다 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Guard", meta = (ClampMin = "0.0"))
	float GuardSearchRadius = 1200.0f;

	/** 탱커가 위협 쪽으로 대장에게서 떨어져 서는 거리 (cm). 충각 반경(300) 안에 대장이 들어오지 않을 만큼만 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Guard", meta = (ClampMin = "0.0"))
	float GuardDistance = 250.0f;

	/** 탱커가 여럿이면 위협 방향 좌우로 벌리는 간격 (cm) */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Guard", meta = (ClampMin = "0.0"))
	float GuardSpacing = 160.0f;

	/** 위협 방향을 다시 고르는 간격 (초). 적 목록을 도는 일이라 매 프레임 하지 않는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Guard", meta = (ClampMin = "0.05"))
	float GuardUpdateInterval = 0.25f;

	/** 탱커 방향이 위협 방향을 따라가는 속도 (도/초). 진형보다 빨라야 돌아 들어오는 적을 따라잡는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Squad|Guard", meta = (ClampMin = "0.0"))
	float GuardTurnRate = 240.0f;

private:
	/** 슬롯별 점유 상태 (MaxMembers개) */
	UPROPERTY()
	TArray<FUSSquadMember> Members;

	/** 탱커가 지금 향하는 방향 (yaw). 위협이 없으면 진형 방향을 따라간다 */
	float GuardYaw = 0.0f;

	/** 탱커가 향할 위협 방향 (yaw, UpdateGuardThreat가 정한다) */
	float GuardTargetYaw = 0.0f;

	/** 주변에 적이 있어 탱커가 위협 쪽에 서는 중인가 */
	bool bHasGuardThreat = false;

	FTimerHandle GuardTimerHandle;

	/** 소유 대장 (BeginPlay에서 캐싱) */
	UPROPERTY()
	TObjectPtr<AUSPlayerCharacter> Leader;

	/** 대장 골드 (BeginPlay에서 캐싱) */
	UPROPERTY()
	TObjectPtr<UUSCurrencyComponent> LeaderCurrency;

	/** 지금까지 부활한 수 (가격 계산) */
	int32 ReviveCount = 0;

	/** 출격 편성 시작 부하. bHasStartingOverride면 BeginPlay가 InitialAllyTypes 대신 이것을 스폰 */
	UPROPERTY()
	TArray<TObjectPtr<const UUSAllyTypeData>> StartingAllyOverride;

	/** 출격 편성을 받았는가 (빈 편성도 유효하므로 배열이 비었는지로 판단하지 않는다) */
	bool bHasStartingOverride = false;

	/** 아직 해금하지 않아 새 부하 후보에서 빼는 종류 */
	UPROPERTY()
	TArray<TObjectPtr<const UUSAllyTypeData>> LockedTypes;

	/** HireableTypes - LockedTypes. GetHireableTypes가 돌려준다 */
	UPROPERTY()
	TArray<TObjectPtr<UUSAllyTypeData>> AvailableHireableTypes;

	/** 받은 편대 강화 (스폰·부활 때 다시 적용) */
	UPROPERTY()
	TArray<FUSTypeUpgrade> TypeUpgrades;

	/** 부하 기종 능력의 현재 단계 (스폰·부활 때 그 기종에 다시 적용) */
	UPROPERTY()
	TMap<TObjectPtr<const UUSAbilityData>, int32> AbilityLevels;

	/** 빌드 보너스 감속 시간(초, 스폰·부활 때 다시 적용) */
	float SlowDurationBonus = 0.0f;

	/** 현재 진형 방향 yaw (도) */
	float FormationYaw = 0.0f;
};
