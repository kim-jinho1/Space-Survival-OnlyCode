#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "USBossData.generated.h"

class AUSEnemyProjectile;
class UCameraShakeBase;
class USoundBase;
class UUSEnemyTypeData;

/**
 * 보스 패턴 수치 (출시 전 체크 R-36, 데모 1구역 보스 "모선").
 * 체력·외형·보상은 일반 적처럼 UUSEnemyTypeData에서, 패턴만 여기서 읽는다 (AUSBossEnemyCharacter).
 * 패턴: ① 방사 탄막(경고 고리 → 원형 탄), ② 졸개 사출(바닥 X → 적 소환). 체력이 PhaseTwoHealthRatio 아래로 내려가면 2페이즈.
 */
UCLASS(BlueprintType)
class US_API UUSBossData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 등장 뒤 첫 패턴까지(초). 경고 띠가 지나간 뒤에 시작하게 */
	UPROPERTY(EditDefaultsOnly, Category = "Pattern", meta = (ClampMin = "0.0"))
	float FirstPatternDelay = 3.0f;

	/** 패턴 사이 간격(초, 1페이즈) */
	UPROPERTY(EditDefaultsOnly, Category = "Pattern", meta = (ClampMin = "0.5"))
	float PatternInterval = 3.5f;

	/** N번째 패턴마다 졸개 사출, 나머지는 탄막 (3이면 탄막·탄막·사출 반복) */
	UPROPERTY(EditDefaultsOnly, Category = "Pattern", meta = (ClampMin = "1"))
	int32 MinionEveryNPatterns = 3;

	/** 탄막 경고 고리가 보이는 시간(초). 고리 머티리얼(TelegraphPool) 길이와 맞춘다 */
	UPROPERTY(EditDefaultsOnly, Category = "Barrage", meta = (ClampMin = "0.0"))
	float BarrageTelegraphTime = 0.7f;

	/** 경고 고리 반지름(uu). 보스 몸집보다 조금 크게 */
	UPROPERTY(EditDefaultsOnly, Category = "Barrage", meta = (ClampMin = "0.0"))
	float BarrageTelegraphRadius = 400.0f;

	/** 한 고리의 탄 수 */
	UPROPERTY(EditDefaultsOnly, Category = "Barrage", meta = (ClampMin = "1"))
	int32 BulletsPerRing = 14;

	/** 탄 클래스 (BP_USEnemyProjectile) */
	UPROPERTY(EditDefaultsOnly, Category = "Barrage")
	TSubclassOf<AUSEnemyProjectile> ProjectileClass;

	/** 탄 속도(cm/s). 편대가 사이로 빠질 수 있게 느리게 */
	UPROPERTY(EditDefaultsOnly, Category = "Barrage", meta = (ClampMin = "1.0"))
	float BulletSpeed = 600.0f;

	/** 탄 한 발 피해 */
	UPROPERTY(EditDefaultsOnly, Category = "Barrage", meta = (ClampMin = "0.0"))
	float BulletDamage = 12.0f;

	/** 탄이 사라지기까지(초) */
	UPROPERTY(EditDefaultsOnly, Category = "Barrage", meta = (ClampMin = "0.1"))
	float BulletLifeSeconds = 4.0f;

	/** 발사음 (선택) */
	UPROPERTY(EditDefaultsOnly, Category = "Barrage")
	TObjectPtr<USoundBase> BarrageSound;

	/** 졸개 종류 (보통 정찰기) */
	UPROPERTY(EditDefaultsOnly, Category = "Minion")
	TObjectPtr<const UUSEnemyTypeData> MinionType;

	/** 한 번에 내보내는 졸개 수 */
	UPROPERTY(EditDefaultsOnly, Category = "Minion", meta = (ClampMin = "1"))
	int32 MinionCount = 6;

	/** 졸개가 나오는 보스 주변 반지름(uu) */
	UPROPERTY(EditDefaultsOnly, Category = "Minion", meta = (ClampMin = "0.0"))
	float MinionSpawnRadius = 500.0f;

	/** 졸개 체력 배율 (그 시각 스포너 배율 대신 고정 — 보스전 난이도를 판마다 같게) */
	UPROPERTY(EditDefaultsOnly, Category = "Minion", meta = (ClampMin = "0.01"))
	float MinionHealthMultiplier = 3.0f;

	/** 체력 비율이 이 값 이하가 되면 2페이즈 */
	UPROPERTY(EditDefaultsOnly, Category = "Phase", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PhaseTwoHealthRatio = 0.5f;

	/** 2페이즈 패턴 간격 배율 (0.7이면 30% 짧게) */
	UPROPERTY(EditDefaultsOnly, Category = "Phase", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float PhaseTwoIntervalMultiplier = 0.7f;

	/** 2페이즈 탄막 고리 수 (두 번째 고리는 반 칸 돌려 틈을 메운다) */
	UPROPERTY(EditDefaultsOnly, Category = "Phase", meta = (ClampMin = "1"))
	int32 PhaseTwoRings = 2;

	/** 2페이즈 고리 사이 간격(초) */
	UPROPERTY(EditDefaultsOnly, Category = "Phase", meta = (ClampMin = "0.0"))
	float RingDelay = 0.3f;

	/** 등장·탄막 화면 흔들림 (대장 피격 흔들림과 같은 클래스를 써도 된다). 설정 창 "화면 흔들림"이 꺼져 있으면 안 흔든다 */
	UPROPERTY(EditDefaultsOnly, Category = "Impact")
	TSubclassOf<UCameraShakeBase> ShakeClass;

	/** 보스가 대장에게 이 거리(uu)까지 처음 다가오면 "도착" — 크게 흔들고 도착음 (대략 화면 가장자리) */
	UPROPERTY(EditDefaultsOnly, Category = "Impact", meta = (ClampMin = "0.0"))
	float ArrivalDistance = 1100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Impact", meta = (ClampMin = "0.0"))
	float ArrivalShakeScale = 3.0f;

	/** 도착음 (선택) */
	UPROPERTY(EditDefaultsOnly, Category = "Impact")
	TObjectPtr<USoundBase> ArrivalSound;

	/** 탄막 고리를 쏠 때마다 흔들림 배율 */
	UPROPERTY(EditDefaultsOnly, Category = "Impact", meta = (ClampMin = "0.0"))
	float BarrageShakeScale = 0.6f;

	/** 격파 때 몸 주변에서 이어 터지는 폭발 수 */
	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (ClampMin = "0"))
	int32 DeathExplosionCount = 6;

	/** 연쇄 폭발 사이 간격(초, 게임 시간 — 슬로모션 중이면 같이 느려진다) */
	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (ClampMin = "0.01"))
	float DeathExplosionInterval = 0.12f;

	/** 연쇄 폭발이 퍼지는 반지름(uu) */
	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (ClampMin = "0.0"))
	float DeathExplosionSpread = 250.0f;

	/** 연쇄 폭발이 끝날 때 몸 가운데에서 한 번 내는 큰 폭발음 (선택). 연쇄 폭발음은 BP_USBoss의 DeathExplosion이 낸다 */
	UPROPERTY(EditDefaultsOnly, Category = "Death")
	TObjectPtr<USoundBase> FinalDeathSound;
};
