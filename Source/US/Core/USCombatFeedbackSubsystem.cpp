#include "Core/USCombatFeedbackSubsystem.h"

void UUSCombatFeedbackSubsystem::ReportEnemyDamaged(const FVector& Location, float Damage, bool bByPlayer)
{
	OnEnemyDamaged.Broadcast(Location, Damage, bByPlayer);
}

void UUSCombatFeedbackSubsystem::ReportEnemyKilled()
{
	OnEnemyKilled.Broadcast();
}

void UUSCombatFeedbackSubsystem::ReportPlayerKilledEnemy(const FVector& Location)
{
	OnPlayerKilledEnemy.Broadcast(Location);
}
