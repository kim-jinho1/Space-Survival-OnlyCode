#include "Core/USTeamSubsystem.h"
#include "Core/USLocationUtils.h"
#include "Agents/USEnemyCharacter.h"
#include "GameFramework/Actor.h"

void UUSTeamSubsystem::RegisterAlly(AActor* Actor)
{
	if (Actor)
	{
		Allies.AddUnique(Actor);
	}
}

void UUSTeamSubsystem::UnregisterAlly(AActor* Actor)
{
	Allies.RemoveSwap(Actor);
}

void UUSTeamSubsystem::RegisterEnemy(AActor* Actor)
{
	if (Actor)
	{
		Enemies.AddUnique(Actor);
	}
}

void UUSTeamSubsystem::UnregisterEnemy(AActor* Actor)
{
	Enemies.RemoveSwap(Actor);
}

void UUSTeamSubsystem::RegisterTaunter(AActor* Actor, float Radius)
{
	if (Actor && Radius > 0.0f)
	{
		Taunters.Add(Actor, Radius);
	}
}

void UUSTeamSubsystem::UnregisterTaunter(AActor* Actor)
{
	Taunters.Remove(Actor);
}

AActor* UUSTeamSubsystem::FindTaunterFor(const FVector& From) const
{
	AActor* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();

	// 도발 대상은 방패기 몇 대뿐이라 선형 탐색으로 충분하다
	for (const TPair<TObjectPtr<AActor>, float>& Pair : Taunters)
	{
		AActor* Taunter = Pair.Key;
		if (Taunter == nullptr || !IsValid(Taunter))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(From, Taunter->GetActorLocation());
		if (DistSq <= FMath::Square(Pair.Value) && DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Taunter;
		}
	}

	return Nearest;
}

AActor* UUSTeamSubsystem::FindNearestEnemy(const FVector& From, float MaxDistance, const FUSScreenBounds* Screen) const
{
	return FindNearest(Enemies, From, MaxDistance, Screen);
}

AActor* UUSTeamSubsystem::FindEnemyAtPoint(const FVector& Point, float Padding) const
{
	AActor* Best = nullptr;
	float BestBoxDistSq = FMath::Square(Padding);
	float BestCenterDistSq = TNumericLimits<float>::Max();

	for (AActor* Actor : Enemies)
	{
		const AUSEnemyCharacter* Enemy = Cast<AUSEnemyCharacter>(Actor);
		if (Enemy == nullptr || !IsValid(Enemy) || !Enemy->IsAlive())
		{
			continue;
		}

		// 탑뷰라 높이는 무시한다 (커서 평면과 기체 높이가 조금 달라도 같은 자리로 본다)
		const FBox Bounds = Enemy->GetPickBounds();
		const FVector FlatPoint(Point.X, Point.Y, Bounds.GetCenter().Z);
		const float BoxDistSq = Bounds.ComputeSquaredDistanceToPoint(FlatPoint);
		const float CenterDistSq = FVector::DistSquared2D(Point, Enemy->GetActorLocation());
		if (BoxDistSq < BestBoxDistSq || (BoxDistSq == BestBoxDistSq && CenterDistSq < BestCenterDistSq))
		{
			BestBoxDistSq = BoxDistSq;
			BestCenterDistSq = CenterDistSq;
			Best = Actor;
		}
	}

	return Best;
}

AActor* UUSTeamSubsystem::FindNearestAlly(const FVector& From, float MaxDistance) const
{
	return FindNearest(Allies, From, MaxDistance);
}

AActor* UUSTeamSubsystem::FindNearest(const TArray<TObjectPtr<AActor>>& Actors, const FVector& From, float MaxDistance,
	const FUSScreenBounds* Screen)
{
	AActor* Nearest = nullptr;
	// 제곱 거리로 비교해 sqrt를 피한다. 거리 제한이 없으면 최댓값에서 시작.
	float NearestDistSq = (MaxDistance > 0.0f) ? FMath::Square(MaxDistance) : TNumericLimits<float>::Max();

	for (AActor* Actor : Actors)
	{
		if (Actor == nullptr || !IsValid(Actor))
		{
			continue;
		}

		// 날아오는 볼트만으로 죽을 적은 건너뛴다 (같은 적에 탄을 몰아 낭비하지 않게)
		const AUSEnemyCharacter* Enemy = Cast<AUSEnemyCharacter>(Actor);
		if (Enemy && Enemy->IsDoomed())
		{
			continue;
		}

		// 적은 몸 가장자리까지 잰다 (큰 보스는 중심이 멀어도 몸이 닿아 있다). 아군은 중심
		const FVector Point = Enemy ? Enemy->GetClosestPickPoint(From) : Actor->GetActorLocation();
		const float DistSq = Enemy ? FVector::DistSquared2D(From, Point) : FVector::DistSquared(From, Point);
		// 거리 비교가 싸므로 먼저 하고, 더 가까운 후보만 화면 투영으로 확인한다
		if (DistSq <= NearestDistSq && (Screen == nullptr || Screen->Contains(Point)))
		{
			NearestDistSq = DistSq;
			Nearest = Actor;
		}
	}

	return Nearest;
}
