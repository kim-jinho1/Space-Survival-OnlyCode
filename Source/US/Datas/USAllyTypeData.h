#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Progression/USProgressionTypes.h"
#include "USAllyTypeData.generated.h"

class AUSAllyDroneCharacter;
class UTexture2D;

/** 부하기 역할. 진형 위치와 행동이 역할에 따라 달라진다 */
UENUM(BlueprintType)
enum class EUSAllyRole : uint8
{
	Tank,
	Ranged,
	Slow
};

/**
 * 격납고 설계도의 도면 정보 (틀·치수선·표제란에 적는 글자). 숫자 사양은 기종 데이터의 스탯에서 따로 읽는다.
 * 부하기는 UUSAllyTypeData::BlueprintSheet, 대장기는 UUSHangarBlueprintWidget::LeaderSheet.
 */
USTRUCT(BlueprintType)
struct FUSBlueprintSheet
{
	GENERATED_BODY()

	/** 도면 번호 (도면 틀 왼쪽 위, 예: US-A02) */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint")
	FText DrawingCode;

	/** 분류 (표제란, 예: 호위기 · A02) */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint")
	FText ClassName;

	/** 치수선에 적을 크기 m (X = 전폭, Y = 전장). Tools/blueprint_from_obj.py가 출력한다. 0이면 치수선을 그리지 않는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Blueprint", meta = (ClampMin = "0.0"))
	FVector2D Size = FVector2D::ZeroVector;
};

/**
 * 부하기 종류 하나의 정의. 어떤 BP로 스폰할지와 시작 스탯을 담는다.
 * 분대(UUSSquadComponent)가 이 데이터로 부하를 스폰하고, 부하는 BeginPlay에서 스탯을 적용한다.
 * 기체 메시·역할 전용 컴포넌트처럼 모양이 다른 것은 DroneClass(BP)에, 숫자는 여기에 둔다.
 */
UCLASS(BlueprintType)
class US_API UUSAllyTypeData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	FORCEINLINE const FText& GetDisplayName() const { return DisplayName; }
	FORCEINLINE const FText& GetSummary() const { return Summary; }
	FORCEINLINE UTexture2D* GetIcon() const { return Icon; }
	FORCEINLINE UTexture2D* GetSilhouetteIcon() const { return SilhouetteIcon; }
	FORCEINLINE UTexture2D* GetBlueprintImage() const { return BlueprintImage; }
	FORCEINLINE const FUSBlueprintSheet& GetBlueprintSheet() const { return BlueprintSheet; }
	FORCEINLINE EUSAllyRole GetRole() const { return Role; }
	FORCEINLINE TSubclassOf<AUSAllyDroneCharacter> GetDroneClass() const { return DroneClass; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetMoveSpeed() const { return MoveSpeed; }
	FORCEINLINE float GetRapidFireInterval() const { return RapidFireInterval; }
	FORCEINLINE float GetAttackDamageMultiplier() const { return AttackDamageMultiplier; }

protected:
	/** 화면에 보일 이름 (상점·초상화) */
	UPROPERTY(EditDefaultsOnly, Category = "Ally")
	FText DisplayName;

	/** 한 줄 설명 (편성 화면 기종 카드, 예: "빠르게 연사해 앞쪽 적을 녹입니다") */
	UPROPERTY(EditDefaultsOnly, Category = "Ally")
	FText Summary;

	/** 기종 그림 (격납고 기종 목록·편성 칸). 기본 모델의 Kenney 아이소메트릭 그림 */
	UPROPERTY(EditDefaultsOnly, Category = "Ally")
	TObjectPtr<UTexture2D> Icon;

	/** 흰색 한 가지 색 실루엣 (게임 중 분대 칸 — 체력 색을 곱해 칠한다. 컬러 그림에 색을 곱하면 탁해진다) */
	UPROPERTY(EditDefaultsOnly, Category = "Ally")
	TObjectPtr<UTexture2D> SilhouetteIcon;

	/** 격납고 설계도 화면의 도면 (기수가 위, 투명 바탕 선 그림). Kenney 원본 OBJ에서 Tools/blueprint_from_obj.py로 뽑는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Ally")
	TObjectPtr<UTexture2D> BlueprintImage;

	/** 설계도 화면의 도면 번호·분류·치수 */
	UPROPERTY(EditDefaultsOnly, Category = "Ally")
	FUSBlueprintSheet BlueprintSheet;

	/** 역할. 진형 위치를 정한다 */
	UPROPERTY(EditDefaultsOnly, Category = "Ally")
	EUSAllyRole Role = EUSAllyRole::Ranged;

	/** 스폰할 부하 BP (기체 메시·역할 전용 컴포넌트 포함) */
	UPROPERTY(EditDefaultsOnly, Category = "Ally")
	TSubclassOf<AUSAllyDroneCharacter> DroneClass;

	/** 최대 체력 */
	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "1.0"))
	float MaxHealth = 100.0f;

	/** 기본 이동 속도. 대장(300/달리기 600)을 따라잡을 수 있게 조금 빠르게 */
	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0.0"))
	float MoveSpeed = 420.0f;

	/** 연사 간격(초). 총이 없는 역할은 무시된다 */
	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0.01"))
	float RapidFireInterval = 0.6f;

	/** 시작 공격력 배율. 격납고·보급선 강화는 이 값에 곱해진다 */
	UPROPERTY(EditDefaultsOnly, Category = "Stats", meta = (ClampMin = "0.0"))
	float AttackDamageMultiplier = 1.0f;
};
