#pragma once

#include "CoreMinimal.h"
#include "Agents/USDroneCharacter.h"
#include "USAllyDroneCharacter.generated.h"

class AUSPlayerCharacter;
class UUSSquadComponent;
class UBehaviorTree;
class UUSAllyTypeData;
class UUSUpgradeVisualComponent;

/**
 * 부하 드론. 대장의 분대 슬롯 하나를 맡고, AUSAllyAIController가 BehaviorTree로 조종한다.
 * 이동·조준·발사 판단은 BT 노드(AI/USBT*)에 있고, 이 클래스는 분대 소속 정보와 스탯만 가진다.
 */
UCLASS()
class US_API AUSAllyDroneCharacter : public AUSDroneCharacter
{
	GENERATED_BODY()

public:
	AUSAllyDroneCharacter();

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	/** 분대 컴포넌트가 스폰 직후 호출. 대장·분대·슬롯 번호를 기억한다 */
	void InitSquadMember(AUSPlayerCharacter* InLeader, UUSSquadComponent* InSquad, int32 InSlotIndex);

	FORCEINLINE AUSPlayerCharacter* GetLeader() const { return Leader; }
	FORCEINLINE UUSSquadComponent* GetSquad() const { return Squad; }
	FORCEINLINE int32 GetSlotIndex() const { return SlotIndex; }
	FORCEINLINE UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }

	/** 내 슬롯의 월드 목표 위치. 분대가 없으면 현재 위치 */
	FVector GetSlotLocation() const;

	/** 스폰 직후(FinishSpawning 전) 분대가 호출. BeginPlay에서 이 종류의 스탯을 적용한다 */
	void SetAllyType(const UUSAllyTypeData* InAllyType);

	FORCEINLINE const UUSAllyTypeData* GetAllyType() const { return AllyType; }

protected:
	/** 사망: 분대에서 빠지고 AI를 멈춘다. 숨겨진 액터는 부활(P0-5)을 위해 남긴다 */
	virtual void HandleDeath() override;

	/** 스탯 배율 변경 시 이속 재계산 */
	virtual void HandleStatsChanged() override;

	/** 이속 배율을 반영해 MaxWalkSpeed 갱신 */
	void RefreshMoveSpeed();

	/** AllyType의 시작 스탯(체력·이속·연사 간격·공격력 배율)을 적용한다. 종류가 없으면 BP 기본값 유지 */
	void ApplyAllyTypeStats();

	/** AI 컨트롤러가 실행할 BehaviorTree */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	/** 기본 이동 속도. 종류 데이터(AllyType)가 있으면 그 값으로 덮어쓴다 */
	UPROPERTY(EditDefaultsOnly, Category = "Move")
	float MoveSpeed = 420.0f;

	/** 업그레이드 외형: 등급에 따라 기체 진화(메시 교체)·크기·발광 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UUSUpgradeVisualComponent> UpgradeVisual;

private:
	UPROPERTY()
	TObjectPtr<AUSPlayerCharacter> Leader;

	UPROPERTY()
	TObjectPtr<UUSSquadComponent> Squad;

	int32 SlotIndex = INDEX_NONE;

	/** 이 부하의 종류 (분대가 스폰할 때 지정) */
	UPROPERTY()
	TObjectPtr<const UUSAllyTypeData> AllyType;
};
