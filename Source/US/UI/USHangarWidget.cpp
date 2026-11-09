#include "UI/USHangarWidget.h"
#include "US.h"
#include "Datas/USAllyTypeData.h"
#include "Datas/USMetaTreeData.h"
#include "Datas/USMetaUpgradeData.h"
#include "Datas/USZoneData.h"
#include "Progression/USMetaProgressionSubsystem.h"
#include "UI/USHangarCraftPanelWidget.h"
#include "UI/USHangarMapWidget.h"
#include "UI/USHangarOptionWidget.h"
#include "UI/USHangarSquadWidget.h"
#include "UI/USStageMapWidget.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"

UUSHangarWidget::UUSHangarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LeaderTitle = NSLOCTEXT("USHangarWidget", "LeaderTitle", "내 전투기");
	LeaderDescription = NSLOCTEXT("USHangarWidget", "LeaderDescription", "아군을 이끄는 전투기");
	CraftUpgradeFormat = NSLOCTEXT("USHangarWidget", "CraftUpgradeFormat", "강화 {0}/{1}");
	LockedCraftSubFormat = NSLOCTEXT("USHangarWidget", "LockedCraftSubFormat", "잠김 · {0} 스크랩");
}

void UUSHangarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (MapButton)
	{
		MapButton->OnClicked.AddUniqueDynamic(this, &UUSHangarWidget::HandleMapClicked);
	}
	if (TitleButton)
	{
		TitleButton->OnClicked.AddUniqueDynamic(this, &UUSHangarWidget::HandleTitleClicked);
	}
	if (MapPanel)
	{
		MapPanel->OnSquadRequested.BindUObject(this, &UUSHangarWidget::HandleSquadRequested);
		MapPanel->OnBackRequested.BindUObject(this, &UUSHangarWidget::HandleMapBackRequested);
	}
	if (SquadPanel)
	{
		SquadPanel->OnLaunchRequested.BindUObject(this, &UUSHangarWidget::HandleSquadLaunchRequested);
		SquadPanel->OnBackRequested.BindUObject(this, &UUSHangarWidget::HandleSquadBackRequested);
	}
	if (StageMapPanel)
	{
		StageMapPanel->OnSquadRequested.BindUObject(this, &UUSHangarWidget::HandleStageSquadRequested);
		StageMapPanel->OnBackRequested.BindUObject(this, &UUSHangarWidget::HandleStageBackRequested);
	}
	// 패널은 늘 떠 있다. 처음 내용은 컨트롤러가 대장기를 골라 채운다
	ShowScreen(EUSHangarScreen::Hangar);

	if (UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem())
	{
		MetaChangedHandle = Meta->OnMetaChanged.AddUObject(this, &UUSHangarWidget::HandleMetaChanged);
	}
	HandleMetaChanged();
}

void UUSHangarWidget::NativeDestruct()
{
	if (UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem())
	{
		Meta->OnMetaChanged.Remove(MetaChangedHandle);
	}
	MetaChangedHandle.Reset();

	Super::NativeDestruct();
}

void UUSHangarWidget::SetupHangar(const UUSMetaTreeData* InTree, const TArray<TObjectPtr<UUSZoneData>>& InZones)
{
	Tree = InTree;

	if (CraftOptionClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSHangarWidget: CraftOptionClass is not set. No craft icons."));
	}
	if (LeaderIcon == nullptr || LockedCraftIcon == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSHangarWidget: LeaderIcon or LockedCraftIcon is not set. Those icons have no picture."));
	}

	if (MapPanel)
	{
		MapPanel->SetupMap(InZones);
	}
	if (SquadPanel)
	{
		SquadPanel->SetupSquad(Tree, LeaderIcon, LeaderTitle);
	}

	HandleMetaChanged();
}

void UUSHangarWidget::ShowCraft(const UUSAllyTypeData* OwnerType)
{
	if (CraftPanel)
	{
		// 부하 기종 설명은 설계도 목업에 없어 넘기지 않는다
		CraftPanel->ShowCraft(Tree, OwnerType, OwnerType ? OwnerType->GetDisplayName() : LeaderTitle, OwnerType ? FText::GetEmpty() : LeaderDescription);
		CraftPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UUSHangarWidget::SetupCraftList(const TArray<const UUSAllyTypeData*>& Types)
{
	if (CraftList == nullptr || CraftOptionClass == nullptr)
	{
		return;
	}

	CraftList->ClearChildren();
	CraftTypes.Reset(Types.Num());
	CraftOptions.Reset(Types.Num());
	CraftOptions.SetNum(Types.Num());

	for (const UUSAllyTypeData* Type : Types)
	{
		CraftTypes.Add(Type);
	}

	for (int32 Index = 0; Index < Types.Num(); ++Index)
	{
		UUSHangarOptionWidget* Option = CreateWidget<UUSHangarOptionWidget>(this, CraftOptionClass);
		if (Option == nullptr)
		{
			continue;
		}

		CraftList->AddChild(Option);
		Option->OnOptionClicked.BindUObject(this, &UUSHangarWidget::HandleCraftOptionClicked);
		CraftOptions[Index] = Option;
	}

	RefreshCraftList();
}

void UUSHangarWidget::SetSelectedCraft(int32 CraftIndex)
{
	SelectedCraft = CraftIndex;
	RefreshCraftList();
}

void UUSHangarWidget::HandleCraftOptionClicked(int32 CraftIndex)
{
	OnCraftSelectRequested.ExecuteIfBound(CraftIndex);
}

void UUSHangarWidget::RefreshCraftList()
{
	const UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem();

	for (int32 Index = 0; Index < CraftOptions.Num() && Index < CraftTypes.Num(); ++Index)
	{
		UUSHangarOptionWidget* Option = CraftOptions[Index];
		if (Option == nullptr)
		{
			continue;
		}

		const UUSAllyTypeData* Type = CraftTypes[Index];
		// 칸이 좁아서(112px) "기관총 전투기"가 한 줄에 안 들어간다. 자동 줄바꿈은 한글을 음절 사이 아무 데서나
		// 끊으므로 공백에서 직접 줄을 바꾼다
		const FText FullName = Type ? Type->GetDisplayName() : LeaderTitle;
		const FText Name = FText::FromString(FullName.ToString().Replace(TEXT(" "), TEXT("\n")));
		const bool bLocked = Meta && Type && !Meta->IsAllyTypeUnlocked(Tree, Type);

		// 잠긴 기종도 눌러서 고를 수 있어야 패널에서 해금한다. SetupOption의 bEnabled는 색만 바꾸고 클릭은 막지 않는다.
		// 고른 칸은 잠겨 있어도 강조색이 이기게 한다 (지금 무엇을 보고 있는지가 더 중요)
		const bool bSelected = Index == SelectedCraft;
		Option->SetupOption(Index, Name, GetCraftSubLabel(Type, bLocked), GetCraftIcon(Type, bLocked), !bLocked || bSelected, bSelected);
	}
}

FText UUSHangarWidget::GetCraftSubLabel(const UUSAllyTypeData* Type, bool bLocked) const
{
	const UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem();
	if (Meta == nullptr || Tree == nullptr)
	{
		return FText::GetEmpty();
	}

	int32 Level = 0;
	int32 MaxLevel = 0;
	for (const UUSMetaUpgradeData* Node : Tree->GetNodes())
	{
		if (Node == nullptr)
		{
			continue;
		}

		// 잠겼으면 그 기종을 여는 칸의 다음 가격을 보여 준다 (패널에서 바로 살 수 있는 값)
		if (bLocked)
		{
			if (Node->GetEffect() == EUSMetaEffect::UnlockAllyType && Node->GetAllyType() == Type)
			{
				return FText::Format(LockedCraftSubFormat, FText::AsNumber(Meta->GetNextCost(Node)));
			}
			continue;
		}

		if (Node->GetOwnerAllyType() == Type && Node->GetEffect() != EUSMetaEffect::UnlockAllyType)
		{
			Level += Meta->GetLevel(Node);
			MaxLevel += Node->GetMaxLevel();
		}
	}

	return bLocked || MaxLevel == 0 ? FText::GetEmpty()
		: FText::Format(CraftUpgradeFormat, FText::AsNumber(Level), FText::AsNumber(MaxLevel));
}

UTexture2D* UUSHangarWidget::GetCraftIcon(const UUSAllyTypeData* Type, bool bLocked) const
{
	if (bLocked && LockedCraftIcon)
	{
		return LockedCraftIcon;
	}
	return Type ? Type->GetIcon() : LeaderIcon.Get();
}

void UUSHangarWidget::HandleMetaChanged()
{
	const UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem();
	if (Meta == nullptr)
	{
		return;
	}

	if (ScrapText)
	{
		ScrapText->SetText(FText::AsNumber(Meta->GetScrap()));
	}
	RefreshCraftList();
}

void UUSHangarWidget::ShowScreen(EUSHangarScreen Screen)
{
	if (ScreenSwitcher)
	{
		ScreenSwitcher->SetActiveWidgetIndex(static_cast<int32>(Screen));
	}
}

void UUSHangarWidget::HandleSquadRequested(const UUSZoneData* Zone)
{
	SortieZone = Zone;

	// 스테이지가 있는 행성은 지역 지도를 거친다
	if (Zone && Zone->HasStages() && StageMapPanel)
	{
		ShowScreen(EUSHangarScreen::Stage);
		StageMapPanel->ShowZone(Zone, false);
		return;
	}
	if (Zone && Zone->HasStages())
	{
		UE_LOG(LogUS, Warning, TEXT("UUSHangarWidget: StageMapPanel is missing. %s launches without a stage."), *GetNameSafe(Zone));
	}

	if (UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem())
	{
		Meta->SetSortie(Zone, nullptr);
	}
	ShowSquadScreen(Zone);
}

void UUSHangarWidget::SetResumeZone(const UUSZoneData* Zone)
{
	ResumeZone = Zone && Zone->HasStages() ? Zone : nullptr;
	if (Zone && ResumeZone == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSHangarWidget: resume zone %s has no stages. Sortie opens the zone map."), *GetNameSafe(Zone));
	}
}

void UUSHangarWidget::HandleStageSquadRequested(const UUSZoneData* Zone, const UUSStageData* Stage)
{
	if (UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem())
	{
		Meta->SetSortie(Zone, Stage);
	}
	ShowSquadScreen(Zone);
}

void UUSHangarWidget::HandleStageBackRequested()
{
	ShowMapScreen(true);
}

void UUSHangarWidget::ShowSquadScreen(const UUSZoneData* Zone)
{
	// 먼저 화면을 바꾼다: ShowForZone이 주는 키보드 포커스는 보이는 위젯만 받을 수 있다
	ShowScreen(EUSHangarScreen::Squad);
	if (SquadPanel)
	{
		SquadPanel->ShowForZone(Zone);
	}
}

void UUSHangarWidget::HandleMapBackRequested()
{
	ShowScreen(EUSHangarScreen::Hangar);
	// 지도가 가져갔던 키보드 포커스를 돌려받는다 (숨은 지도에 남으면 키가 사라진다)
	SetKeyboardFocus();
}

void UUSHangarWidget::HandleSquadBackRequested()
{
	// 편성에서 돌아오면 방금 고른 지역(또는 구역) 그대로
	if (SortieZone && SortieZone->HasStages() && StageMapPanel)
	{
		ShowScreen(EUSHangarScreen::Stage);
		StageMapPanel->ShowZone(SortieZone, true);
		return;
	}
	ShowMapScreen(true);
}

void UUSHangarWidget::ShowMapScreen(bool bKeepSelection)
{
	ShowScreen(EUSHangarScreen::Map);
	if (MapPanel)
	{
		MapPanel->ShowMap(bKeepSelection);
	}
}

void UUSHangarWidget::HandleSquadLaunchRequested(const UUSZoneData* Zone)
{
	OnLaunchRequested.ExecuteIfBound(Zone);
}

void UUSHangarWidget::HandleMapClicked()
{
	// 행성을 깨는 중이면 강화를 마치고 바로 그 지역 지도로 (Hades·Rogue Legacy: 거점 강화 → 다시 출발)
	if (ResumeZone && StageMapPanel)
	{
		SortieZone = ResumeZone;
		ShowScreen(EUSHangarScreen::Stage);
		StageMapPanel->ShowZone(ResumeZone, false);
		return;
	}

	// 격납고에서 들어오면 구역은 사용자가 새로 고른다
	ShowMapScreen(false);
}

void UUSHangarWidget::HandleTitleClicked()
{
	OnTitleRequested.ExecuteIfBound();
}

UUSMetaProgressionSubsystem* UUSHangarWidget::GetMetaSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr;
}
