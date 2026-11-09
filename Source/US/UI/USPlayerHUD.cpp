#include "UI/USPlayerHUD.h"
#include "USExperienceBar.h"
#include "USHealthBar.h"
#include "USQuickInfoUI.h"
#include "Components/USAttributeComponent.h"
#include "Components/USCurrencyComponent.h"
#include "Components/USExperienceComponent.h"
#include "Core/USRunStatsSubsystem.h"
#include "UI/USClockText.h"
#include "US.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"

UUSPlayerHUD::UUSPlayerHUD(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RemainingTimeFormat = NSLOCTEXT("USPlayerHUD", "RemainingTimeFormat", "남은 시간  {0}:{1}");
	ContinueTimeFormat = NSLOCTEXT("USPlayerHUD", "ContinueTimeFormat", "계속 가기  +{0}:{1}");
	BossFightTimeText = NSLOCTEXT("USPlayerHUD", "BossFightTimeText", "보스 등장!");
}

void UUSPlayerHUD::SetRunTime(float Seconds, bool bContinuing) const
{
	if (RunTimeText == nullptr)
	{
		return;
	}

	FText Minutes;
	FText SecondsText;
	USClockText::Split(Seconds, Minutes, SecondsText);
	RunTimeText->SetText(FText::Format(bContinuing ? ContinueTimeFormat : RemainingTimeFormat, Minutes, SecondsText));
}

void UUSPlayerHUD::ShowBossFightTime() const
{
	if (RunTimeText)
	{
		RunTimeText->SetText(BossFightTimeText);
	}
}

void UUSPlayerHUD::InitializeForPawn(APawn* InPawn)
{
	if (InPawn == nullptr)
	{
		return;
	}

	if (UUSAttributeComponent* AttributeComponent = InPawn->FindComponentByClass<UUSAttributeComponent>())
	{
		AttributeComponent->OnHealthChanged.AddUObject(this, &UUSPlayerHUD::OnHealthChanged);
		OnHealthChanged(AttributeComponent->GetCurrentHealth(), AttributeComponent->GetMaxHealth());
	}

	if (UUSExperienceComponent* ExperienceComponent = InPawn->FindComponentByClass<UUSExperienceComponent>())
	{
		ExperienceComponent->OnExperiencePointsChanged.AddUObject(this, &UUSPlayerHUD::OnExperienceChanged);
		ExperienceComponent->OnLevelChanged.AddUObject(this, &UUSPlayerHUD::OnLevelChanged);

		OnExperienceChanged(
			ExperienceComponent->GetCurrentExperiencePoints(),
			ExperienceComponent->GetTargetExperiencePoints());
		OnLevelChanged(ExperienceComponent->GetCurrentLevel());
	}

	if (UUSCurrencyComponent* CurrencyComponent = InPawn->FindComponentByClass<UUSCurrencyComponent>())
	{
		CurrencyComponent->OnGoldChanged.AddUObject(this, &UUSPlayerHUD::OnQuickInfoChanged);

		OnQuickInfoChanged(CurrencyComponent->GetGold());
	}

	// 폰이 바뀌어도(다시 하기) 같은 월드의 통계를 보므로 중복 바인딩을 먼저 지운다
	if (UUSRunStatsSubsystem* RunStats = GetWorld() ? GetWorld()->GetSubsystem<UUSRunStatsSubsystem>() : nullptr)
	{
		RunStats->OnKillCountChanged.RemoveAll(this);
		RunStats->OnSquadDpsChanged.RemoveAll(this);
		RunStats->OnKillCountChanged.AddUObject(this, &UUSPlayerHUD::HandleKillCountChanged);
		RunStats->OnSquadDpsChanged.AddUObject(this, &UUSPlayerHUD::HandleSquadDpsChanged);

		HandleKillCountChanged(RunStats->GetKillCount());
		HandleSquadDpsChanged(RunStats->GetSquadDps(), RunStats->GetAllyDps());
	}
	else
	{
		UE_LOG(LogUS, Warning, TEXT("%s: no UUSRunStatsSubsystem; kill count and squad DPS will not update"), *GetName());
	}
}

void UUSPlayerHUD::HandleKillCountChanged(int32 KillCount) const
{
	if (QuickInfoUI)
	{
		QuickInfoUI->SetKillCount(KillCount);
	}
}

void UUSPlayerHUD::HandleSquadDpsChanged(float SquadDps, float AllyDps) const
{
	if (QuickInfoUI)
	{
		QuickInfoUI->SetSquadDps(SquadDps, GetSafeRatio(AllyDps, SquadDps));
	}
}

void UUSPlayerHUD::OnHealthChanged(const float CurrentValue, const float MaxValue) const
{
	if (HealthBarUI)
	{
		HealthBarUI->SetRatio(GetSafeRatio(CurrentValue, MaxValue));
		HealthBarUI->SetText(CurrentValue, MaxValue);
	}
}

void UUSPlayerHUD::OnExperienceChanged(const float CurrentExperience, const float TargetExperience) const
{
	if (ExperienceBarUI)
	{
		ExperienceBarUI->SetRatio(GetSafeRatio(CurrentExperience, TargetExperience));
		ExperienceBarUI->SetExperience(CurrentExperience, TargetExperience);
	}
}

void UUSPlayerHUD::OnLevelChanged(const int Value) const
{
	if (LevelText)
	{
		LevelText->SetText(FText::AsNumber(Value));
	}

	if (ExperienceBarUI)
	{
		ExperienceBarUI->SetLevel(Value);
	}
}

void UUSPlayerHUD::OnQuickInfoChanged(const int32 Value) const
{
	if (QuickInfoUI)
	{
		QuickInfoUI->SetGoldAmount(Value);
	}
}

float UUSPlayerHUD::GetSafeRatio(float CurrentValue, float MaxValue)
{
	return MaxValue > 0.0f ? CurrentValue / MaxValue : 0.0f;
}
