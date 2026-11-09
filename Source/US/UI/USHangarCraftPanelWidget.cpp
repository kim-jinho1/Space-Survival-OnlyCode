#include "UI/USHangarCraftPanelWidget.h"
#include "US.h"
#include "Datas/USMetaTreeData.h"
#include "Datas/USMetaUpgradeData.h"
#include "Progression/USMetaProgressionSubsystem.h"
#include "UI/USMetaNodeEntryWidget.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

void UUSHangarCraftPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem())
	{
		MetaChangedHandle = Meta->OnMetaChanged.AddUObject(this, &UUSHangarCraftPanelWidget::HandleMetaChanged);
	}
}

void UUSHangarCraftPanelWidget::NativeDestruct()
{
	if (UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem())
	{
		Meta->OnMetaChanged.Remove(MetaChangedHandle);
	}
	MetaChangedHandle.Reset();

	Super::NativeDestruct();
}

void UUSHangarCraftPanelWidget::ShowCraft(const UUSMetaTreeData* Tree, const UUSAllyTypeData* OwnerType, const FText& Title, const FText& Description)
{
	if (TitleText)
	{
		TitleText->SetText(Title);
	}
	if (DescriptionText)
	{
		DescriptionText->SetText(Description);
	}

	if (Tree == nullptr)
	{
		return;
	}

	if (NodeEntryClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] NodeEntryClass is not set. The panel will be empty."), *GetName());
		return;
	}

	ClearNodeEntries();
	NodeEntries.Reset();

	// 트리 순서 그대로 한 줄씩. 묶음 제목("기체"·"보급")은 목업에 없어 뺐다 — 칸이 기체당 5개 남짓이라 제목 없이도 읽힌다
	for (const UUSMetaUpgradeData* Node : Tree->GetNodes())
	{
		if (Node == nullptr || Node->GetOwnerAllyType() != OwnerType)
		{
			continue;
		}

		UUSMetaNodeEntryWidget* Entry = CreateWidget<UUSMetaNodeEntryWidget>(this, NodeEntryClass);
		if (Entry == nullptr)
		{
			continue;
		}

		Entry->SetupNode(Node);
		Entry->OnNodeClicked.BindUObject(this, &UUSHangarCraftPanelWidget::HandleNodeClicked);
		AddNodeEntry(Entry, Node);
		NodeEntries.Add(Entry);
	}

	HandleMetaChanged();
}

void UUSHangarCraftPanelWidget::AddNodeEntry(UUSMetaNodeEntryWidget* Entry, const UUSMetaUpgradeData* Node)
{
	if (NodeList == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] NodeList is missing. Upgrade %s is not shown."), *GetName(), *GetNameSafe(Node));
		return;
	}
	NodeList->AddChild(Entry);
}

void UUSHangarCraftPanelWidget::ClearNodeEntries()
{
	if (NodeList)
	{
		NodeList->ClearChildren();
	}
}

void UUSHangarCraftPanelWidget::HandleMetaChanged()
{
	const UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem();
	if (Meta == nullptr)
	{
		return;
	}

	for (UUSMetaNodeEntryWidget* Entry : NodeEntries)
	{
		if (Entry)
		{
			Entry->RefreshNode(*Meta);
		}
	}
}

void UUSHangarCraftPanelWidget::HandleNodeClicked(const UUSMetaUpgradeData* Node)
{
	UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem();
	const bool bBought = Meta && Meta->TryBuy(Node);

	// 성공하면 TryBuy 안의 OnMetaChanged가 패널·진열·편성을 다시 그린다
	USoundBase* Sound = bBought ? BuySound.Get() : DeniedSound.Get();
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

UUSMetaProgressionSubsystem* UUSHangarCraftPanelWidget::GetMetaSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr;
}
