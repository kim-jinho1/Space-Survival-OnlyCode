// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawning/USEnemySpawner.h"
#include "Agents/USBossEnemyCharacter.h"
#include "Agents/USEnemyCharacter.h"
#include "Components/USRingPulsePoolComponent.h"
#include "Core/USLocationUtils.h"
#include "Datas/USEnemyTypeData.h"
#include "US.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "Engine/World.h"
#include "TimerManager.h"

AUSEnemySpawner::AUSEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// 판들은 절대 위치라 루트 위치와 상관없다. 스포너는 다른 컴포넌트가 없어 이것을 루트로 둔다
	SpawnMarkers = CreateDefaultSubobject<UUSRingPulsePoolComponent>(TEXT("SpawnMarkers"));
	RootComponent = SpawnMarkers;
}

void AUSEnemySpawner::BeginPlay()
{
	Super::BeginPlay();

	CacheDefaults();

	if (bAutoStart && !bSpawningStarted)
	{
		StartSpawning();
	}
}

void AUSEnemySpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopSpawning();

	Super::EndPlay(EndPlayReason);
}

void AUSEnemySpawner::StartSpawning()
{
	if (SpawnTable.IsEmpty())
	{
		UE_LOG(LogUS, Warning, TEXT("%s: SpawnTable is empty; cannot start spawning"), *GetName());
		return;
	}

	const UWorld* World = GetWorld();
	SpawnStartTime = World ? World->GetTimeSeconds() : 0.0f;
	bSpawningStarted = true;

	if (SpawnInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			SpawnTimerHandle,
			this,
			&AUSEnemySpawner::SpawnWave,
			SpawnInterval,
			true,
			0.0f /* 첫 웨이브 즉시 */);
	}

	if (DifficultyRampInterval > 0.0f && EnemiesPerWaveIncrease > 0)
	{
		GetWorldTimerManager().SetTimer(
			DifficultyTimerHandle,
			this,
			&AUSEnemySpawner::RampDifficulty,
			DifficultyRampInterval,
			true);
	}

	// 무리는 시각이 정해져 있으므로 처음에 한꺼번에 예약한다. 월드 타이머라 카드·상점으로 멈추면 같이 멈춘다
	HordeTimerHandles.SetNum(HordeEvents.Num());
	PendingHordeLocations.SetNum(HordeEvents.Num());
	for (int32 Index = 0; Index < HordeEvents.Num(); ++Index)
	{
		const FUSHordeEvent& Event = HordeEvents[Index];
		if (Event.EnemyType == nullptr)
		{
			UE_LOG(LogUS, Warning, TEXT("%s: HordeEvents[%d] has no EnemyType; skipping"), *GetName(), Index);
			continue;
		}

		// SetTimer는 0 이하 지연을 "타이머 해제"로 받으므로 아주 작은 값으로 올린다
		const float Delay = FMath::Max(KINDA_SMALL_NUMBER, Event.Time - Event.WarningTime);
		GetWorldTimerManager().SetTimer(HordeTimerHandles[Index],
			FTimerDelegate::CreateUObject(this, &AUSEnemySpawner::HandleHordeWarningTime, Index), Delay, false);
	}
}

void AUSEnemySpawner::StopSpawning()
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(DifficultyTimerHandle);

	for (FTimerHandle& Handle : HordeTimerHandles)
	{
		GetWorldTimerManager().ClearTimer(Handle);
	}

	for (FTimerHandle& Handle : PendingWaveTimerHandles)
	{
		GetWorldTimerManager().ClearTimer(Handle);
	}
	PendingWaveTimerHandles.Reset();
	PendingSpawnCount = 0;
}

void AUSEnemySpawner::CacheDefaults()
{
	if (bDefaultsCached)
	{
		return;
	}
	bDefaultsCached = true;
	DefaultSpawnTable = SpawnTable;
	DefaultSpawnInterval = SpawnInterval;
	DefaultEnemiesPerWave = EnemiesPerWave;
	DefaultEnemiesPerWaveMax = EnemiesPerWaveMax;
	DefaultMaxAliveEnemies = MaxAliveEnemies;
	DefaultMaxAliveEnemiesCap = MaxAliveEnemiesCap;
}

void AUSEnemySpawner::ApplyStage(const TArray<FUSEnemySpawnEntry>& InSpawnTable, float InHealthMultiplier, float AmountMultiplier, bool bInBossEnabled)
{
	// 컨트롤러 BeginPlay가 스포너 BeginPlay보다 먼저일 수 있어 여기서도 시작값을 잡는다
	CacheDefaults();

	const bool bRestart = bSpawningStarted;
	StopSpawning();

	SpawnTable = InSpawnTable.IsEmpty() ? DefaultSpawnTable : InSpawnTable;
	StageHealthMultiplier = FMath::Max(KINDA_SMALL_NUMBER, InHealthMultiplier);
	bBossEnabled = bInBossEnabled;

	// 물량은 시작값에서 다시 계산한다 (지난 스테이지 난이도 상승이 쌓이지 않게)
	const float Amount = FMath::Max(0.1f, AmountMultiplier);
	SpawnInterval = DefaultSpawnInterval;
	EnemiesPerWave = FMath::Max(1, FMath::RoundToInt(DefaultEnemiesPerWave * Amount));
	EnemiesPerWaveMax = FMath::Max(EnemiesPerWave, FMath::RoundToInt(DefaultEnemiesPerWaveMax * Amount));
	MaxAliveEnemies = FMath::Max(1, FMath::RoundToInt(DefaultMaxAliveEnemies * Amount));
	MaxAliveEnemiesCap = FMath::Max(MaxAliveEnemies, FMath::RoundToInt(DefaultMaxAliveEnemiesCap * Amount));

	bContinueMode = false;
	bBossPhase = false;

	UE_LOG(LogUS, Log, TEXT("%s: stage applied (%d table rows, HP x%.2f, amount x%.2f, boss %s), wave %d, max alive %d"),
		*GetName(), SpawnTable.Num(), StageHealthMultiplier, Amount, bBossEnabled ? TEXT("on") : TEXT("off"), EnemiesPerWave, MaxAliveEnemies);

	if (bRestart)
	{
		StartSpawning();
	}
}

float AUSEnemySpawner::GetElapsedTime() const
{
	const UWorld* World = GetWorld();
	return World ? FMath::Max(0.0f, World->GetTimeSeconds() - SpawnStartTime) : 0.0f;
}

float AUSEnemySpawner::GetHealthMultiplier() const
{
	// 덧셈(1 + 0.25t)이 아니라 지수: 우리 편 강화가 곱셈으로 쌓이므로 적 체력도 같은 모양이어야 템포가 유지된다
	const float ElapsedTime = GetElapsedTime();
	float Multiplier = StageHealthMultiplier * FMath::Pow(HealthGrowthPerMinute, ElapsedTime / 60.0f);

	// 계속 가기는 부드러운 곡선이 아니라 구간마다 한 번에 뛴다. 플레이어가 "지금 세졌다"를 느끼게 하려는 계단식
	if (bContinueMode)
	{
		const int32 Steps = FMath::FloorToInt((ElapsedTime - ContinueStartElapsed) / ContinueStepSeconds);
		Multiplier *= FMath::Pow(ContinueHealthStep, static_cast<float>(FMath::Max(0, Steps)));
	}

	return Multiplier;
}

void AUSEnemySpawner::EnterContinueMode(float StepSeconds)
{
	if (bContinueMode)
	{
		return;
	}

	if (StepSeconds > 0.0f)
	{
		// 0에 가까운 값이면 GetHealthMultiplier의 구간 수가 폭발하므로 최소 1초
		ContinueStepSeconds = FMath::Max(1.0f, StepSeconds);
	}

	bContinueMode = true;
	ContinueStartElapsed = GetElapsedTime();
}

const UUSEnemyTypeData* AUSEnemySpawner::PickEnemyType(float ElapsedTime) const
{
	float TotalWeight = 0.0f;
	for (const FUSEnemySpawnEntry& Entry : SpawnTable)
	{
		if (Entry.EnemyType && Entry.StartTime <= ElapsedTime)
		{
			TotalWeight += Entry.Weight;
		}
	}

	if (TotalWeight <= 0.0f)
	{
		return nullptr;
	}

	float Roll = FMath::FRandRange(0.0f, TotalWeight);
	for (const FUSEnemySpawnEntry& Entry : SpawnTable)
	{
		if (Entry.EnemyType == nullptr || Entry.StartTime > ElapsedTime)
		{
			continue;
		}

		Roll -= Entry.Weight;
		if (Roll <= 0.0f)
		{
			return Entry.EnemyType;
		}
	}

	// 부동소수 오차로 끝까지 남으면 마지막 등장 줄
	for (int32 Index = SpawnTable.Num() - 1; Index >= 0; --Index)
	{
		const FUSEnemySpawnEntry& Entry = SpawnTable[Index];
		if (Entry.EnemyType && Entry.StartTime <= ElapsedTime && Entry.Weight > 0.0f)
		{
			return Entry.EnemyType;
		}
	}

	return nullptr;
}

void AUSEnemySpawner::SpawnWave()
{
	AActor* Player = GetPlayerActor();
	if (Player == nullptr)
		return;

	UWorld* World = GetWorld();
	if (World == nullptr)
		return;

	const FVector Center = Player->GetActorLocation();

	// 현재 생존 수를 정확히 센 뒤, 상한까지 부족한 만큼을 이번 버스트 한도 내에서 채운다.
	// (상한에 빈자리가 생기면 즉시 메우므로 후반 처치 속도가 빨라도 휑하지 않다.)
	const int32 AliveNow = PurgeAndCountAlive();
	const int32 SpawnBudget = FMath::Min(EnemiesPerWave, MaxAliveEnemies - AliveNow - PendingSpawnCount);
	if (SpawnBudget <= 0)
	{
		return;
	}

	const float ElapsedTime = GetElapsedTime();

	// 한 웨이브를 원 둘레에 고르게 나눈다. 모두 무작위 방향이면 한쪽에 몰려 한 덩어리로 다가온다
	const float BaseAngle = FMath::FRandRange(0.0f, 2.0f * PI);
	const float AngleStep = 2.0f * PI / SpawnBudget;
	const float AngleJitter = AngleStep * 0.5f * SpawnAngleJitter;

	TArray<FUSPendingSpawn> Batch;
	Batch.Reserve(SpawnBudget);

	for (int32 i = 0; i < SpawnBudget; ++i)
	{
		const UUSEnemyTypeData* EnemyType = PickEnemyType(ElapsedTime);
		if (EnemyType == nullptr || EnemyType->GetEnemyClass() == nullptr)
		{
			UE_LOG(LogUS, Warning, TEXT("%s: no spawnable enemy type at %.0fs (empty table row or missing EnemyClass)"),
				*GetName(), ElapsedTime);
			return;
		}

		FVector SpawnLocation;
		if (!FindSpawnLocation(Center, BaseAngle + AngleStep * i, AngleJitter, SpawnLocation))
			continue;

		Batch.Add({ EnemyType, SpawnLocation });
	}

	if (Batch.IsEmpty())
	{
		return;
	}

	if (SpawnWarningTime <= 0.0f)
	{
		const float HealthMultiplier = GetHealthMultiplier();
		for (const FUSPendingSpawn& Spawn : Batch)
		{
			SpawnEnemyAt(Spawn.EnemyType, Spawn.Location, HealthMultiplier);
		}
		return;
	}

	if (SpawnMarkers)
	{
		for (const FUSPendingSpawn& Spawn : Batch)
		{
			// 스폰 높이(캡슐 반높이)에서 바닥으로 내리고, 바닥과 겹쳐 깜빡이지 않게 살짝 띄운다 (Ring 무리 X와 같은 방식)
			constexpr float MarkerFloorLift = 2.0f;
			SpawnMarkers->PlayPulse(Spawn.Location - FVector(0.0f, 0.0f, SpawnHeightOffset - MarkerFloorLift), SpawnMarkerRadius);
		}
	}

	// 끝난 예고 타이머는 여기서 정리한다 (웨이브마다 하나씩 쌓이므로)
	PendingWaveTimerHandles.RemoveAll([this](const FTimerHandle& Handle)
	{
		return !GetWorldTimerManager().IsTimerActive(Handle);
	});

	PendingSpawnCount += Batch.Num();
	FTimerHandle& Handle = PendingWaveTimerHandles.AddDefaulted_GetRef();
	GetWorldTimerManager().SetTimer(Handle,
		FTimerDelegate::CreateUObject(this, &AUSEnemySpawner::SpawnPendingWave, MoveTemp(Batch)), SpawnWarningTime, false);
}

void AUSEnemySpawner::SpawnPendingWave(TArray<FUSPendingSpawn> Batch)
{
	PendingSpawnCount = FMath::Max(0, PendingSpawnCount - Batch.Num());

	const float HealthMultiplier = GetHealthMultiplier();
	for (const FUSPendingSpawn& Spawn : Batch)
	{
		SpawnEnemyAt(Spawn.EnemyType, Spawn.Location, HealthMultiplier);
	}
}

AUSEnemyCharacter* AUSEnemySpawner::SpawnEnemyAt(const UUSEnemyTypeData* EnemyType, const FVector& Location, float HealthMultiplier)
{
	UWorld* World = GetWorld();
	if (World == nullptr || EnemyType == nullptr || EnemyType->GetEnemyClass() == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: cannot spawn %s (no world, type or EnemyClass)"), *GetName(), *GetNameSafe(EnemyType));
		return nullptr;
	}

	// 종류·체력 배율을 BeginPlay 전에 넣어야 적이 BeginPlay에서 스탯을 적용할 수 있다 → 지연 스폰
	const FTransform SpawnTransform(FRotator::ZeroRotator, Location);
	AUSEnemyCharacter* Enemy = World->SpawnActorDeferred<AUSEnemyCharacter>(EnemyType->GetEnemyClass(), SpawnTransform,
		nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (Enemy == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: failed to spawn enemy %s"), *GetName(), *GetNameSafe(EnemyType));
		return nullptr;
	}

	Enemy->SetEnemyType(EnemyType, HealthMultiplier);

	// 보스는 졸개를 이 스포너로 내보내므로 BeginPlay 전에 알려 준다
	if (AUSBossEnemyCharacter* Boss = Cast<AUSBossEnemyCharacter>(Enemy))
	{
		Boss->SetSpawner(this);
	}

	Enemy->FinishSpawning(SpawnTransform);
	ActiveEnemies.Add(Enemy);

	SpawnedEnemyTypes.AddUnique(EnemyType);
	return Enemy;
}

AUSBossEnemyCharacter* AUSEnemySpawner::StartBossPhase()
{
	if (BossEnemyType == nullptr || bBossPhase)
	{
		return nullptr;
	}

	AActor* Player = GetPlayerActor();
	if (Player == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: no player; cannot spawn boss"), *GetName());
		return nullptr;
	}

	FVector Location;
	if (!FindSpawnLocation(Player->GetActorLocation(), FMath::FRandRange(0.0f, 2.0f * PI), PI, Location))
	{
		return nullptr;
	}

	AUSBossEnemyCharacter* Boss = Cast<AUSBossEnemyCharacter>(SpawnEnemyAt(BossEnemyType, Location, 1.0f));
	if (Boss == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: %s did not spawn an AUSBossEnemyCharacter (check its EnemyClass)"), *GetName(), *GetNameSafe(BossEnemyType));
		return nullptr;
	}

	// 예고 중인 웨이브도 취소한다 — 정리 직후 X 자리에서 적이 다시 튀어나오면 정리한 느낌이 사라진다
	for (FTimerHandle& Handle : PendingWaveTimerHandles)
	{
		GetWorldTimerManager().ClearTimer(Handle);
	}
	PendingWaveTimerHandles.Reset();
	PendingSpawnCount = 0;

	if (bClearEnemiesOnBoss)
	{
		const int32 Cleared = ClearAllEnemies(Boss);
		UE_LOG(LogUS, Log, TEXT("%s: cleared %d enemies for the boss"), *GetName(), Cleared);
	}

	// 무리 이벤트와 난이도 상승은 멈추고, 웨이브는 상한만 줄여 계속 돈다 (화면이 휑하지 않게)
	bBossPhase = true;
	GetWorldTimerManager().ClearTimer(DifficultyTimerHandle);
	for (FTimerHandle& Handle : HordeTimerHandles)
	{
		GetWorldTimerManager().ClearTimer(Handle);
	}
	MaxAliveBeforeBoss = MaxAliveEnemies;
	MaxAliveEnemies = FMath::Min(MaxAliveEnemies, BossPhaseMaxAlive);

	UE_LOG(LogUS, Log, TEXT("%s: boss %s spawned (HP %.0f), max alive %d -> %d"), *GetName(), *GetNameSafe(BossEnemyType),
		Boss->GetMaxHealth(), MaxAliveBeforeBoss, MaxAliveEnemies);
	return Boss;
}

int32 AUSEnemySpawner::ClearAllEnemies(const AActor* Except)
{
	// 배열을 복사해 도는 이유: 격추가 연쇄 폭발로 다른 적을 지우며 ActiveEnemies가 바뀔 수 있다
	const TArray<TWeakObjectPtr<AUSEnemyCharacter>> Snapshot = ActiveEnemies;
	int32 Cleared = 0;
	for (const TWeakObjectPtr<AUSEnemyCharacter>& Weak : Snapshot)
	{
		AUSEnemyCharacter* Enemy = Weak.Get();
		if (Enemy && Enemy != Except && Enemy->IsAlive())
		{
			UGameplayStatics::ApplyDamage(Enemy, 1.0e9f, nullptr, this, UDamageType::StaticClass());
			++Cleared;
		}
	}
	return Cleared;
}

void AUSEnemySpawner::EndBossPhase()
{
	if (!bBossPhase)
	{
		return;
	}

	bBossPhase = false;
	MaxAliveEnemies = FMath::Max(MaxAliveEnemies, MaxAliveBeforeBoss);
	if (DifficultyRampInterval > 0.0f && EnemiesPerWaveIncrease > 0)
	{
		GetWorldTimerManager().SetTimer(DifficultyTimerHandle, this, &AUSEnemySpawner::RampDifficulty, DifficultyRampInterval, true);
	}
}

void AUSEnemySpawner::SpawnMinions(const UUSEnemyTypeData* MinionType, const FVector& Center, int32 Count, float Radius, float HealthMultiplier)
{
	if (MinionType == nullptr || Count <= 0)
	{
		return;
	}

	TArray<FUSPendingSpawn> Batch;
	Batch.Reserve(Count);
	const float BaseAngle = FMath::FRandRange(0.0f, 2.0f * PI);
	const float Step = 2.0f * PI / Count;
	for (int32 Index = 0; Index < Count; ++Index)
	{
		FVector Location;
		if (USLocationUtils::TryFindFloorLocationNearAngle(GetWorld(), Center, BaseAngle + Step * Index, Step * 0.25f,
			Radius * 0.6f, Radius, SpawnHeightOffset, Location))
		{
			Batch.Add({ MinionType, Location });
		}
	}

	if (Batch.IsEmpty())
	{
		return;
	}

	if (SpawnWarningTime <= 0.0f)
	{
		for (const FUSPendingSpawn& Spawn : Batch)
		{
			SpawnEnemyAt(Spawn.EnemyType, Spawn.Location, HealthMultiplier);
		}
		return;
	}

	if (SpawnMarkers)
	{
		constexpr float MarkerFloorLift = 2.0f;
		for (const FUSPendingSpawn& Spawn : Batch)
		{
			SpawnMarkers->PlayPulse(Spawn.Location - FVector(0.0f, 0.0f, SpawnHeightOffset - MarkerFloorLift), SpawnMarkerRadius);
		}
	}

	// 웨이브 예고와 같은 길이로 기다렸다가 X 자리에서 나온다. 체력 배율은 보스 데이터 고정값
	FTimerHandle& Handle = PendingWaveTimerHandles.AddDefaulted_GetRef();
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this, Batch = MoveTemp(Batch), HealthMultiplier]()
	{
		for (const FUSPendingSpawn& Spawn : Batch)
		{
			SpawnEnemyAt(Spawn.EnemyType, Spawn.Location, HealthMultiplier);
		}
	}), SpawnWarningTime, false);
}

void AUSEnemySpawner::HandleHordeWarningTime(int32 EventIndex)
{
	const AActor* Player = GetPlayerActor();
	if (Player == nullptr || !HordeEvents.IsValidIndex(EventIndex))
	{
		return;
	}

	const FUSHordeEvent& Event = HordeEvents[EventIndex];
	const FVector PlayerLocation = Player->GetActorLocation();

	// 자리는 예고 때 정한다 — 화살표가 가리킨 쪽, 바닥 X가 찍힌 곳에서 실제로 나와야 한다 (Brotato 스폰 표시와 같은 약속).
	// Ring은 예고 시점의 대장 자리를 중심으로 잡는다 (예고 동안 움직인 만큼 고리 한쪽이 가까워진다 = 피할 틈)
	FVector Center = PlayerLocation;
	if (Event.Shape == EUSHordeShape::Rush)
	{
		const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
		if (!USLocationUtils::TryFindFloorLocationNearAngle(GetWorld(), PlayerLocation, Angle, PI / 4.0f,
			Event.Distance, Event.Distance, 0.0f, Center))
		{
			UE_LOG(LogUS, Warning, TEXT("%s: HordeEvents[%d] found no floor %.0f away from player; skipping"),
				*GetName(), EventIndex, Event.Distance);
			return;
		}
	}

	const int32 CoreCount = Event.CoreEnemyType ? Event.CoreCount : 0;
	TArray<FVector>& Locations = PendingHordeLocations[EventIndex];
	BuildHordeLocations(Event, Center, Event.Count + CoreCount, Locations);
	if (Locations.IsEmpty())
	{
		UE_LOG(LogUS, Warning, TEXT("%s: HordeEvents[%d] found no floor for any member; skipping"), *GetName(), EventIndex);
		return;
	}

	// 두 모양 다 바닥에 표시한다. Rush 거리(1500)도 웨이브 반경처럼 화면 안이라 X가 없으면 뿅 하고 나온다
	if (SpawnMarkers)
	{
		for (const FVector& Location : Locations)
		{
			// 스폰 높이(캡슐 반높이)에서 바닥으로 내리고, 바닥과 겹쳐 깜빡이지 않게 살짝 띄운다
			constexpr float MarkerFloorLift = 2.0f;
			SpawnMarkers->PlayPulse(Location - FVector(0.0f, 0.0f, SpawnHeightOffset - MarkerFloorLift), SpawnMarkerRadius);
		}
	}

	OnHordeWarning.Broadcast(Event.Shape, Center, Event.WarningTime);

	if (Event.WarningTime <= 0.0f)
	{
		SpawnHorde(EventIndex);
		return;
	}

	GetWorldTimerManager().SetTimer(HordeTimerHandles[EventIndex],
		FTimerDelegate::CreateUObject(this, &AUSEnemySpawner::SpawnHorde, EventIndex), Event.WarningTime, false);
}

void AUSEnemySpawner::SpawnHorde(int32 EventIndex)
{
	if (!HordeEvents.IsValidIndex(EventIndex) || !PendingHordeLocations.IsValidIndex(EventIndex))
	{
		return;
	}

	const FUSHordeEvent& Event = HordeEvents[EventIndex];
	const TArray<FVector>& Locations = PendingHordeLocations[EventIndex];
	const int32 CoreCount = Event.CoreEnemyType ? Event.CoreCount : 0;

	// 섞는 적(기뢰)은 Rush면 덩어리 가운데(위치 0번 = 중심), Ring이면 고리 위에 고르게 둔다
	const float HealthMultiplier = GetHealthMultiplier();
	const int32 CoreStride = CoreCount > 0 ? FMath::Max(1, Locations.Num() / CoreCount) : 0;
	int32 CoresPlaced = 0;
	for (int32 Index = 0; Index < Locations.Num(); ++Index)
	{
		const bool bCore = CoresPlaced < CoreCount && Index % CoreStride == 0;
		CoresPlaced += bCore ? 1 : 0;
		SpawnEnemyAt(bCore ? Event.CoreEnemyType.Get() : Event.EnemyType.Get(), Locations[Index], HealthMultiplier);
	}

	PendingHordeLocations[EventIndex].Empty();
}

void AUSEnemySpawner::BuildHordeLocations(const FUSHordeEvent& Event, const FVector& Center, int32 Total,
	TArray<FVector>& OutLocations) const
{
	const UWorld* World = GetWorld();
	OutLocations.Reset(Total);

	for (int32 Index = 0; Index < Total; ++Index)
	{
		FVector Candidate;
		if (Event.Shape == EUSHordeShape::Ring)
		{
			// 고리를 Total등분해 빈틈없이 둘러싼다. 벽 밖 자리는 빠져서 그쪽이 탈출구가 된다
			const float Angle = 2.0f * PI * Index / Total;
			Candidate = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Event.Distance;
		}
		else
		{
			// 0번은 정확히 중심(기뢰 자리), 나머지는 원판 안에 고르게 (반지름에 sqrt를 씌워야 가운데로 몰리지 않는다)
			const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
			const float Radius = Index == 0 ? 0.0f : Event.ClusterRadius * FMath::Sqrt(FMath::FRand());
			Candidate = Center + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Radius;
		}

		FVector FloorLocation;
		if (USLocationUtils::TryFindFloorLocationAt(World, Candidate, Center.Z, SpawnHeightOffset, FloorLocation)
			&& USLocationUtils::HasClearPathFlat(World, Center, FloorLocation))
		{
			OutLocations.Add(FloorLocation);
		}
	}
}

void AUSEnemySpawner::RampDifficulty()
{
	EnemiesPerWave = FMath::Min(EnemiesPerWave + EnemiesPerWaveIncrease, EnemiesPerWaveMax);
	MaxAliveEnemies = FMath::Min(MaxAliveEnemies + MaxAliveIncreasePerRamp, MaxAliveEnemiesCap);

	// 웨이브 간격 단축 → 시간이 갈수록 압박이 거세진다.
	const float NewInterval = FMath::Max(SpawnIntervalMin, SpawnInterval - SpawnIntervalDecreasePerRamp);
	if (!FMath::IsNearlyEqual(NewInterval, SpawnInterval))
	{
		SpawnInterval = NewInterval;
		// 새 주기로 스폰 타이머 재설정
		GetWorldTimerManager().SetTimer(
			SpawnTimerHandle,
			this,
			&AUSEnemySpawner::SpawnWave,
			SpawnInterval,
			true);
	}
}

int32 AUSEnemySpawner::PurgeAndCountAlive()
{
	for (int32 i = ActiveEnemies.Num() - 1; i >= 0; --i)
	{
		AUSEnemyCharacter* Enemy = ActiveEnemies[i].Get();

		// 무효(파괴/GC)·파괴 진행 중·사망한 적은 슬롯을 비운다.
		// → 어떤 경로로 사라지든(맵 밖 추락 등) 카운트가 새지 않는다.
		if (Enemy == nullptr || Enemy->IsActorBeingDestroyed() || !Enemy->IsAlive())
		{
			ActiveEnemies.RemoveAtSwap(i);
		}
	}

	return ActiveEnemies.Num();
}

bool AUSEnemySpawner::FindSpawnLocation(const FVector& Center, float Angle, float AngleJitter, FVector& OutLocation) const
{
	if (USLocationUtils::TryFindFloorLocationNearAngle(GetWorld(), Center, Angle, AngleJitter, SpawnRadiusMin, SpawnRadiusMax,
		SpawnHeightOffset, OutLocation))
	{
		return true;
	}

	// 예전에는 여기서 허공(플레이어 높이)에 그대로 스폰해 벽 밖에 적이 생겼다. 이제는 이번 스폰을 건너뛴다.
	UE_LOG(LogUS, Warning, TEXT("%s: no spawn location on navmesh or floor in ring %.0f-%.0f around %s; skipping this spawn"),
		*GetName(), SpawnRadiusMin, SpawnRadiusMax, *Center.ToCompactString());
	return false;
}

AActor* AUSEnemySpawner::GetPlayerActor() const
{
	return UGameplayStatics::GetPlayerPawn(this, 0);
}
