#include "Agents/USHangarPlayerController.h"
#include "Core/USLevelTravelSubsystem.h"
#include "US.h"
#include "Agents/USAllyDroneCharacter.h"
#include "Agents/USDroneCharacter.h"
#include "Datas/USAllyTypeData.h"
#include "Hangar/USHangarModelStage.h"
#include "Progression/USMetaProgressionSubsystem.h"
#include "Datas/USZoneData.h"
#include "UI/USHangarWidget.h"
#include "Kismet/GameplayStatics.h"

AUSHangarPlayerController::AUSHangarPlayerController()
{
	bShowMouseCursor = true;
}

void AUSHangarPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (MetaTree == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] MetaTree is not set. Hangar panels and loadout will be empty."), *GetName());
	}
	if (CraftTypes.Num() == 0)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] CraftTypes is empty. Only the leader craft can be selected."), *GetName());
	}

	ModelStage = Cast<AUSHangarModelStage>(UGameplayStatics::GetActorOfClass(this, AUSHangarModelStage::StaticClass()));
	if (ModelStage == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] No AUSHangarModelStage in the map. The 3D craft view will be empty."), *GetName());
	}

	// 해금하면 무대의 홀로그램도 바로 실물로 바뀌어야 한다
	if (UUSMetaProgressionSubsystem* Meta = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr)
	{
		MetaChangedHandle = Meta->OnMetaChanged.AddUObject(this, &AUSHangarPlayerController::HandleMetaChanged);
	}

	if (HangarWidgetClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] HangarWidgetClass is not set. Hangar screen will be empty."), *GetName());
		return;
	}

	HangarWidget = CreateWidget<UUSHangarWidget>(this, HangarWidgetClass);
	if (HangarWidget == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] Failed to create hangar widget (%s)."), *GetName(), *GetNameSafe(HangarWidgetClass));
		return;
	}

	HangarWidget->OnLaunchRequested.BindUObject(this, &AUSHangarPlayerController::HandleLaunchRequested);
	HangarWidget->OnTitleRequested.BindUObject(this, &AUSHangarPlayerController::HandleTitleRequested);
	HangarWidget->OnCraftSelectRequested.BindUObject(this, &AUSHangarPlayerController::HandleCraftSelectRequested);
	HangarWidget->AddToViewport();
	HangarWidget->SetupHangar(MetaTree, Zones);

	TArray<const UUSAllyTypeData*> Types;
	Types.Reserve(GetCraftCount());
	for (int32 Index = 0; Index < GetCraftCount(); ++Index)
	{
		Types.Add(GetCraftType(Index));
	}
	HangarWidget->SetupCraftList(Types);

	// 3D 클릭이 없어져 월드 입력이 필요 없다 → UI 전용 (예전엔 진열대 클릭 때문에 게임+UI였다)
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(HangarWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);

	// 카메라를 기다릴 일이 없어 다음 틱으로 미루지 않고 바로 대장기를 고른다
	SelectCraft(0);

	// 스테이지를 깨고 돌아왔으면 격납고(강화)는 그대로 보이고, "출격"이 그 행성 지역 지도로 바로 간다
	if (UUSMetaProgressionSubsystem* Meta = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr)
	{
		if (Meta->ConsumeReturnToStageMap())
		{
			HangarWidget->SetResumeZone(Meta->GetSortieZone());
		}
	}
}

void AUSHangarPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UUSMetaProgressionSubsystem* Meta = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr)
	{
		Meta->OnMetaChanged.Remove(MetaChangedHandle);
	}

	Super::EndPlay(EndPlayReason);
}

int32 AUSHangarPlayerController::GetCraftCount() const
{
	return CraftTypes.Num() + 1;
}

const UUSAllyTypeData* AUSHangarPlayerController::GetCraftType(int32 CraftIndex) const
{
	return CraftIndex > 0 && CraftTypes.IsValidIndex(CraftIndex - 1) ? CraftTypes[CraftIndex - 1].Get() : nullptr;
}

void AUSHangarPlayerController::HandleCraftSelectRequested(int32 CraftIndex)
{
	if (CraftIndex != SelectedCraft)
	{
		SelectCraft(CraftIndex);
	}
}

void AUSHangarPlayerController::SelectCraft(int32 CraftIndex)
{
	if (CraftIndex < 0 || CraftIndex >= GetCraftCount() || HangarWidget == nullptr)
	{
		return;
	}

	SelectedCraft = CraftIndex;
	HangarWidget->SetSelectedCraft(CraftIndex);
	HangarWidget->ShowCraft(GetCraftType(CraftIndex));
	RefreshModelStage();
}

void AUSHangarPlayerController::RefreshModelStage()
{
	if (ModelStage == nullptr || SelectedCraft == INDEX_NONE)
	{
		return;
	}

	const UUSAllyTypeData* Type = GetCraftType(SelectedCraft);
	const TSubclassOf<AUSDroneCharacter> CraftClass = Type ? TSubclassOf<AUSDroneCharacter>(Type->GetDroneClass()) : LeaderCraftClass;

	// 대장기는 늘 열려 있다. 부하는 메타 진행으로 해금 여부를 본다 (설계도 위젯의 흐린 도면과 같은 기준)
	const UUSMetaProgressionSubsystem* Meta = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr;
	const bool bLocked = Type && Meta && !Meta->IsAllyTypeUnlocked(MetaTree, Type);

	ModelStage->ShowCraft(CraftClass, bLocked);
}

void AUSHangarPlayerController::HandleMetaChanged()
{
	RefreshModelStage();
}

void AUSHangarPlayerController::HandleLaunchRequested(const UUSZoneData* Zone)
{
	if (Zone == nullptr || Zone->GetMap().IsNull())
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] Zone %s has no map. Cannot launch."), *GetName(), *GetNameSafe(Zone));
		return;
	}

	UUSLevelTravelSubsystem::TravelFrom(this, Zone->GetMap());
}

void AUSHangarPlayerController::HandleTitleRequested()
{
	if (TitleMap.IsNull())
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] TitleMap is not set. Cannot go to the title."), *GetName());
		return;
	}

	UUSLevelTravelSubsystem::TravelFrom(this, TitleMap);
}
