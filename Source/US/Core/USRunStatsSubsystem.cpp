#include "Core/USRunStatsSubsystem.h"
#include "Core/USCombatFeedbackSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"

bool UUSRunStatsSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UUSRunStatsSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// 서브시스템끼리는 생성 순서가 정해져 있지 않아 Initialize가 아니라 BeginPlay에서 찾는다
	CombatFeedback = InWorld.GetSubsystem<UUSCombatFeedbackSubsystem>();
	if (CombatFeedback)
	{
		CombatFeedback->OnEnemyDamaged.AddUObject(this, &UUSRunStatsSubsystem::HandleEnemyDamaged);
		CombatFeedback->OnEnemyKilled.AddUObject(this, &UUSRunStatsSubsystem::HandleEnemyKilled);
	}

	CurrentBucketSecond = FMath::FloorToInt(InWorld.GetTimeSeconds());
	InWorld.GetTimerManager().SetTimer(DpsTimerHandle, FTimerDelegate::CreateUObject(this, &UUSRunStatsSubsystem::RefreshDps),
		DpsRefreshInterval, true);
}

void UUSRunStatsSubsystem::Deinitialize()
{
	if (CombatFeedback)
	{
		CombatFeedback->OnEnemyDamaged.RemoveAll(this);
		CombatFeedback->OnEnemyKilled.RemoveAll(this);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DpsTimerHandle);
	}

	Super::Deinitialize();
}

void UUSRunStatsSubsystem::HandleEnemyDamaged(const FVector& /*Location*/, float Damage, bool bByPlayer)
{
	const UWorld* World = GetWorld();
	if (World == nullptr || Damage <= 0.0f)
	{
		return;
	}

	AdvanceBuckets(World->GetTimeSeconds());

	const int32 Index = CurrentBucketSecond % DpsWindowSeconds;
	TotalDamageBuckets[Index] += Damage;
	if (!bByPlayer)
	{
		AllyDamageBuckets[Index] += Damage;
	}
}

void UUSRunStatsSubsystem::HandleEnemyKilled()
{
	++KillCount;
	OnKillCountChanged.Broadcast(KillCount);
}

void UUSRunStatsSubsystem::AdvanceBuckets(float Now)
{
	const int32 NowSecond = FMath::FloorToInt(Now);
	if (NowSecond <= CurrentBucketSecond)
	{
		return;
	}

	// 그 사이 지나간 칸(최대 칸 수만큼)은 옛 피해가 남아 있으므로 비운다
	const int32 StepsToClear = FMath::Min(NowSecond - CurrentBucketSecond, DpsWindowSeconds);
	for (int32 Step = 1; Step <= StepsToClear; ++Step)
	{
		const int32 Index = (CurrentBucketSecond + Step) % DpsWindowSeconds;
		TotalDamageBuckets[Index] = 0.0f;
		AllyDamageBuckets[Index] = 0.0f;
	}

	CurrentBucketSecond = NowSecond;
}

void UUSRunStatsSubsystem::RefreshDps()
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	// 한동안 아무도 안 맞아도 옛 칸이 비워지도록 먼저 민다
	AdvanceBuckets(World->GetTimeSeconds());

	float TotalDamage = 0.0f;
	float AllyDamage = 0.0f;
	for (int32 Index = 0; Index < DpsWindowSeconds; ++Index)
	{
		TotalDamage += TotalDamageBuckets[Index];
		AllyDamage += AllyDamageBuckets[Index];
	}

	SquadDps = TotalDamage / DpsWindowSeconds;
	AllyDps = AllyDamage / DpsWindowSeconds;
	OnSquadDpsChanged.Broadcast(SquadDps, AllyDps);
}
