#include "Progression/USMetaProgressionSubsystem.h"
#include "US.h"
#include "Agents/USPlayerCharacter.h"
#include "Components/USAttributeComponent.h"
#include "Components/USCurrencyComponent.h"
#include "Components/USExperienceComponent.h"
#include "Components/USSquadComponent.h"
#include "Datas/USAllyTypeData.h"
#include "Datas/USMetaTreeData.h"
#include "Datas/USMetaUpgradeData.h"
#include "Datas/USScrapRewardData.h"
#include "Datas/USStageData.h"
#include "Datas/USZoneData.h"
#include "Progression/USMetaSaveGame.h"
#include "Progression/USProgressionTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectIterator.h"

const FString UUSMetaProgressionSubsystem::SlotName = TEXT("MetaProgress");

namespace
{
	/** 저장 슬롯의 사용자 번호. 로컬 1인 게임이라 항상 0 */
	constexpr int32 SaveUserIndex = 0;
}

void UUSMetaProgressionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex))
	{
		SaveData = Cast<UUSMetaSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, SaveUserIndex));
		if (SaveData == nullptr)
		{
			// 파일은 있는데 읽지 못함 (깨졌거나 다른 클래스). 다음 저장이 덮어쓰기 전에 원본 바이트를 따로 남겨 둔다 (R-29)
			BackupUnreadableSave();
		}
	}

	if (SaveData == nullptr)
	{
		SaveData = Cast<UUSMetaSaveGame>(UGameplayStatics::CreateSaveGameObject(UUSMetaSaveGame::StaticClass()));
	}
	else if (SaveData->Version < UUSMetaSaveGame::CurrentVersion)
	{
		// 버전 1 → 2: 출격 편성(Loadout)만 새로 생겼고 비어 있으면 기본 기종으로 채우므로 변환할 값이 없다
		// 버전 2 → 3: 비운 칸 목록(EmptyLoadoutSlots)만 새로 생겼고 비어 있으면 "비운 칸 없음"이라 역시 변환할 값이 없다
		// 버전 3 → 4: 깬 스테이지 목록(ClearedStages)만 새로 생겼고 비어 있으면 첫 스테이지부터라 변환할 값이 없다
		UE_LOG(LogUS, Log, TEXT("UUSMetaProgressionSubsystem: save version %d -> %d"), SaveData->Version, UUSMetaSaveGame::CurrentVersion);
		SaveData->Version = UUSMetaSaveGame::CurrentVersion;
	}

	UE_LOG(LogUS, Log, TEXT("UUSMetaProgressionSubsystem: loaded (scrap %d, %d nodes, %d runs)"),
		GetScrap(), SaveData ? SaveData->NodeLevels.Num() : 0, SaveData ? SaveData->RunCount : 0);
}

void UUSMetaProgressionSubsystem::BackupUnreadableSave() const
{
	TArray<uint8> RawBytes;
	if (!UGameplayStatics::LoadDataFromSlot(RawBytes, SlotName, SaveUserIndex) || RawBytes.IsEmpty())
	{
		UE_LOG(LogUS, Warning, TEXT("UUSMetaProgressionSubsystem: failed to load save slot '%s' and could not read it for a backup. Starting a new save."), *SlotName);
		return;
	}

	// 깨진 파일이 여러 번 생겨도 서로 덮어쓰지 않게 시각을 붙인다
	const FString BackupSlot = FString::Printf(TEXT("%s_Broken_%s"), *SlotName, *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	const bool bBackedUp = UGameplayStatics::SaveDataToSlot(RawBytes, BackupSlot, SaveUserIndex);
	UE_LOG(LogUS, Warning, TEXT("UUSMetaProgressionSubsystem: failed to load save slot '%s'. Backup %s ('%s', %d bytes). Starting a new save."),
		*SlotName, bBackedUp ? TEXT("written") : TEXT("FAILED"), *BackupSlot, RawBytes.Num());
}

int32 UUSMetaProgressionSubsystem::GetScrap() const
{
	return SaveData ? SaveData->Scrap : 0;
}

int32 UUSMetaProgressionSubsystem::GetRunCount() const
{
	return SaveData ? SaveData->RunCount : 0;
}

int32 UUSMetaProgressionSubsystem::GetClearCount() const
{
	return SaveData ? SaveData->ClearCount : 0;
}

float UUSMetaProgressionSubsystem::GetBestSurvivedSeconds() const
{
	return SaveData ? SaveData->BestSurvivedSeconds : 0.0f;
}

int32 UUSMetaProgressionSubsystem::GetBestKillCount() const
{
	return SaveData ? SaveData->BestKillCount : 0;
}

int32 UUSMetaProgressionSubsystem::GetTotalLevel(const UUSMetaTreeData* Tree, EUSMetaEffect Effect) const
{
	int32 Total = 0;
	if (Tree == nullptr)
	{
		return Total;
	}

	for (const UUSMetaUpgradeData* Node : Tree->GetNodes())
	{
		if (Node && Node->GetEffect() == Effect)
		{
			Total += GetLevel(Node);
		}
	}
	return Total;
}

int32 UUSMetaProgressionSubsystem::GetLevel(const UUSMetaUpgradeData* Node) const
{
	if (Node == nullptr || SaveData == nullptr)
	{
		return 0;
	}

	const int32* Level = SaveData->NodeLevels.Find(Node->GetNodeId());
	return Level ? *Level : 0;
}

bool UUSMetaProgressionSubsystem::IsNodeOpen(const UUSMetaUpgradeData* Node) const
{
	if (Node == nullptr)
	{
		return false;
	}

	const UUSMetaUpgradeData* Prerequisite = Node->GetPrerequisite();
	return Prerequisite == nullptr || GetLevel(Prerequisite) > 0;
}

bool UUSMetaProgressionSubsystem::IsNodeMaxed(const UUSMetaUpgradeData* Node) const
{
	return Node && GetLevel(Node) >= Node->GetMaxLevel();
}

bool UUSMetaProgressionSubsystem::IsAllyTypeUnlocked(const UUSMetaTreeData* Tree, const UUSAllyTypeData* AllyType) const
{
	if (Tree == nullptr || AllyType == nullptr)
	{
		return AllyType != nullptr;
	}

	for (const UUSMetaUpgradeData* Node : Tree->GetNodes())
	{
		if (Node && Node->GetEffect() == EUSMetaEffect::UnlockAllyType && Node->GetAllyType() == AllyType)
		{
			return GetLevel(Node) > 0;
		}
	}
	return true;
}

int32 UUSMetaProgressionSubsystem::GetLoadoutSlotCount(const UUSMetaTreeData* Tree) const
{
	return Tree ? Tree->GetBaseLoadoutSlots() + GetTotalLevel(Tree, EUSMetaEffect::StartingAlly) : 0;
}

TArray<const UUSAllyTypeData*> UUSMetaProgressionSubsystem::GetLoadoutSlots(const UUSMetaTreeData* Tree) const
{
	TArray<const UUSAllyTypeData*> Slots;
	const int32 SlotCount = GetLoadoutSlotCount(Tree);
	if (Tree == nullptr || SlotCount <= 0)
	{
		return Slots;
	}

	Slots.Reserve(SlotCount);
	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		if (SaveData && SaveData->EmptyLoadoutSlots.Contains(Index))
		{
			Slots.Add(nullptr);
			continue;
		}

		// 부하 종류 애셋은 트리·분대가 이미 로드해 두어 TryLoad가 디스크를 읽지 않는다 (못 찾으면 기본 기종)
		const UUSAllyTypeData* Type = nullptr;
		if (SaveData && SaveData->Loadout.IsValidIndex(Index))
		{
			Type = Cast<UUSAllyTypeData>(SaveData->Loadout[Index].TryLoad());
		}
		if (Type == nullptr || !IsAllyTypeUnlocked(Tree, Type))
		{
			Type = Tree->GetDefaultAllyType();
		}
		Slots.Add(Type);
	}
	return Slots;
}

TArray<const UUSAllyTypeData*> UUSMetaProgressionSubsystem::GetLoadout(const UUSMetaTreeData* Tree) const
{
	TArray<const UUSAllyTypeData*> Loadout = GetLoadoutSlots(Tree);
	Loadout.RemoveAll([](const UUSAllyTypeData* Type) { return Type == nullptr; });
	return Loadout;
}

bool UUSMetaProgressionSubsystem::SetLoadoutSlot(const UUSMetaTreeData* Tree, int32 SlotIndex, const UUSAllyTypeData* AllyType)
{
	if (SaveData == nullptr || SlotIndex < 0 || SlotIndex >= GetLoadoutSlotCount(Tree))
	{
		return false;
	}

	if (AllyType == nullptr)
	{
		SaveData->EmptyLoadoutSlots.Add(SlotIndex);
		SaveAndNotify();
		return true;
	}

	if (!IsAllyTypeUnlocked(Tree, AllyType))
	{
		return false;
	}

	if (SaveData->Loadout.Num() <= SlotIndex)
	{
		SaveData->Loadout.SetNum(SlotIndex + 1);
	}
	SaveData->Loadout[SlotIndex] = FSoftObjectPath(AllyType);
	SaveData->EmptyLoadoutSlots.Remove(SlotIndex);
	SaveAndNotify();
	return true;
}

int32 UUSMetaProgressionSubsystem::GetNextCost(const UUSMetaUpgradeData* Node) const
{
	return Node ? Node->GetCostAtLevel(GetLevel(Node)) : 0;
}

bool UUSMetaProgressionSubsystem::TryBuy(const UUSMetaUpgradeData* Node)
{
	if (Node == nullptr || SaveData == nullptr)
	{
		return false;
	}

	// 저장 키가 비면 사도 저장되지 않으므로 애셋 설정 실수를 바로 알린다
	if (Node->GetNodeId().IsNone())
	{
		UE_LOG(LogUS, Warning, TEXT("UUSMetaProgressionSubsystem: %s has no NodeId; cannot buy"), *GetNameSafe(Node));
		return false;
	}

	if (!IsNodeOpen(Node) || IsNodeMaxed(Node))
	{
		return false;
	}

	const int32 Cost = GetNextCost(Node);
	if (SaveData->Scrap < Cost)
	{
		return false;
	}

	SaveData->Scrap -= Cost;
	SaveData->NodeLevels.FindOrAdd(Node->GetNodeId()) += 1;
	SaveAndNotify();
	return true;
}

float UUSMetaProgressionSubsystem::GetMultiplier(const UUSMetaTreeData* Tree, EUSMetaEffect Effect) const
{
	float Multiplier = 1.0f;
	if (Tree == nullptr)
	{
		return Multiplier;
	}

	for (const UUSMetaUpgradeData* Node : Tree->GetNodes())
	{
		if (Node && Node->GetEffect() == Effect)
		{
			Multiplier *= FMath::Pow(1.0f + Node->GetValuePerLevel(), static_cast<float>(GetLevel(Node)));
		}
	}
	return Multiplier;
}

float UUSMetaProgressionSubsystem::GetAdditiveTotal(const UUSMetaTreeData* Tree, EUSMetaEffect Effect) const
{
	float Total = 0.0f;
	if (Tree == nullptr)
	{
		return Total;
	}

	for (const UUSMetaUpgradeData* Node : Tree->GetNodes())
	{
		if (Node && Node->GetEffect() == Effect)
		{
			Total += Node->GetValuePerLevel() * GetLevel(Node);
		}
	}
	return Total;
}

void UUSMetaProgressionSubsystem::ApplyToRun(const UUSMetaTreeData* Tree, APawn* Leader) const
{
	const AUSPlayerCharacter* Player = Cast<AUSPlayerCharacter>(Leader);
	if (Tree == nullptr || Player == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSMetaProgressionSubsystem: ApplyToRun skipped (tree %s, leader %s)"),
			*GetNameSafe(Tree), *GetNameSafe(Leader));
		return;
	}

	UUSAttributeComponent* Attribute = Player->GetAttributeComponent();
	UUSSquadComponent* Squad = Player->GetSquadComponent();
	TArray<const UUSAllyTypeData*> LockedTypes;

	for (const UUSMetaUpgradeData* Node : Tree->GetNodes())
	{
		if (Node == nullptr)
		{
			continue;
		}

		const int32 Level = GetLevel(Node);
		const float Value = Node->GetValuePerLevel();

		// 스탯 칸은 단계 수만큼 한 단계씩 곱한다. 레벨업 카드·상점 강화와 같은 ApplyEffect 경로라 쌓이는 방식이 같다
		switch (Node->GetEffect())
		{
		case EUSMetaEffect::LeaderDamage:
		case EUSMetaEffect::LeaderHealth:
			if (Attribute)
			{
				const EUSEffectType Effect = Node->GetEffect() == EUSMetaEffect::LeaderDamage ? EUSEffectType::AttackDamage : EUSEffectType::MaxHealth;
				for (int32 Step = 0; Step < Level; ++Step)
				{
					Attribute->ApplyEffect(Effect, Value);
				}
			}
			break;

		case EUSMetaEffect::LeaderMoveSpeed:
			// 이속은 더해서 쌓인다 (UUSAttributeComponent::AddMoveSpeedMultiplier). 추진 불꽃은 대장이 이 배율로 키운다
			if (Attribute)
			{
				for (int32 Step = 0; Step < Level; ++Step)
				{
					Attribute->ApplyEffect(EUSEffectType::MoveSpeed, Value);
				}
			}
			break;

		case EUSMetaEffect::SquadDamage:
		case EUSMetaEffect::SquadHealth:
			if (Squad)
			{
				// 분대 기록에 남아 시작 부하·이후 합류·부활하는 부하 모두에게 적용된다
				const EUSEffectType Effect = Node->GetEffect() == EUSMetaEffect::SquadDamage ? EUSEffectType::AttackDamage : EUSEffectType::MaxHealth;
				for (int32 Step = 0; Step < Level; ++Step)
				{
					Squad->ApplySquadUpgrade(Effect, Value);
				}
			}
			break;

		case EUSMetaEffect::UnlockAllyType:
			if (Level == 0 && Node->GetAllyType())
			{
				LockedTypes.Add(Node->GetAllyType());
			}
			break;

		// 골드·경험치·스크랩은 칸 여러 개를 배율 하나로 모아 아래에서 한 번에 적용한다. 편성 칸은 GetLoadout이 센다.
		// 출격 시간·카드 다시 뽑기는 대장이 아니라 컨트롤러가 판 시작 때 GetAdditiveTotal로 읽는다
		default:
			break;
		}
	}

	if (UUSCurrencyComponent* Currency = Player->GetCurrencyComponent())
	{
		Currency->ScaleGoldGain(GetMultiplier(Tree, EUSMetaEffect::GoldGain));
	}

	if (UUSExperienceComponent* Experience = Player->GetExperienceComponent())
	{
		Experience->ScaleExpGain(GetMultiplier(Tree, EUSMetaEffect::ExpGain));
	}

	if (Squad)
	{
		Squad->SetLockedTypes(LockedTypes);
		Squad->SetStartingAllies(GetLoadout(Tree));
	}

	UE_LOG(LogUS, Log, TEXT("UUSMetaProgressionSubsystem: applied meta to %s (%d locked ally types)"), *GetNameSafe(Leader), LockedTypes.Num());
}

void UUSMetaProgressionSubsystem::RecordRun(const FUSRunResult& Result)
{
	if (SaveData == nullptr)
	{
		return;
	}

	SaveData->Scrap += FMath::Max(0, Result.TotalScrap);
	SaveData->RunCount += 1;
	if (Result.bCleared)
	{
		SaveData->ClearCount += 1;
	}
	SaveData->BestSurvivedSeconds = FMath::Max(SaveData->BestSurvivedSeconds, Result.SurvivedSeconds);
	SaveData->BestKillCount = FMath::Max(SaveData->BestKillCount, Result.KillCount);

	SaveAndNotify();
}

void UUSMetaProgressionSubsystem::AddScrap(int32 Amount)
{
	if (SaveData == nullptr)
	{
		return;
	}

	SaveData->Scrap = FMath::Max(0, SaveData->Scrap + Amount);
	SaveAndNotify();
}

void UUSMetaProgressionSubsystem::SetLevel(FName NodeId, int32 Level)
{
	if (SaveData == nullptr || NodeId.IsNone())
	{
		return;
	}

	if (Level <= 0)
	{
		SaveData->NodeLevels.Remove(NodeId);
	}
	else
	{
		SaveData->NodeLevels.Add(NodeId, Level);
	}
	SaveAndNotify();
}

void UUSMetaProgressionSubsystem::ResetProgress()
{
	SaveData = Cast<UUSMetaSaveGame>(UGameplayStatics::CreateSaveGameObject(UUSMetaSaveGame::StaticClass()));
	SaveAndNotify();
}

void UUSMetaProgressionSubsystem::LogStatus() const
{
	if (SaveData == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSMetaProgressionSubsystem: no save data"));
		return;
	}

	UE_LOG(LogUS, Display, TEXT("[Meta] scrap %d | runs %d, clears %d | best %.0fs, %d kills"),
		SaveData->Scrap, SaveData->RunCount, SaveData->ClearCount, SaveData->BestSurvivedSeconds, SaveData->BestKillCount);
	for (const FName& StageId : SaveData->ClearedStages)
	{
		UE_LOG(LogUS, Display, TEXT("[Meta]   stage %s cleared"), *StageId.ToString());
	}
	for (const TPair<FName, int32>& Pair : SaveData->NodeLevels)
	{
		UE_LOG(LogUS, Display, TEXT("[Meta]   %s = %d"), *Pair.Key.ToString(), Pair.Value);
	}
}

bool UUSMetaProgressionSubsystem::IsStageCleared(const UUSStageData* Stage) const
{
	return SaveData && Stage && SaveData->ClearedStages.Contains(Stage->GetStageId());
}

bool UUSMetaProgressionSubsystem::IsStageOpen(const UUSZoneData* Zone, const UUSStageData* Stage) const
{
	if (Zone == nullptr || Stage == nullptr)
	{
		return false;
	}
	if (IsStageCleared(Stage))
	{
		return true;
	}

	const TArray<TObjectPtr<UUSStageData>>& Stages = Zone->GetStages();
	if (Stage->IsBossStage())
	{
		// 보스는 마지막: 나머지 지역을 다 깨야 열린다
		for (const UUSStageData* Other : Stages)
		{
			if (Other && Other != Stage && !Other->IsBossStage() && !IsStageCleared(Other))
			{
				return false;
			}
		}
		return true;
	}

	// 처음 들어오면 1번(가장 작은 번호)만 열려 있다
	const UUSStageData* First = nullptr;
	for (const UUSStageData* Other : Stages)
	{
		if (Other && (First == nullptr || Other->GetStageNumber() < First->GetStageNumber()))
		{
			First = Other;
		}
	}
	if (Stage == First)
	{
		return true;
	}

	// 맞닿은 지역 중 깬 곳이 있으면 열린다. 데이터는 한쪽에만 적어도 되게 양쪽을 다 본다
	for (const UUSStageData* Neighbor : Stage->GetNeighbors())
	{
		if (IsStageCleared(Neighbor))
		{
			return true;
		}
	}
	for (const UUSStageData* Other : Stages)
	{
		if (Other && IsStageCleared(Other) && Other->GetNeighbors().Contains(Stage))
		{
			return true;
		}
	}
	return false;
}

const UUSStageData* UUSMetaProgressionSubsystem::FindNextStage(const UUSZoneData* Zone, const UUSStageData* Current) const
{
	if (Zone == nullptr)
	{
		return nullptr;
	}

	const UUSStageData* Best = nullptr;
	for (const UUSStageData* Stage : Zone->GetStages())
	{
		if (Stage == nullptr || Stage == Current || IsStageCleared(Stage) || !IsStageOpen(Zone, Stage))
		{
			continue;
		}
		if (Best == nullptr || Stage->GetStageNumber() < Best->GetStageNumber())
		{
			Best = Stage;
		}
	}
	return Best;
}

int32 UUSMetaProgressionSubsystem::GetClearedStageCount(const UUSZoneData* Zone) const
{
	int32 Count = 0;
	if (Zone)
	{
		for (const UUSStageData* Stage : Zone->GetStages())
		{
			Count += IsStageCleared(Stage) ? 1 : 0;
		}
	}
	return Count;
}

void UUSMetaProgressionSubsystem::MarkStageCleared(const UUSStageData* Stage)
{
	if (SaveData == nullptr || Stage == nullptr)
	{
		return;
	}
	if (Stage->GetStageId().IsNone())
	{
		UE_LOG(LogUS, Warning, TEXT("UUSMetaProgressionSubsystem: %s has no StageId; clear is not saved"), *GetNameSafe(Stage));
		return;
	}

	bool bAlreadyCleared = false;
	SaveData->ClearedStages.Add(Stage->GetStageId(), &bAlreadyCleared);
	if (!bAlreadyCleared)
	{
		SaveAndNotify();
	}
}

void UUSMetaProgressionSubsystem::SetSortie(const UUSZoneData* Zone, const UUSStageData* Stage)
{
	SortieZone = Zone;
	SortieStage = Stage;
}

bool UUSMetaProgressionSubsystem::ConsumeReturnToStageMap()
{
	const bool bReturn = bReturnToStageMap;
	bReturnToStageMap = false;
	return bReturn;
}

void UUSMetaProgressionSubsystem::SaveAndNotify()
{
	if (SaveData && !UGameplayStatics::SaveGameToSlot(SaveData, SlotName, SaveUserIndex))
	{
		UE_LOG(LogUS, Warning, TEXT("UUSMetaProgressionSubsystem: failed to write save slot '%s'"), *SlotName);
	}

	OnMetaChanged.Broadcast();
}

// ---------------- 시험용 콘솔 명령 (D5-b 트리 UI 전, D6 밸런스용) ----------------

#if !UE_BUILD_SHIPPING
namespace
{
	UUSMetaProgressionSubsystem* FindMetaSubsystem(const UWorld* World)
	{
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		UUSMetaProgressionSubsystem* Meta = GameInstance ? GameInstance->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr;
		if (Meta == nullptr)
		{
			UE_LOG(LogUS, Warning, TEXT("[Meta] no meta subsystem in this world (run the command during PIE)"));
		}
		return Meta;
	}

	/** NodeId로 칸 애셋을 찾는다. 트리를 들고 있는 컨트롤러가 칸을 로드해 둔 상태(게임 맵 PIE)에서만 찾힌다 */
	const UUSMetaUpgradeData* FindNodeById(FName NodeId)
	{
		for (TObjectIterator<UUSMetaUpgradeData> It; It; ++It)
		{
			if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->GetNodeId() == NodeId)
			{
				return *It;
			}
		}
		return nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs MetaStatusCommand(
		TEXT("US.Meta.Status"),
		TEXT("Print scrap, node levels and records."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (const UUSMetaProgressionSubsystem* Meta = FindMetaSubsystem(World))
			{
				Meta->LogStatus();
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs MetaAddScrapCommand(
		TEXT("US.Meta.AddScrap"),
		TEXT("US.Meta.AddScrap <Amount>: add (or remove) scrap."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UUSMetaProgressionSubsystem* Meta = FindMetaSubsystem(World);
			if (Meta && Args.Num() > 0)
			{
				Meta->AddScrap(FCString::Atoi(*Args[0]));
				Meta->LogStatus();
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs MetaBuyCommand(
		TEXT("US.Meta.Buy"),
		TEXT("US.Meta.Buy <NodeId>: buy one level with scrap (same rules as the tree)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UUSMetaProgressionSubsystem* Meta = FindMetaSubsystem(World);
			if (Meta == nullptr || Args.Num() == 0)
			{
				return;
			}

			const UUSMetaUpgradeData* Node = FindNodeById(FName(*Args[0]));
			if (Node == nullptr)
			{
				UE_LOG(LogUS, Warning, TEXT("[Meta] node '%s' not found (is the tree loaded?)"), *Args[0]);
				return;
			}

			const bool bBought = Meta->TryBuy(Node);
			UE_LOG(LogUS, Display, TEXT("[Meta] buy %s: %s (open %d, level %d/%d, next cost %d)"), *Args[0],
				bBought ? TEXT("OK") : TEXT("FAILED"), Meta->IsNodeOpen(Node), Meta->GetLevel(Node), Node->GetMaxLevel(), Meta->GetNextCost(Node));
		}));

	FAutoConsoleCommandWithWorldAndArgs MetaSetLevelCommand(
		TEXT("US.Meta.SetLevel"),
		TEXT("US.Meta.SetLevel <NodeId> <Level>: set a node level for free."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UUSMetaProgressionSubsystem* Meta = FindMetaSubsystem(World);
			if (Meta && Args.Num() > 1)
			{
				Meta->SetLevel(FName(*Args[0]), FCString::Atoi(*Args[1]));
				Meta->LogStatus();
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs MetaResetCommand(
		TEXT("US.Meta.Reset"),
		TEXT("Reset all meta progression (scrap, nodes, records)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UUSMetaProgressionSubsystem* Meta = FindMetaSubsystem(World))
			{
				Meta->ResetProgress();
				Meta->LogStatus();
			}
		}));
}
#endif
