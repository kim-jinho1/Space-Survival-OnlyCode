#include "UI/USHangarSquadWidget.h"
#include "US.h"
#include "Datas/USMetaTreeData.h"
#include "Datas/USMetaUpgradeData.h"
#include "Datas/USZoneData.h"
#include "Progression/USMetaProgressionSubsystem.h"
#include "UI/USHangarOptionWidget.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

UUSHangarSquadWidget::UUSHangarSquadWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 격납고 설계도 분류와 같은 말
	RoleTexts.Add(EUSAllyRole::Ranged, NSLOCTEXT("USHangarSquadWidget", "RoleRanged", "공격형"));
	RoleTexts.Add(EUSAllyRole::Tank, NSLOCTEXT("USHangarSquadWidget", "RoleTank", "방어형"));
	RoleTexts.Add(EUSAllyRole::Slow, NSLOCTEXT("USHangarSquadWidget", "RoleSlow", "지원형"));
	LeaderSubText = NSLOCTEXT("USHangarSquadWidget", "LeaderSubText", "고정");
	EmptySlotText = NSLOCTEXT("USHangarSquadWidget", "EmptySlotText", "비어 있음");
	PickingSubText = NSLOCTEXT("USHangarSquadWidget", "PickingSubText", "고르는 중");
	LockedSlotSubText = NSLOCTEXT("USHangarSquadWidget", "LockedSlotSubText", "격납고 강화");
	PickerTitleFormat = NSLOCTEXT("USHangarSquadWidget", "PickerTitleFormat", "{0}번 자리에 넣을 전투기");
	CountFormat = NSLOCTEXT("USHangarSquadWidget", "CountFormat", "{0} / {1}대");
}

void UUSHangarSquadWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LaunchButton)
	{
		LaunchButton->OnClicked.AddUniqueDynamic(this, &UUSHangarSquadWidget::HandleLaunchClicked);
	}
	if (BackButton)
	{
		BackButton->OnClicked.AddUniqueDynamic(this, &UUSHangarSquadWidget::HandleBackClicked);
	}
	if (ClearButton)
	{
		ClearButton->OnClicked.AddUniqueDynamic(this, &UUSHangarSquadWidget::HandleClearClicked);
	}

	if (UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem())
	{
		MetaChangedHandle = Meta->OnMetaChanged.AddUObject(this, &UUSHangarSquadWidget::HandleMetaChanged);
	}

	// 키를 받으려면 포커스를 받을 수 있어야 한다 (포커스는 ShowForZone에서 준다). 버튼들은 WBP에서 IsFocusable = false
	SetIsFocusable(true);
}

void UUSHangarSquadWidget::NativeDestruct()
{
	if (UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem())
	{
		Meta->OnMetaChanged.Remove(MetaChangedHandle);
	}
	MetaChangedHandle.Reset();

	Super::NativeDestruct();
}

FReply UUSHangarSquadWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		HandleBackClicked();
		return FReply::Handled();
	}

	if (Key == EKeys::Up || Key == EKeys::W || Key == EKeys::Gamepad_DPad_Up)
	{
		MovePickingSlot(-1);
	}
	else if (Key == EKeys::Down || Key == EKeys::S || Key == EKeys::Gamepad_DPad_Down)
	{
		MovePickingSlot(1);
	}
	else if (Key == EKeys::Left || Key == EKeys::A || Key == EKeys::Gamepad_DPad_Left)
	{
		MovePickerCursor(-1);
	}
	else if (Key == EKeys::Right || Key == EKeys::D || Key == EKeys::Gamepad_DPad_Right)
	{
		MovePickerCursor(1);
	}
	else if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		PlaceType(PickerCursor);
	}
	else if (Key == EKeys::Delete || Key == EKeys::BackSpace || Key == EKeys::Gamepad_FaceButton_Left)
	{
		HandleClearClicked();
	}
	else
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}

	// 키를 한 번이라도 쓰면 기종 커서를 보인다
	if (!bShowKeyCursor)
	{
		bShowKeyCursor = true;
		RefreshPickerCursor();
	}
	return FReply::Handled();
}

void UUSHangarSquadWidget::SetupSquad(const UUSMetaTreeData* InTree, UTexture2D* InLeaderIcon, const FText& InLeaderName)
{
	Tree = InTree;
	LeaderIcon = InLeaderIcon;
	LeaderName = InLeaderName;

	if (CardClass == nullptr || PickerOptionClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSHangarSquadWidget: CardClass or PickerOptionClass is not set. The squad screen will be empty."));
	}
	if (EmptySlotIcon == nullptr || LockedSlotIcon == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSHangarSquadWidget: EmptySlotIcon or LockedSlotIcon is not set. Those cards show text only."));
	}

	Refresh();
}

void UUSHangarSquadWidget::ShowForZone(const UUSZoneData* InZone)
{
	Zone = InZone;
	// 화면에 들어올 때마다 1번 자리부터
	PickingSlot = 0;
	bShowKeyCursor = false;
	ResetPickerCursor();
	Refresh();

	if (LaunchButton)
	{
		LaunchButton->SetIsEnabled(Zone != nullptr);
	}
	SetKeyboardFocus();
}

void UUSHangarSquadWidget::HandleMetaChanged()
{
	Refresh();
}

void UUSHangarSquadWidget::Refresh()
{
	// 편성 칸 수가 줄 일은 없지만(강화는 늘리기만 한다) 세이브가 바뀌어도 고른 자리가 칸 밖에 있지 않게
	if (const UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem(); Meta && Tree)
	{
		const int32 SlotCount = Meta->GetLoadoutSlotCount(Tree);
		PickingSlot = SlotCount > 0 ? FMath::Clamp(PickingSlot, 0, SlotCount - 1) : INDEX_NONE;
	}

	RefreshCards();
	RefreshPicker();
}

void UUSHangarSquadWidget::RefreshCards()
{
	const UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem();
	if (CardList == nullptr || CardClass == nullptr || Meta == nullptr || Tree == nullptr)
	{
		return;
	}

	// 자리 줄을 누를 때·편성이 바뀔 때만 불리므로(클릭) 매번 새로 만들어도 비용이 작다
	CardList->ClearChildren();
	const TArray<const UUSAllyTypeData*> Slots = Meta->GetLoadoutSlots(Tree);

	// 대장 카드: 늘 첫 자리, 바꿀 수 없다 (번호 INDEX_NONE → 클릭 무시)
	if (UUSHangarOptionWidget* LeaderCard = CreateWidget<UUSHangarOptionWidget>(this, CardClass))
	{
		CardList->AddChild(LeaderCard);
		LeaderCard->SetupOption(INDEX_NONE, LeaderName, LeaderSubText, LeaderIcon, true, false);
		LeaderCard->SetButtonColor(LeaderCardColor);
	}

	int32 FilledCount = 0;
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		UUSHangarOptionWidget* Card = CreateWidget<UUSHangarOptionWidget>(this, CardClass);
		if (Card == nullptr)
		{
			continue;
		}
		CardList->AddChild(Card);
		Card->OnOptionClicked.BindUObject(this, &UUSHangarSquadWidget::HandleCardClicked);
		Card->SetCornerText(FText::AsNumber(Index + 1));

		const UUSAllyTypeData* Type = Slots[Index];
		const bool bPicking = Index == PickingSlot;
		if (Type)
		{
			++FilledCount;
			Card->SetupOption(Index, Type->GetDisplayName(), bPicking ? PickingSubText : GetRoleText(Type), Type->GetIcon(), true, bPicking);
		}
		else
		{
			Card->SetupOption(Index, EmptySlotText, bPicking ? PickingSubText : FText::GetEmpty(), EmptySlotIcon, true, bPicking);
		}
	}

	// 편성 칸 업그레이드가 남았으면 다음 칸을 잠긴 카드로 보여 "칸을 늘릴 수 있다"를 알린다 (번호 = 칸 수 → 클릭 시 거절음)
	for (const UUSMetaUpgradeData* Node : Tree->GetNodes())
	{
		if (Node && Node->GetEffect() == EUSMetaEffect::StartingAlly && !Meta->IsNodeMaxed(Node))
		{
			if (UUSHangarOptionWidget* LockedCard = CreateWidget<UUSHangarOptionWidget>(this, CardClass))
			{
				CardList->AddChild(LockedCard);
				LockedCard->OnOptionClicked.BindUObject(this, &UUSHangarSquadWidget::HandleCardClicked);
				LockedCard->SetupOption(Slots.Num(), FText::GetEmpty(), LockedSlotSubText, LockedSlotIcon, false, false);
				LockedCard->SetCornerText(FText::AsNumber(Slots.Num() + 1));
			}
			break;
		}
	}

	if (ZoneText)
	{
		ZoneText->SetText(Zone ? Zone->GetDisplayName() : FText::GetEmpty());
	}
	if (CountText)
	{
		CountText->SetText(FText::Format(CountFormat, FText::AsNumber(FilledCount), FText::AsNumber(Slots.Num())));
	}
}

void UUSHangarSquadWidget::HandleCardClicked(int32 CardIndex)
{
	const UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem();
	if (CardIndex == INDEX_NONE || Meta == nullptr)
	{
		return;
	}

	// 마우스로 누르면 키보드 테두리를 숨긴다. 버튼이 포커스를 안 받아도 혹시 잃었으면 돌려받는다
	bShowKeyCursor = false;
	SetKeyboardFocus();

	if (CardIndex >= Meta->GetLoadoutSlotCount(Tree))
	{
		PlayDenied();
		return;
	}

	PickingSlot = CardIndex;
	ResetPickerCursor();
	Refresh();
}

void UUSHangarSquadWidget::MovePickingSlot(int32 Delta)
{
	const UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem();
	if (Meta == nullptr || Tree == nullptr || PickingSlot == INDEX_NONE)
	{
		return;
	}

	// 대장 줄(고정)과 잠긴 줄은 건너뛴다: 움직일 수 있는 건 편성 칸뿐
	const int32 NewSlot = FMath::Clamp(PickingSlot + Delta, 0, Meta->GetLoadoutSlotCount(Tree) - 1);
	if (NewSlot == PickingSlot)
	{
		return;
	}

	PickingSlot = NewSlot;
	ResetPickerCursor();
	Refresh();
}

void UUSHangarSquadWidget::MovePickerCursor(int32 Delta)
{
	if (PickerTypes.IsEmpty())
	{
		return;
	}

	PickerCursor = FMath::Clamp(PickerCursor + Delta, 0, PickerTypes.Num() - 1);
	RefreshPickerCursor();
}

void UUSHangarSquadWidget::ResetPickerCursor()
{
	PickerCursor = 0;

	const UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem();
	if (Meta == nullptr || Tree == nullptr)
	{
		return;
	}

	const TArray<const UUSAllyTypeData*> Slots = Meta->GetLoadoutSlots(Tree);
	const UUSAllyTypeData* Current = Slots.IsValidIndex(PickingSlot) ? Slots[PickingSlot] : nullptr;
	if (Current == nullptr)
	{
		return;
	}

	TArray<const UUSAllyTypeData*> Candidates;
	GetCandidates(Candidates);
	PickerCursor = FMath::Max(Candidates.IndexOfByKey(Current), 0);
}

void UUSHangarSquadWidget::RefreshPickerCursor()
{
	if (PickerList == nullptr)
	{
		return;
	}

	// 카드는 RefreshPicker가 PickerTypes와 같은 순서로 만든다
	for (int32 Index = 0; Index < PickerList->GetChildrenCount(); ++Index)
	{
		if (UUSHangarOptionWidget* Option = Cast<UUSHangarOptionWidget>(PickerList->GetChildAt(Index)))
		{
			Option->SetKeyFocused(bShowKeyCursor && Index == PickerCursor);
		}
	}
}

void UUSHangarSquadWidget::RefreshPicker()
{
	const UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem();
	if (PickerList == nullptr || Meta == nullptr || Tree == nullptr)
	{
		return;
	}

	PickerList->ClearChildren();
	PickerTypes.Reset();

	const TArray<const UUSAllyTypeData*> Slots = Meta->GetLoadoutSlots(Tree);
	const bool bHasSlot = Slots.IsValidIndex(PickingSlot);
	const UUSAllyTypeData* Current = bHasSlot ? Slots[PickingSlot] : nullptr;

	if (PickerTitleText)
	{
		PickerTitleText->SetText(bHasSlot ? FText::Format(PickerTitleFormat, FText::AsNumber(PickingSlot + 1)) : FText::GetEmpty());
	}
	if (ClearButton)
	{
		ClearButton->SetIsEnabled(Current != nullptr);
	}
	if (!bHasSlot || PickerOptionClass == nullptr)
	{
		return;
	}

	TArray<const UUSAllyTypeData*> Candidates;
	GetCandidates(Candidates);
	PickerTypes.Reserve(Candidates.Num());

	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		const UUSAllyTypeData* Type = Candidates[Index];
		PickerTypes.Add(Type);
		if (UUSHangarOptionWidget* Option = CreateWidget<UUSHangarOptionWidget>(this, PickerOptionClass))
		{
			PickerList->AddChild(Option);
			// 잠긴 기종은 기체 그림 대신 자물쇠 (격납고 위쪽 기종 아이콘과 같은 규칙)
			const bool bUnlocked = Meta->IsAllyTypeUnlocked(Tree, Type);
			UTexture2D* Icon = bUnlocked || LockedSlotIcon == nullptr ? Type->GetIcon() : LockedSlotIcon.Get();
			Option->SetupOption(Index, Type->GetDisplayName(), Type->GetSummary(), Icon, bUnlocked, Type == Current);
			Option->SetCornerText(GetRoleText(Type));
			Option->OnOptionClicked.BindUObject(this, &UUSHangarSquadWidget::HandlePickerClicked);
		}
	}

	PickerCursor = PickerTypes.IsEmpty() ? 0 : FMath::Clamp(PickerCursor, 0, PickerTypes.Num() - 1);
	RefreshPickerCursor();
}

void UUSHangarSquadWidget::HandlePickerClicked(int32 OptionIndex)
{
	// 마우스로 누른 카드 = 커서. 테두리는 숨긴다
	bShowKeyCursor = false;
	PickerCursor = OptionIndex;
	SetKeyboardFocus();
	RefreshPickerCursor();
	PlaceType(OptionIndex);
}

void UUSHangarSquadWidget::PlaceType(int32 OptionIndex)
{
	UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem();
	if (Meta == nullptr || PickingSlot == INDEX_NONE || !PickerTypes.IsValidIndex(OptionIndex))
	{
		return;
	}

	// 성공하면 OnMetaChanged가 화면을 다시 그린다. 고른 자리는 그대로 둔다
	if (!Meta->SetLoadoutSlot(Tree, PickingSlot, PickerTypes[OptionIndex]))
	{
		// 잠긴 기종
		PlayDenied();
	}
}

void UUSHangarSquadWidget::HandleClearClicked()
{
	if (UUSMetaProgressionSubsystem* Meta = GetMetaSubsystem(); Meta && PickingSlot != INDEX_NONE)
	{
		Meta->SetLoadoutSlot(Tree, PickingSlot, nullptr);
	}
}

void UUSHangarSquadWidget::GetCandidates(TArray<const UUSAllyTypeData*>& OutTypes) const
{
	OutTypes.Reset();
	if (Tree == nullptr)
	{
		return;
	}

	if (Tree->GetDefaultAllyType())
	{
		OutTypes.Add(Tree->GetDefaultAllyType());
	}
	for (const UUSMetaUpgradeData* Node : Tree->GetNodes())
	{
		if (Node && Node->GetEffect() == EUSMetaEffect::UnlockAllyType && Node->GetAllyType())
		{
			OutTypes.AddUnique(Node->GetAllyType());
		}
	}
}

FText UUSHangarSquadWidget::GetRoleText(const UUSAllyTypeData* Type) const
{
	const FText* Text = Type ? RoleTexts.Find(Type->GetRole()) : nullptr;
	return Text ? *Text : FText::GetEmpty();
}

void UUSHangarSquadWidget::PlayDenied() const
{
	if (DeniedSound)
	{
		UGameplayStatics::PlaySound2D(this, DeniedSound);
	}
}

void UUSHangarSquadWidget::HandleLaunchClicked()
{
	if (Zone)
	{
		OnLaunchRequested.ExecuteIfBound(Zone);
	}
}

void UUSHangarSquadWidget::HandleBackClicked()
{
	OnBackRequested.ExecuteIfBound();
}

UUSMetaProgressionSubsystem* UUSHangarSquadWidget::GetMetaSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr;
}
