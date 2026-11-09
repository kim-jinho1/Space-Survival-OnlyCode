#include "Components/USSquadComponent.h"
#include "US.h"
#include "Agents/USAllyDroneCharacter.h"
#include "Agents/USEnemyCharacter.h"
#include "Agents/USPlayerCharacter.h"
#include "Components/USAttributeComponent.h"
#include "Components/USCurrencyComponent.h"
#include "Components/USEmpPulseComponent.h"
#include "Core/USTeamSubsystem.h"
#include "Datas/USAbilityData.h"
#include "Datas/USAllyTypeData.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

UUSSquadComponent::UUSSquadComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// 기본 진형 (몸길이 약 80uu 기준 간격): 탱커는 이동 방향 앞에서 막고, 원거리는 뒤 두 줄, 슬로우는 대장 양옆.
	Slots = {
		{ FVector2D(130.0f, -70.0f), EUSAllyRole::Tank }, { FVector2D(130.0f, 70.0f), EUSAllyRole::Tank },
		{ FVector2D(-100.0f, -80.0f), EUSAllyRole::Ranged }, { FVector2D(-100.0f, 80.0f), EUSAllyRole::Ranged },
		{ FVector2D(-180.0f, -40.0f), EUSAllyRole::Ranged }, { FVector2D(-180.0f, 40.0f), EUSAllyRole::Ranged },
		{ FVector2D(0.0f, -150.0f), EUSAllyRole::Slow }, { FVector2D(0.0f, 150.0f), EUSAllyRole::Slow },
	};
}

void UUSSquadComponent::BeginPlay()
{
	Super::BeginPlay();

	Leader = Cast<AUSPlayerCharacter>(GetOwner());
	if (Leader == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSSquadComponent: owner %s is not an AUSPlayerCharacter"), *GetNameSafe(GetOwner()));
		return;
	}

	LeaderCurrency = Leader->GetCurrencyComponent();
	if (LeaderCurrency == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSSquadComponent: %s has no currency component; revive is disabled"), *GetNameSafe(Leader));
	}

	Members.SetNum(MaxMembers);
	FormationYaw = Leader ? Leader->GetActorRotation().Yaw : 0.0f;
	GuardYaw = FormationYaw;
	RefreshAvailableHireableTypes();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(GuardTimerHandle, this, &UUSSquadComponent::UpdateGuardThreat, GuardUpdateInterval, true);
	}

	// 출격 편성이 있으면 그것을, 없으면(격납고를 거치지 않은 맵 등) BP의 InitialAllyTypes를 스폰한다.
	// 편성은 컨트롤러가 빙의(OnPossess) 때 넣어 두었다 — 빙의는 월드 BeginPlay보다 먼저 일어난다
	if (bHasStartingOverride)
	{
		for (const UUSAllyTypeData* AllyType : StartingAllyOverride)
		{
			SpawnStartingAlly(AllyType);
		}
	}
	else
	{
		for (const UUSAllyTypeData* AllyType : InitialAllyTypes)
		{
			SpawnStartingAlly(AllyType);
		}
	}

	OnSquadChanged.Broadcast();
}

void UUSSquadComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GuardTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UUSSquadComponent::SetStartingAllies(const TArray<const UUSAllyTypeData*>& AllyTypes)
{
	if (HasBegunPlay())
	{
		// 이미 InitialAllyTypes로 시작했다. 늦게 온 편성은 더하기만 한다 (정상 흐름에서는 일어나지 않으므로 알린다)
		UE_LOG(LogUS, Warning, TEXT("UUSSquadComponent: SetStartingAllies after BeginPlay; adding %d allies"), AllyTypes.Num());
		for (const UUSAllyTypeData* AllyType : AllyTypes)
		{
			SpawnStartingAlly(AllyType);
		}
		OnSquadChanged.Broadcast();
		return;
	}

	bHasStartingOverride = true;
	StartingAllyOverride.Reset(AllyTypes.Num());
	for (const UUSAllyTypeData* AllyType : AllyTypes)
	{
		StartingAllyOverride.Add(AllyType);
	}
}

void UUSSquadComponent::SetLockedTypes(const TArray<const UUSAllyTypeData*>& Types)
{
	LockedTypes.Reset(Types.Num());
	for (const UUSAllyTypeData* Type : Types)
	{
		LockedTypes.Add(Type);
	}

	RefreshAvailableHireableTypes();
}

bool UUSSquadComponent::SpawnStartingAlly(const UUSAllyTypeData* AllyType)
{
	const int32 SlotIndex = AllyType ? FindHireSlot(AllyType->GetRole()) : INDEX_NONE;
	if (SlotIndex == INDEX_NONE)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSSquadComponent: no slot for starting ally %s"), *GetNameSafe(AllyType));
		return false;
	}

	return SpawnAllyInSlot(AllyType, SlotIndex) != nullptr;
}

void UUSSquadComponent::RefreshAvailableHireableTypes()
{
	AvailableHireableTypes.Reset(HireableTypes.Num());
	for (UUSAllyTypeData* Type : HireableTypes)
	{
		if (Type && !LockedTypes.Contains(Type))
		{
			AvailableHireableTypes.Add(Type);
		}
	}
}

void UUSSquadComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (Leader == nullptr)
	{
		return;
	}

	// 진형 방향은 이동 방향을 제한 속도로 따라간다 (기획서 개정 2026-10-18: 조준 → 이동)
	FormationYaw = FMath::FixedTurn(FormationYaw, GetFormationTargetYaw(), FormationTurnRate * DeltaTime);

	// 탱커 방향: 위협이 있으면 위협 쪽, 없으면 진형 방향으로 돌아온다 (위협이 다시 생겨도 진형 앞에서 출발해 튀지 않게)
	GuardYaw = FMath::FixedTurn(GuardYaw, bHasGuardThreat ? GuardTargetYaw : FormationYaw, GuardTurnRate * DeltaTime);
}

bool UUSSquadComponent::TryRevive(int32 SlotIndex)
{
	if (!Members.IsValidIndex(SlotIndex) || !Members[SlotIndex].bDown || !TrySpendGold(GetReviveCost()))
	{
		PlayFeedbackSound(DeniedSound);
		return false;
	}

	// 옛 몸을 치우고 같은 종류를 같은 자리에 새로 스폰한다 (상태를 되돌리는 것보다 초기화가 확실하다).
	// 종류 강화는 스폰할 때 다시 적용되므로 부활해도 유지된다
	const UUSAllyTypeData* AllyType = Members[SlotIndex].Type;
	ClearSlot(SlotIndex);

	++ReviveCount;
	SpawnAllyInSlot(AllyType, SlotIndex);
	PlayFeedbackSound(ReviveSound);
	OnSquadChanged.Broadcast();
	return true;
}

bool UUSSquadComponent::TryAddFreeAlly(const UUSAllyTypeData* AllyType)
{
	if (AllyType == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSSquadComponent: TryAddFreeAlly called without an ally type"));
		return false;
	}

	const int32 SlotIndex = FindHireSlot(AllyType->GetRole());
	if (SlotIndex == INDEX_NONE)
	{
		// 카드 후보를 만들 때 CanAddAlly로 걸렀으므로 여기 오면 후보 생성과 적용 사이에 자리가 찬 것이다
		UE_LOG(LogUS, Warning, TEXT("UUSSquadComponent: no slot for free ally %s"), *GetNameSafe(AllyType));
		return false;
	}

	ClearSlot(SlotIndex);
	if (SpawnAllyInSlot(AllyType, SlotIndex) == nullptr)
	{
		return false;
	}

	PlayFeedbackSound(HireSound);
	OnSquadChanged.Broadcast();
	return true;
}

void UUSSquadComponent::SetAbilityLevel(const UUSAbilityData* Ability, int32 Level)
{
	if (Ability == nullptr || Ability->IsLeaderAbility())
	{
		UE_LOG(LogUS, Warning, TEXT("UUSSquadComponent: SetAbilityLevel needs an ally ability, got %s"), *GetNameSafe(Ability));
		return;
	}

	AbilityLevels.Add(Ability, Level);

	// 단계 수치는 절대값이라 덮어써도 된다. 격추된 부하는 부활할 때 스폰하면서 다시 받으므로 살아 있는 부하에만 바로 적용한다
	for (const FUSSquadMember& Member : Members)
	{
		if (Member.IsActive() && Member.Type == Ability->GetOwnerType() && Member.Ally)
		{
			Ability->ApplyTo(Member.Ally, Level);
		}
	}
}

void UUSSquadComponent::ApplySquadUpgrade(EUSEffectType Effect, float Value)
{
	// 종류별 기록과 같은 배열에 Type == nullptr로 넣어, 스폰 때 재적용하는 루프 하나로 두 강화를 다 처리한다
	FUSTypeUpgrade& Upgrade = TypeUpgrades.AddDefaulted_GetRef();
	Upgrade.Type = nullptr;
	Upgrade.Effect = Effect;
	Upgrade.Value = Value;

	for (const FUSSquadMember& Member : Members)
	{
		UUSAttributeComponent* Attribute = (Member.IsActive() && Member.Ally) ? Member.Ally->GetAttributeComponent() : nullptr;
		if (Attribute)
		{
			Attribute->ApplyEffect(Effect, Value);
		}
	}
}

void UUSSquadComponent::AddSlowDurationBonus(float Bonus)
{
	SlowDurationBonus += Bonus;

	for (const FUSSquadMember& Member : Members)
	{
		UUSEmpPulseComponent* Pulse = (Member.IsActive() && Member.Ally) ? Member.Ally->FindComponentByClass<UUSEmpPulseComponent>() : nullptr;
		if (Pulse)
		{
			Pulse->SetSlowDurationBonus(SlowDurationBonus);
		}
	}
}

bool UUSSquadComponent::CanAddAlly() const
{
	// FindHireSlot은 역할이 안 맞아도 빈자리·격추 자리를 돌려주므로 역할은 아무거나 넣어도 된다
	return FindHireSlot(EUSAllyRole::Ranged) != INDEX_NONE;
}

void UUSSquadComponent::GetActiveTypes(TArray<const UUSAllyTypeData*>& OutTypes) const
{
	OutTypes.Reset();
	for (const FUSSquadMember& Member : Members)
	{
		if (Member.IsActive())
		{
			OutTypes.AddUnique(Member.Type);
		}
	}
}

void UUSSquadComponent::ClearSlot(int32 SlotIndex)
{
	if (!Members.IsValidIndex(SlotIndex))
	{
		return;
	}

	// 격추 자리를 덮어쓰면 숨겨 두었던 옛 부하는 치운다
	if (AUSAllyDroneCharacter* OldAlly = Members[SlotIndex].Ally)
	{
		OldAlly->Destroy();
	}
	Members[SlotIndex] = FUSSquadMember();
}

bool UUSSquadComponent::RepairAll(float Fraction)
{
	bool bRepaired = false;

	for (const FUSSquadMember& Member : Members)
	{
		UUSAttributeComponent* Attribute = (Member.IsActive() && Member.Ally) ? Member.Ally->GetAttributeComponent() : nullptr;
		if (Attribute == nullptr || Attribute->GetCurrentHealth() >= Attribute->GetMaxHealth())
		{
			continue;
		}

		Attribute->Heal(Attribute->GetMaxHealth() * Fraction);
		bRepaired = true;
	}

	return bRepaired;
}

bool UUSSquadComponent::HasDamagedMember() const
{
	for (const FUSSquadMember& Member : Members)
	{
		const UUSAttributeComponent* Attribute = (Member.IsActive() && Member.Ally) ? Member.Ally->GetAttributeComponent() : nullptr;
		if (Attribute && Attribute->GetCurrentHealth() < Attribute->GetMaxHealth())
		{
			return true;
		}
	}

	return false;
}

void UUSSquadComponent::HandleMemberDown(AUSAllyDroneCharacter* Member)
{
	if (Member == nullptr)
	{
		return;
	}

	const int32 Index = Members.IndexOfByPredicate([Member](const FUSSquadMember& Entry) { return Entry.Ally == Member; });
	if (Index != INDEX_NONE)
	{
		Members[Index].bDown = true;
		OnMemberDown.Broadcast(Index);
		OnSquadChanged.Broadcast();
	}
}

int32 UUSSquadComponent::GetReviveCost() const
{
	const float Cost = ReviveBaseCost * FMath::Pow(ReviveCostGrowth, static_cast<float>(ReviveCount));
	return FMath::Min(FMath::RoundToInt(Cost), ReviveCostMax);
}

AUSAllyDroneCharacter* UUSSquadComponent::SpawnAllyInSlot(const UUSAllyTypeData* AllyType, int32 SlotIndex)
{
	if (AllyType == nullptr || AllyType->GetDroneClass() == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSSquadComponent: ally type %s has no DroneClass"), *GetNameSafe(AllyType));
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (World == nullptr || Leader == nullptr || !Members.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}

	// 슬롯 위치에 바로 스폰. 캡슐 높이만큼 띄워 바닥에 끼지 않게 한다.
	const FVector SpawnLocation = GetSlotWorldLocation(SlotIndex) + FVector(0.0f, 0.0f, 50.0f);
	const FRotator SpawnRotation(0.0f, FormationYaw, 0.0f);

	// 종류·분대 정보를 BeginPlay 전에 넣어야 부하가 BeginPlay에서 스탯을 적용할 수 있다 → 지연 스폰
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);
	AUSAllyDroneCharacter* Ally = World->SpawnActorDeferred<AUSAllyDroneCharacter>(AllyType->GetDroneClass(), SpawnTransform,
		Leader, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (Ally == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSSquadComponent: failed to spawn ally %s"), *GetNameSafe(AllyType));
		return nullptr;
	}

	Ally->InitSquadMember(Leader, this, SlotIndex);
	Ally->SetAllyType(AllyType);
	Ally->FinishSpawning(SpawnTransform);

	// 종류 시작 스탯은 부하 BeginPlay(FinishSpawning 안)에서 적용되므로, 그 위에 이 종류·편대 전체가 받은 강화를 쌓는다
	if (UUSAttributeComponent* Attribute = Ally->GetAttributeComponent())
	{
		for (const FUSTypeUpgrade& Upgrade : TypeUpgrades)
		{
			if (Upgrade.Type == nullptr || Upgrade.Type == AllyType)
			{
				Attribute->ApplyEffect(Upgrade.Effect, Upgrade.Value);
			}
		}
	}

	// 이 기종이 배운 능력도 현재 단계로 적용한다 (무기는 부하 BeginPlay에서 장착되어 이미 있다)
	for (const TPair<TObjectPtr<const UUSAbilityData>, int32>& Pair : AbilityLevels)
	{
		if (Pair.Key && Pair.Key->GetOwnerType() == AllyType)
		{
			Pair.Key->ApplyTo(Ally, Pair.Value);
		}
	}

	if (SlowDurationBonus > 0.0f)
	{
		if (UUSEmpPulseComponent* Pulse = Ally->FindComponentByClass<UUSEmpPulseComponent>())
		{
			Pulse->SetSlowDurationBonus(SlowDurationBonus);
		}
	}

	Members[SlotIndex].Ally = Ally;
	Members[SlotIndex].Type = AllyType;
	Members[SlotIndex].bDown = false;
	return Ally;
}

FVector UUSSquadComponent::GetSlotWorldLocation(int32 SlotIndex) const
{
	if (Leader == nullptr)
	{
		return FVector::ZeroVector;
	}

	const FVector LeaderLocation = Leader->GetActorLocation();
	if (SlotIndex < 0)
	{
		return LeaderLocation;
	}

	FVector GuardLocation;
	if (TryGetGuardLocation(SlotIndex, GuardLocation))
	{
		return GuardLocation;
	}

	// 오프셋이 부족한 슬롯은 대장 바로 뒤로
	const FVector2D Offset = Slots.IsValidIndex(SlotIndex) ? Slots[SlotIndex].Offset : FVector2D(-150.0f, 0.0f);
	const FRotator FormationRotation(0.0f, FormationYaw, 0.0f);
	const FVector WorldOffset = FormationRotation.RotateVector(FVector(Offset.X, Offset.Y, 0.0f));

	return LeaderLocation + WorldOffset;
}

int32 UUSSquadComponent::GetMemberCount() const
{
	int32 Count = 0;
	for (const FUSSquadMember& Member : Members)
	{
		if (Member.IsActive())
		{
			++Count;
		}
	}
	return Count;
}

int32 UUSSquadComponent::FindHireSlot(EUSAllyRole Role) const
{
	// 우선순위: 0 = 역할 빈자리, 1 = 아무 빈자리, 2 = 역할 격추 자리, 3 = 아무 격추 자리
	int32 BestIndex = INDEX_NONE;
	int32 BestRank = MAX_int32;

	for (int32 Index = 0; Index < Members.Num(); ++Index)
	{
		const FUSSquadMember& Member = Members[Index];
		if (Member.IsActive())
		{
			continue;
		}

		const bool bRoleMatch = Slots.IsValidIndex(Index) && Slots[Index].Role == Role;
		const int32 Rank = (Member.IsEmpty() ? 0 : 2) + (bRoleMatch ? 0 : 1);
		if (Rank < BestRank)
		{
			BestRank = Rank;
			BestIndex = Index;
		}
	}

	return BestIndex;
}

bool UUSSquadComponent::TrySpendGold(int32 Cost) const
{
	return LeaderCurrency != nullptr && LeaderCurrency->TrySpend(Cost);
}

void UUSSquadComponent::PlayFeedbackSound(USoundBase* Sound) const
{
	if (Sound)
	{
		// 부활은 보급선 상점(일시정지 중)에서 일어난다. UI 소리로 틀어야 일시정지에 묶이지 않고 바로 들린다
		UGameplayStatics::PlaySound2D(this, Sound, 1.0f, 1.0f, 0.0f, nullptr, nullptr, true);
	}
}

float UUSSquadComponent::GetFormationTargetYaw() const
{
	if (Leader == nullptr)
	{
		return FormationYaw;
	}

	// 멈춰 있거나 관성으로 미끄러지는 중이면 진형을 그대로 둔다 (속도 방향이 흔들려 진형이 떨지 않게)
	const FVector Velocity = Leader->GetVelocity();
	if (Velocity.SizeSquared2D() < FMath::Square(FormationMinMoveSpeed))
	{
		return FormationYaw;
	}

	return Velocity.Rotation().Yaw;
}

void UUSSquadComponent::UpdateGuardThreat()
{
	const UWorld* World = GetWorld();
	const UUSTeamSubsystem* Team = World ? World->GetSubsystem<UUSTeamSubsystem>() : nullptr;
	if (Leader == nullptr || Team == nullptr || GuardSearchRadius <= 0.0f)
	{
		bHasGuardThreat = false;
		return;
	}

	// 가까운 적일수록 무겁게(1 → 반경 끝 0) 방향을 더한다. 무게중심 대신 방향 합이라 한쪽에 몰린 무리 쪽을 가리킨다
	const FVector LeaderLocation = Leader->GetActorLocation();
	const float RadiusSq = FMath::Square(GuardSearchRadius);
	FVector2D ThreatSum = FVector2D::ZeroVector;
	for (const AActor* Actor : Team->GetEnemies())
	{
		const AUSEnemyCharacter* Enemy = Cast<AUSEnemyCharacter>(Actor);
		if (Enemy == nullptr || !Enemy->IsAlive())
		{
			continue;
		}

		const FVector2D ToEnemy(Enemy->GetActorLocation() - LeaderLocation);
		const float DistSq = ToEnemy.SizeSquared();
		if (DistSq > RadiusSq || DistSq < KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const float Dist = FMath::Sqrt(DistSq);
		ThreatSum += ToEnemy / Dist * (1.0f - Dist / GuardSearchRadius);
	}

	// 사방에서 고르게 오면 합이 거의 0 → 방향을 바꾸지 않는다 (탱커가 떨지 않게)
	bHasGuardThreat = !ThreatSum.IsNearlyZero(0.05f);
	if (bHasGuardThreat)
	{
		GuardTargetYaw = FMath::RadiansToDegrees(FMath::Atan2(ThreatSum.Y, ThreatSum.X));
	}
}

bool UUSSquadComponent::TryGetGuardLocation(int32 SlotIndex, FVector& OutLocation) const
{
	if (!bHasGuardThreat || !Members.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FUSSquadMember& Member = Members[SlotIndex];
	if (!Member.IsActive() || Member.Type->GetRole() != EUSAllyRole::Tank)
	{
		return false;
	}

	// 살아 있는 탱커 중 몇 번째인지 → 위협 방향 좌우로 고르게 벌린다 (1기면 정면)
	int32 TankCount = 0;
	int32 TankOrder = 0;
	for (int32 Index = 0; Index < Members.Num(); ++Index)
	{
		if (Members[Index].IsActive() && Members[Index].Type->GetRole() == EUSAllyRole::Tank)
		{
			if (Index < SlotIndex)
			{
				++TankOrder;
			}
			++TankCount;
		}
	}

	const float Lateral = (TankOrder - (TankCount - 1) * 0.5f) * GuardSpacing;
	const FVector WorldOffset = FRotator(0.0f, GuardYaw, 0.0f).RotateVector(FVector(GuardDistance, Lateral, 0.0f));
	OutLocation = Leader->GetActorLocation() + WorldOffset;
	return true;
}
