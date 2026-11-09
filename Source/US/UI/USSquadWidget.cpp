#include "UI/USSquadWidget.h"
#include "US.h"
#include "Agents/USAllyDroneCharacter.h"
#include "Components/USAttributeComponent.h"
#include "Components/USCurrencyComponent.h"
#include "Components/USSquadComponent.h"
#include "Datas/USAllyTypeData.h"
#include "UI/USPlayerHUD.h"
#include "UI/USSquadSlotWidget.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void UUSSquadWidget::InitializeForSquad(UUSSquadComponent* InSquad, UUSCurrencyComponent* InCurrency)
{
	if (InSquad == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: no squad component; squad panel stays empty"), *GetName());
		return;
	}
	if (SlotClass == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: SlotClass is not set; squad panel stays empty"), *GetName());
	}

	Squad = InSquad;
	Currency = InCurrency;

	Squad->OnSquadChanged.AddUObject(this, &UUSSquadWidget::HandleSquadChanged);
	Squad->OnMemberDown.AddUObject(this, &UUSSquadWidget::HandleMemberDown);
	if (Currency)
	{
		Currency->OnGoldChanged.AddUObject(this, &UUSSquadWidget::HandleGoldChanged);
	}

	if (DownNoticeText)
	{
		DownNoticeText->SetVisibility(ESlateVisibility::Collapsed);
	}

	RebuildSlots();
}

void UUSSquadWidget::NativeDestruct()
{
	UnbindMemberHealth();

	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DownFlashTimerHandle);
		World->GetTimerManager().ClearTimer(DownNoticeTimerHandle);
	}

	if (Squad)
	{
		Squad->OnSquadChanged.RemoveAll(this);
		Squad->OnMemberDown.RemoveAll(this);
	}

	if (Currency)
	{
		Currency->OnGoldChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UUSSquadWidget::HandleSquadChanged()
{
	RebuildSlots();
}

void UUSSquadWidget::HandleGoldChanged(int32 NewGold)
{
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		RefreshSlot(Index);
	}
}

void UUSSquadWidget::HandleMemberHealthChanged(float CurrentHealth, float MaxHealth, int32 MemberIndex)
{
	RefreshSlot(MemberIndex);
}

void UUSSquadWidget::HandleMemberDown(int32 MemberIndex)
{
	UWorld* World = GetWorld();
	if (World == nullptr || Squad == nullptr)
	{
		return;
	}

	// 깜빡임: 여러 기가 연달아 격추되면 마지막 칸만 깜빡인다 (앞 칸은 회색으로 끝난 상태가 된다)
	const int32 PreviousIndex = FlashIndex;
	FlashIndex = MemberIndex;
	FlashTogglesLeft = DownFlashToggles;
	RefreshSlot(PreviousIndex);
	World->GetTimerManager().SetTimer(DownFlashTimerHandle, this, &UUSSquadWidget::HandleDownFlashTick, DownFlashInterval, true);

	const TArray<FUSSquadMember>& Members = Squad->GetMembers();
	const UUSAllyTypeData* Type = Members.IsValidIndex(MemberIndex) ? Members[MemberIndex].Type.Get() : nullptr;
	if (DownNoticeText && Type)
	{
		DownNoticeText->SetText(FText::Format(DownNoticeFormat, Type->GetDisplayName()));
		DownNoticeText->SetVisibility(ESlateVisibility::HitTestInvisible);
		World->GetTimerManager().SetTimer(DownNoticeTimerHandle, this, &UUSSquadWidget::HideDownNotice, DownNoticeSeconds, false);
	}

	if (DownSound)
	{
		UGameplayStatics::PlaySound2D(this, DownSound);
	}
}

void UUSSquadWidget::HandleDownFlashTick()
{
	--FlashTogglesLeft;
	const int32 Index = FlashIndex;
	if (FlashTogglesLeft <= 0)
	{
		FlashIndex = INDEX_NONE;
		if (const UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(DownFlashTimerHandle);
		}
	}
	RefreshSlot(Index);
}

void UUSSquadWidget::HideDownNotice()
{
	if (DownNoticeText)
	{
		DownNoticeText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UUSSquadWidget::RebuildSlots()
{
	UnbindMemberHealth();

	if (SlotList == nullptr || Squad == nullptr || SlotClass == nullptr)
	{
		return;
	}

	SlotList->ClearChildren();
	const TArray<FUSSquadMember>& Members = Squad->GetMembers();
	Slots.Reset(Members.Num());
	Slots.SetNum(Members.Num());

	for (int32 Index = 0; Index < Members.Num(); ++Index)
	{
		const FUSSquadMember& Member = Members[Index];
		if (Member.IsEmpty())
		{
			continue;
		}

		UUSSquadSlotWidget* SlotWidget = CreateWidget<UUSSquadSlotWidget>(this, SlotClass);
		if (SlotWidget == nullptr)
		{
			continue;
		}

		SlotList->AddChild(SlotWidget);
		Slots[Index] = SlotWidget;

		// 실루엣이 없는 기종은 컬러 그림으로 대신한다 (색을 곱하면 탁해지지만 아무것도 안 보이는 것보단 낫다)
		UTexture2D* Icon = Member.Type->GetSilhouetteIcon();
		if (Icon == nullptr)
		{
			UE_LOG(LogUS, Warning, TEXT("%s: %s has no SilhouetteIcon; using Icon"), *GetName(), *GetNameSafe(Member.Type));
			Icon = Member.Type->GetIcon();
		}

		// 키 번호는 부활 키·보급선 부활 목록 번호와 같다 (슬롯 인덱스 + 1)
		SlotWidget->SetupSlot(Index + 1, Icon);

		// 격추된 부하는 숨겨진 채 남아 있어 체력이 바뀌지 않는다. 살아 있는 부하만 구독한다
		if (Member.IsActive() && Member.Ally)
		{
			if (UUSAttributeComponent* Attribute = Member.Ally->GetAttributeComponent())
			{
				Attribute->OnHealthChanged.AddUObject(this, &UUSSquadWidget::HandleMemberHealthChanged, Index);
				BoundAttributes.Add(Attribute);
			}
		}

		RefreshSlot(Index);
	}
}

void UUSSquadWidget::RefreshSlot(int32 MemberIndex)
{
	if (Squad == nullptr || !Slots.IsValidIndex(MemberIndex) || Slots[MemberIndex] == nullptr)
	{
		return;
	}

	const TArray<FUSSquadMember>& Members = Squad->GetMembers();
	if (!Members.IsValidIndex(MemberIndex))
	{
		return;
	}

	const FUSSquadMember& Member = Members[MemberIndex];
	UUSSquadSlotWidget* SlotWidget = Slots[MemberIndex];

	if (Member.bDown)
	{
		const int32 ReviveCost = Squad->GetReviveCost();
		const bool bAffordable = Currency && Currency->GetGold() >= ReviveCost;
		// 격추 직후 깜빡이는 동안은 빨강·회색을 번갈아 (남은 횟수 짝수 = 빨강, 시작이 빨강)
		const bool bFlashRed = MemberIndex == FlashIndex && (FlashTogglesLeft % 2) == 0;
		SlotWidget->SetIconColor(bFlashRed ? DownFlashColor : DownColor);
		SlotWidget->SetCost(FText::AsNumber(ReviveCost), bAffordable ? AffordableCostColor : UnaffordableCostColor);
		return;
	}

	float HealthRatio = 1.0f;
	if (const UUSAttributeComponent* Attribute = Member.Ally ? Member.Ally->GetAttributeComponent() : nullptr)
	{
		HealthRatio = UUSPlayerHUD::GetSafeRatio(Attribute->GetCurrentHealth(), Attribute->GetMaxHealth());
	}
	SlotWidget->SetIconColor(GetHealthColor(HealthRatio));
	SlotWidget->SetCost(FText::GetEmpty(), AffordableCostColor);
}

FLinearColor UUSSquadWidget::GetHealthColor(float HealthRatio) const
{
	if (HealthRatio > HealthyThreshold)
	{
		return HealthyColor;
	}
	return HealthRatio > CriticalThreshold ? DamagedColor : CriticalColor;
}

void UUSSquadWidget::UnbindMemberHealth()
{
	for (const TWeakObjectPtr<UUSAttributeComponent>& Attribute : BoundAttributes)
	{
		if (Attribute.IsValid())
		{
			Attribute->OnHealthChanged.RemoveAll(this);
		}
	}
	BoundAttributes.Reset();
}
