// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "USEnemySpawner.generated.h"

class AUSBossEnemyCharacter;
class AUSEnemyCharacter;
class UUSEnemyTypeData;
class UUSRingPulsePoolComponent;

/** 스폰 테이블 한 줄: 어떤 적을, 언제부터, 얼마나 자주 */
USTRUCT(BlueprintType)
struct FUSEnemySpawnEntry
{
	GENERATED_BODY()

	/** 적 종류 */
	UPROPERTY(EditAnywhere, Category = "Spawner")
	TObjectPtr<UUSEnemyTypeData> EnemyType;

	/** 뽑힐 가중치 (등장한 줄끼리의 비율) */
	UPROPERTY(EditAnywhere, Category = "Spawner", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	/** 스폰 시작 뒤 이 시간(초)부터 나온다 */
	UPROPERTY(EditAnywhere, Category = "Spawner", meta = (ClampMin = "0.0"))
	float StartTime = 0.0f;
};

/** 무리 모양 */
UENUM(BlueprintType)
enum class EUSHordeShape : uint8
{
	/** 한 방향에서 촘촘한 덩어리가 몰려온다 (Vampire Survivors 박쥐 떼) */
	Rush,
	/** 대장 둘레 원형으로 한꺼번에 나타나 조여 온다 (Vampire Survivors 꽃 벽) */
	Ring,
};

/** 무리 이벤트 한 줄: 정해진 시각에 예고 뒤 한꺼번에 나오는 적 무리 */
USTRUCT(BlueprintType)
struct FUSHordeEvent
{
	GENERATED_BODY()

	/** 스폰 시작 뒤 이 시간(초)에 무리가 나온다. 예고는 WarningTime만큼 먼저 */
	UPROPERTY(EditAnywhere, Category = "Horde", meta = (ClampMin = "0.0"))
	float Time = 30.0f;

	UPROPERTY(EditAnywhere, Category = "Horde")
	EUSHordeShape Shape = EUSHordeShape::Rush;

	/** 무리를 이루는 적 */
	UPROPERTY(EditAnywhere, Category = "Horde")
	TObjectPtr<UUSEnemyTypeData> EnemyType;

	UPROPERTY(EditAnywhere, Category = "Horde", meta = (ClampMin = "1"))
	int32 Count = 14;

	/** 무리에 섞는 적 (기뢰 — 맞히면 대폭발로 무리를 쓸어 낸다). 비우면 안 섞는다 */
	UPROPERTY(EditAnywhere, Category = "Horde")
	TObjectPtr<UUSEnemyTypeData> CoreEnemyType;

	UPROPERTY(EditAnywhere, Category = "Horde", meta = (ClampMin = "0"))
	int32 CoreCount = 1;

	/** Rush: 대장에서 덩어리 중심까지 거리 / Ring: 고리 반지름 */
	UPROPERTY(EditAnywhere, Category = "Horde", meta = (ClampMin = "100.0"))
	float Distance = 1500.0f;

	/** Rush 덩어리의 반지름. Ring에서는 쓰지 않는다 */
	UPROPERTY(EditAnywhere, Category = "Horde", meta = (ClampMin = "0.0"))
	float ClusterRadius = 250.0f;

	/** 예고(화면 문구·화살표, Ring은 바닥 X 표시)를 무리보다 먼저 띄우는 시간(초). X 머티리얼 길이와 맞춘다 */
	UPROPERTY(EditAnywhere, Category = "Horde", meta = (ClampMin = "0.0"))
	float WarningTime = 1.5f;
};

/** 무리 예고: 모양, 무리 중심(Rush) 또는 대장 위치(Ring), 나오기까지 남은 초 */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FDelegateOnHordeWarning, EUSHordeShape, const FVector&, float);

/** 예고 중인 웨이브의 한 자리 (X를 띄운 곳에서 그대로 나와야 한다) */
struct FUSPendingSpawn
{
	/** SpawnTable(UPROPERTY)이 잡고 있는 데이터라 원시 포인터로 둔다 */
	const UUSEnemyTypeData* EnemyType = nullptr;
	FVector Location = FVector::ZeroVector;
};

/**
 * 뱀서형 적 스포너.
 * 플레이어 주변의 링(최소~최대 반경, 화면 가장자리 안팎) 위에 웨이브 단위로 적을 스폰한다.
 * 화면 안에서도 나오므로 자리마다 바닥 X를 먼저 띄우고 SpawnWarningTime 뒤에 스폰한다 (Brotato 방식).
 * 스폰 테이블에서 경과 시간에 맞는 종류를 가중치로 뽑고, 체력은 경과 시간에 따라 지수로 키운다.
 * 동시 생존 수를 제한하고, 일정 시간마다 웨이브당 적 수를 늘려 난이도를 올린다.
 * HordeEvents는 웨이브와 따로, 정해진 시각에 예고 뒤 무리를 한꺼번에 내보낸다 (동시 생존 상한 무시).
 */
UCLASS()
class US_API AUSEnemySpawner : public AActor
{
	GENERATED_BODY()

public:
	AUSEnemySpawner();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** 스폰 시작/중지 */
	void StartSpawning();
	void StopSpawning();

	/**
	 * 지금 스폰하는 적에게 곱할 체력 배율 = HealthGrowthPerMinute ^ 경과 분.
	 * 계속 가기 중이면 여기에 ContinueHealthStep ^ (계속 가기 뒤 ContinueStepSeconds 구간 수)를 더 곱한다
	 */
	float GetHealthMultiplier() const;

	/**
	 * 클리어 후 "계속 가기": 지금부터 StepSeconds마다 적 체력이 ContinueHealthStep배씩 오른다.
	 * 구간 길이는 스크랩 보너스 구간과 같아야 하므로 컨트롤러가 UUSScrapRewardData 값을 넘긴다 (0 이하면 기본 30초)
	 */
	void EnterContinueMode(float StepSeconds);

	/** 무리 예고 (HUD가 문구·방향 띠를 띄운다) */
	FDelegateOnHordeWarning OnHordeWarning;

	/** 이번 판에 한 번이라도 스폰한 적 종류, 처음 나온 순서 (일시정지 "만난 적" 목록, R-41 5). 보스 포함 */
	FORCEINLINE const TArray<const UUSEnemyTypeData*>& GetSpawnedEnemyTypes() const { return SpawnedEnemyTypes; }

	/** 보스가 정해져 있는가 (없거나 보스 없는 스테이지면 컨트롤러가 바로 클리어) */
	FORCEINLINE bool HasBoss() const { return bBossEnabled && BossEnemyType != nullptr; }

	/**
	 * 스테이지 설정으로 스폰을 처음부터 다시 시작한다 (판 시작, "다음 스테이지").
	 * InSpawnTable이 비면 맵 스포너 테이블 그대로. 물량 배율은 웨이브당 수·동시 생존 상한(시작값·한계)에 곱하고,
	 * 체력 배율은 경과 시간 성장 위에 곱한다. 이미 돌던 타이머·예고는 지우고 경과 시간도 0부터 센다
	 */
	void ApplyStage(const TArray<FUSEnemySpawnEntry>& InSpawnTable, float InHealthMultiplier, float AmountMultiplier, bool bInBossEnabled);

	/** 보스 종류 (이름·그림을 체력바·클리어 창이 쓴다) */
	FORCEINLINE const UUSEnemyTypeData* GetBossEnemyType() const { return BossEnemyType; }

	/**
	 * 보스전 시작 (출시 전 체크 R-36): 무리 이벤트·난이도 상승을 멈추고 동시 생존 상한을 BossPhaseMaxAlive로 낮춘 뒤,
	 * 대장 주변 스폰 링(화면 밖)에 보스를 스폰한다. 체력은 종류 데이터 그대로(경과 시간 성장 없음). 실패하면 nullptr
	 */
	AUSBossEnemyCharacter* StartBossPhase();

	/** 보스전 끝(격파): 동시 생존 상한·난이도 상승을 되돌린다 ("계속 가기"가 평소처럼 이어지게) */
	void EndBossPhase();

	/**
	 * 살아 있는 적을 전부 큰 피해로 격추한다 (Except 제외). 폭발·보상·처치 수가 평소 처치와 같게 흐른다.
	 * 보스 등장(bClearEnemiesOnBoss)과 스테이지 클리어가 쓴다. 격추한 수를 돌려준다
	 */
	int32 ClearAllEnemies(const AActor* Except = nullptr);

	/**
	 * 보스 졸개: Center 주변 Radius 안쪽 바닥에 X를 띄우고 SpawnWarningTime 뒤 Count마리를 스폰한다.
	 * 동시 생존 상한은 무시한다 (패턴이라 정해진 수가 나와야 한다)
	 */
	void SpawnMinions(const UUSEnemyTypeData* MinionType, const FVector& Center, int32 Count, float Radius, float HealthMultiplier);

protected:
	/** 한 웨이브 분량의 자리를 정해 바닥 X를 띄우고, SpawnWarningTime 뒤 SpawnPendingWave를 예약한다 (0이면 바로 스폰) */
	void SpawnWave();

	/** 예고가 끝난 웨이브를 X 자리에서 스폰한다. 체력 배율은 지금 시각 기준 */
	void SpawnPendingWave(TArray<FUSPendingSpawn> Batch);

	/** 난이도 상승: 웨이브당 적 수 증가, 동시 생존 상한 증가, 웨이브 간격 단축 */
	void RampDifficulty();

	/** 스폰 시작 뒤 흐른 시간(초). 월드 시간이라 일시정지(카드·상점) 동안은 멈춘다 */
	float GetElapsedTime() const;

	/** 경과 시간에 등장한 줄 중에서 가중치로 하나를 뽑는다. 없으면 nullptr */
	const UUSEnemyTypeData* PickEnemyType(float ElapsedTime) const;

	/**
	 * 중심 주변 링에서 스폰 가능한 지면 위치를 찾는다 (USLocationUtils).
	 * Angle 방향 부근을 먼저 보고, 없으면 링 전체에서 찾는다. 실패하면 Warning 후 false
	 */
	bool FindSpawnLocation(const FVector& Center, float Angle, float AngleJitter, FVector& OutLocation) const;

	/** 스폰 기준이 되는 플레이어 액터 */
	AActor* GetPlayerActor() const;

	/** 적 한 마리를 지연 스폰한다 (종류·체력 배율을 BeginPlay 전에 넣는다). 실패하면 Warning 후 nullptr */
	AUSEnemyCharacter* SpawnEnemyAt(const UUSEnemyTypeData* EnemyType, const FVector& Location, float HealthMultiplier);

	/** 무리 예고 시각: 스폰 자리를 정해 예고를 보내고(Ring은 바닥 X) WarningTime 뒤 SpawnHorde를 예약한다 */
	void HandleHordeWarningTime(int32 EventIndex);

	/** 예고 때 정한 자리(PendingHordeLocations)에 무리를 한꺼번에 스폰한다 */
	void SpawnHorde(int32 EventIndex);

	/** 무리 모양대로 스폰 위치를 만든다 (바닥이 없는 자리는 뺀다) */
	void BuildHordeLocations(const FUSHordeEvent& Event, const FVector& Center, int32 Total, TArray<FVector>& OutLocations) const;

protected:
	/** 스폰 테이블. 경과 시간에 등장한 줄끼리 가중치로 뽑는다 */
	UPROPERTY(EditAnywhere, Category = "Spawner")
	TArray<FUSEnemySpawnEntry> SpawnTable;

	/** 클리어 시각에 나오는 보스 종류 (EnemyClass = AUSBossEnemyCharacter BP). 비우면 보스 없이 바로 클리어 */
	UPROPERTY(EditAnywhere, Category = "Spawner|Boss")
	TObjectPtr<const UUSEnemyTypeData> BossEnemyType;

	/** 보스가 나오는 순간 화면의 적을 한꺼번에 격추한다 (판 흐름을 끊어 "다른 국면"을 보이게, 보상 젬은 그대로 떨어진다) */
	UPROPERTY(EditAnywhere, Category = "Spawner|Boss")
	bool bClearEnemiesOnBoss = true;

	/** 보스전 동안 일반 적 동시 생존 상한 (보스와 탄막에 집중하게 줄인다) */
	UPROPERTY(EditAnywhere, Category = "Spawner|Boss", meta = (ClampMin = "0"))
	int32 BossPhaseMaxAlive = 10;

	/** 보스를 내보내는가 (보스 스테이지만 true, 스테이지가 없는 판도 true) */
	bool bBossEnabled = true;

	/** 무리 이벤트. 줄마다 한 판에 한 번 나온다 */
	UPROPERTY(EditAnywhere, Category = "Spawner|Horde")
	TArray<FUSHordeEvent> HordeEvents;

	/** 웨이브 간격(초). 난이도 램프로 점점 짧아진다. */
	UPROPERTY(EditAnywhere, Category = "Spawner")
	float SpawnInterval = 3.0f;

	/** 웨이브당 스폰 수(버스트 상한). 난이도 램프로 늘어난다. */
	UPROPERTY(EditAnywhere, Category = "Spawner")
	int32 EnemiesPerWave = 2;

	/** 플레이어로부터의 최소 스폰 반경. 화면 밖이어야 적이 눈앞에서 튀어나오지 않는다 */
	UPROPERTY(EditAnywhere, Category = "Spawner")
	float SpawnRadiusMin = 1200.0f;

	/** 플레이어로부터의 최대 스폰 반경 */
	UPROPERTY(EditAnywhere, Category = "Spawner")
	float SpawnRadiusMax = 1600.0f;

	/** 웨이브 적이 바닥 X 예고 뒤 나오기까지의 시간(초). 0이면 예고 없이 바로. X 머티리얼(M_USSpawnMarker) 길이와 맞춘다 */
	UPROPERTY(EditAnywhere, Category = "Spawner", meta = (ClampMin = "0.0"))
	float SpawnWarningTime = 1.5f;

	/** 웨이브 안에서 각 적의 방향을 N등분 각도에서 흔드는 비율 (0 = 정확히 등분, 1 = 옆 칸까지) */
	UPROPERTY(EditAnywhere, Category = "Spawner", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SpawnAngleJitter = 0.5f;

	/** 동시 생존 가능한 최대 적 수(시작값). 난이도 램프로 늘어난다. */
	UPROPERTY(EditAnywhere, Category = "Spawner")
	int32 MaxAliveEnemies = 8;

	/** 시작 시 자동 스폰 여부 */
	UPROPERTY(EditAnywhere, Category = "Spawner")
	bool bAutoStart = true;

	/** 1분마다 적 체력에 곱해지는 배율 (1.25면 8분에 약 6배). 강화 곡선과 같은 지수 모양으로 맞춘다 */
	UPROPERTY(EditAnywhere, Category = "Spawner|Difficulty", meta = (ClampMin = "1.0"))
	float HealthGrowthPerMinute = 1.25f;

	/** 계속 가기 중 구간마다 적 체력에 곱해지는 배율 (기획서: 30초마다 약 ×1.5) */
	UPROPERTY(EditAnywhere, Category = "Spawner|Difficulty", meta = (ClampMin = "1.0"))
	float ContinueHealthStep = 1.5f;

	/** 난이도 상승 간격(초). 0 이하이면 상승 없음 */
	UPROPERTY(EditAnywhere, Category = "Spawner|Difficulty")
	float DifficultyRampInterval = 20.0f;

	/** 난이도 상승 시 늘어나는 웨이브당 적 수 */
	UPROPERTY(EditAnywhere, Category = "Spawner|Difficulty")
	int32 EnemiesPerWaveIncrease = 1;

	/** 웨이브당 적 수 상한 (난이도 상승 한계) */
	UPROPERTY(EditAnywhere, Category = "Spawner|Difficulty")
	int32 EnemiesPerWaveMax = 8;

	/** 난이도 상승 시 늘어나는 동시 생존 상한 */
	UPROPERTY(EditAnywhere, Category = "Spawner|Difficulty")
	int32 MaxAliveIncreasePerRamp = 4;

	/** 동시 생존 상한의 최종 한계 */
	UPROPERTY(EditAnywhere, Category = "Spawner|Difficulty")
	int32 MaxAliveEnemiesCap = 40;

	/** 난이도 상승 시 줄어드는 웨이브 간격(초) */
	UPROPERTY(EditAnywhere, Category = "Spawner|Difficulty")
	float SpawnIntervalDecreasePerRamp = 0.25f;

	/** 웨이브 간격의 하한(초) */
	UPROPERTY(EditAnywhere, Category = "Spawner|Difficulty")
	float SpawnIntervalMin = 1.0f;

protected:
	/** 스폰한 적들(약포인터). 매 웨이브 무효/사망 항목을 정리해 동시 생존 수를 정확히 센다. */
	TArray<TWeakObjectPtr<AUSEnemyCharacter>> ActiveEnemies;

	/** 이번 판에 스폰한 적 종류 (처음 나온 순서). 종류 데이터는 스폰 테이블·BossEnemyType(UPROPERTY)이 잡고 있어 원시 포인터로 둔다 */
	TArray<const UUSEnemyTypeData*> SpawnedEnemyTypes;

	/** 무효/사망 적을 정리하고 현재 생존 수를 반환 (카운트 누수 방지) */
	int32 PurgeAndCountAlive();

	FTimerHandle SpawnTimerHandle;
	FTimerHandle DifficultyTimerHandle;

	/** 무리 이벤트마다 하나 (예고 → 스폰에 같은 핸들을 다시 쓴다) */
	TArray<FTimerHandle> HordeTimerHandles;

	/** 예고 중인 웨이브마다 하나. 끝난 핸들은 다음 웨이브 때 정리한다 */
	TArray<FTimerHandle> PendingWaveTimerHandles;

	/** 예고 중(아직 스폰 전)인 적 수. 동시 생존 상한 계산에 넣어 예고 중에 상한을 넘겨 더 뽑지 않게 한다 */
	int32 PendingSpawnCount = 0;

	/** 무리 이벤트마다 예고 때 정한 스폰 자리. 바닥 X가 가리킨 곳에서 그대로 나와야 한다 */
	TArray<TArray<FVector>> PendingHordeLocations;

	/** Ring 무리가 나올 자리의 바닥 X 표시 (폭발탄 고리와 같은 판 풀, 머티리얼만 X). 무리 수보다 PoolSize가 커야 한다 */
	UPROPERTY(VisibleAnywhere, Category = "Spawner|Horde")
	TObjectPtr<UUSRingPulsePoolComponent> SpawnMarkers;

	/** 바닥 X 하나의 반지름 (판 크기) */
	UPROPERTY(EditAnywhere, Category = "Spawner|Horde", meta = (ClampMin = "10.0"))
	float SpawnMarkerRadius = 60.0f;

private:
	/** 스폰 위치를 바닥에서 띄우는 높이 = 캡슐 반높이(약 90). 바닥에 끼지 않게 */
	static constexpr float SpawnHeightOffset = 90.0f;

	/** StartSpawning 때의 월드 시간 (경과 시간 기준) */
	float SpawnStartTime = 0.0f;

	/** 계속 가기 중인가 */
	bool bContinueMode = false;

	/** 계속 가기를 시작한 경과 시간(초) */
	float ContinueStartElapsed = 0.0f;

	/** 계속 가기 체력 상승 구간 길이(초). EnterContinueMode에서 받는다 */
	float ContinueStepSeconds = 30.0f;

	/** 보스전 중인가 */
	bool bBossPhase = false;

	/** 보스전 전 동시 생존 상한 (EndBossPhase가 되돌린다) */
	int32 MaxAliveBeforeBoss = 0;

	/** 맵에 놓인 시작값을 기억해 두었는가 (스테이지마다 시작값에서 다시 계산한다) */
	void CacheDefaults();
	bool bDefaultsCached = false;
	TArray<FUSEnemySpawnEntry> DefaultSpawnTable;
	float DefaultSpawnInterval = 0.0f;
	int32 DefaultEnemiesPerWave = 0;
	int32 DefaultEnemiesPerWaveMax = 0;
	int32 DefaultMaxAliveEnemies = 0;
	int32 DefaultMaxAliveEnemiesCap = 0;

	/** 스테이지 체력 배율 (ApplyStage) */
	float StageHealthMultiplier = 1.0f;

	/** StartSpawning이 돌았는가 (ApplyStage가 판 시작 전이면 다시 시작하지 않고 값만 바꾼다) */
	bool bSpawningStarted = false;
};
