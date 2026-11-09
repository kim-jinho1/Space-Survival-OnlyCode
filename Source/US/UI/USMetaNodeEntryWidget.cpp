#include "UI/USMetaNodeEntryWidget.h"
#include "Datas/USMetaUpgradeData.h"
#include "Progression/USMetaProgressionSubsystem.h"
#include "UI/USPipRow.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"

UUSMetaNodeEntryWidget::UUSMetaNodeEntryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LevelFormat = NSLOCTEXT("USMetaNodeEntryWidget", "LevelFormat", "{0} / {1}");
	CostFormat = NSLOCTEXT("USMetaNodeEntryWidget", "CostFormat", "{0}");
	MaxedText = NSLOCTEXT("USMetaNodeEntryWidget", "MaxedText", "최대");
	LockedFormat = NSLOCTEXT("USMetaNodeEntryWidget", "LockedFormat", "{0} 필요");
	PartEffectFormat = NSLOCTEXT("USMetaNodeEntryWidget", "PartEffectFormat", "{1} → {2}");
	PartMaxedFormat = NSLOCTEXT("USMetaNodeEntryWidget", "PartMaxedFormat", "{1}");
	BuyText = NSLOCTEXT("USMetaNodeEntryWidget", "BuyText", "강화");
	ShortText = NSLOCTEXT("USMetaNodeEntryWidget", "ShortText", "부족");
	ActionMaxedText = NSLOCTEXT("USMetaNodeEntryWidget", "ActionMaxedText", "MAX");
	ActionLockedText = NSLOCTEXT("USMetaNodeEntryWidget", "ActionLockedText", "잠김");
}

void UUSMetaNodeEntryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// RefreshNode가 화면에 붙기 전(NativeConstruct 전)에도 불리므로 생성 직후에 잡아 둔다
	if (PartText)
	{
		DefaultPartTextColor = PartText->GetColorAndOpacity();
	}
}

void UUSMetaNodeEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (NodeButton)
	{
		NodeButton->OnClicked.AddUniqueDynamic(this, &UUSMetaNodeEntryWidget::HandleButtonClicked);
	}
}

void UUSMetaNodeEntryWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	bHovered = true;
	RefreshChip();
}

void UUSMetaNodeEntryWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bHovered = false;
	RefreshChip();
}

void UUSMetaNodeEntryWidget::SetupNode(const UUSMetaUpgradeData* InNode)
{
	Node = InNode;
	if (Node == nullptr)
	{
		return;
	}

	if (NameText)
	{
		NameText->SetText(Node->GetDisplayName());
	}
	if (DescriptionText)
	{
		DescriptionText->SetText(Node->GetDescription());
	}
	if (PartText)
	{
		PartText->SetText(Node->GetPartName());
	}
}

void UUSMetaNodeEntryWidget::RefreshNode(const UUSMetaProgressionSubsystem& Meta)
{
	if (Node == nullptr)
	{
		return;
	}

	const int32 Level = Meta.GetLevel(Node);
	const int32 Cost = Meta.GetNextCost(Node);

	EUSMetaNodeState State = EUSMetaNodeState::Affordable;
	if (Meta.IsNodeMaxed(Node))
	{
		State = EUSMetaNodeState::Maxed;
	}
	else if (!Meta.IsNodeOpen(Node))
	{
		State = EUSMetaNodeState::Locked;
	}
	else if (Meta.GetScrap() < Cost)
	{
		State = EUSMetaNodeState::Unaffordable;
	}
	CurrentState = State;

	if (LevelText)
	{
		LevelText->SetText(FText::Format(LevelFormat, FText::AsNumber(Level), FText::AsNumber(Node->GetMaxLevel())));
	}
	RefreshPips(Level, Node->GetMaxLevel());

	// 부위 줄에 이번 강화로 무엇이 바뀌는지 ("+10% → +15%", 부위 이름까지 넣으면 가격 버튼 옆에서 잘린다). 수치가 없는 칸(시작 부하·해금)은 부위만
	// 잠긴 칸은 강화 버튼에 "잠김"만 보이므로, 부위 줄을 선행 칸 안내로 바꾼다 (사용자 2026-11-02: 무엇을 먼저 찍어야 하는지 몰라 불친절)
	const UUSMetaUpgradeData* Prerequisite = Node->GetPrerequisite();
	const bool bShowPrerequisite = ActionText && State == EUSMetaNodeState::Locked && Prerequisite;
	if (PartText)
	{
		PartText->SetColorAndOpacity(bShowPrerequisite ? FSlateColor(LockedPartTextColor) : DefaultPartTextColor);
	}

	if (PartText && bShowPrerequisite)
	{
		PartText->SetText(FText::Format(LockedFormat, Prerequisite->GetDisplayName()));
	}
	else if (PartText)
	{
		const FText CurrentEffect = Node->GetEffectText(Level);
		if (CurrentEffect.IsEmpty())
		{
			PartText->SetText(Node->GetPartName());
		}
		else if (State == EUSMetaNodeState::Maxed)
		{
			PartText->SetText(FText::Format(PartMaxedFormat, Node->GetPartName(), CurrentEffect));
		}
		else
		{
			PartText->SetText(FText::Format(PartEffectFormat, Node->GetPartName(), CurrentEffect, Node->GetEffectText(Level + 1)));
		}
	}

	if (ActionText)
	{
		// 강화 버튼 모양: 상태 글자는 ActionText, 가격 숫자는 살 수 있음·부족일 때만
		const FText* Labels[] = { &ActionLockedText, &BuyText, &ShortText, &ActionMaxedText };
		ActionText->SetText(*Labels[static_cast<uint8>(State)]);

		const bool bShowCost = State == EUSMetaNodeState::Affordable || State == EUSMetaNodeState::Unaffordable;
		if (CostText)
		{
			CostText->SetText(FText::Format(CostFormat, FText::AsNumber(Cost)));
			CostText->SetVisibility(bShowCost ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (ScrapIcon)
		{
			ScrapIcon->SetVisibility(bShowCost ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
	else if (CostText)
	{
		switch (State)
		{
		case EUSMetaNodeState::Maxed:
			CostText->SetText(MaxedText);
			break;

		case EUSMetaNodeState::Locked:
			CostText->SetText(FText::Format(LockedFormat,
				Prerequisite ? Prerequisite->GetDisplayName() : FText::GetEmpty()));
			break;

		default:
			CostText->SetText(FText::Format(CostFormat, FText::AsNumber(Cost)));
			break;
		}
	}
	RefreshChip();

	if (NodeButton)
	{
		const FLinearColor* Colors[] = { &LockedColor, &AffordableColor, &UnaffordableColor, &MaxedColor };
		NodeButton->SetBackgroundColor(*Colors[static_cast<uint8>(State)]);

		// 스크랩 부족은 눌렀을 때 거절음을 들려주려고 켜 둔다. 잠김·최대는 누를 이유가 없으므로 끈다
		NodeButton->SetIsEnabled(State == EUSMetaNodeState::Affordable || State == EUSMetaNodeState::Unaffordable);
	}
}

void UUSMetaNodeEntryWidget::RefreshChip()
{
	// 칩은 버튼을 꺼도(잠김·최대) 바탕색이 그대로라 직접 칠한다 (보급선 가격 칩과 같은 방식)
	const bool bActive = CurrentState == EUSMetaNodeState::Affordable;
	const bool bMaxed = CurrentState == EUSMetaNodeState::Maxed;

	FLinearColor ChipColor = bActive ? ChipAffordableColor : (bMaxed && ActionText ? ChipMaxedColor : ChipIdleColor);
	// 누를 수 있는 상태(살 수 있음·부족)에서만 마우스를 올리면 밝아진다 → "눌리는 것"으로 읽히게
	if (bHovered && (bActive || CurrentState == EUSMetaNodeState::Unaffordable))
	{
		ChipColor = FLinearColor(ChipColor.R * ChipHoverBrightness, ChipColor.G * ChipHoverBrightness, ChipColor.B * ChipHoverBrightness, ChipColor.A);
	}
	if (CostChip)
	{
		CostChip->SetBrushColor(ChipColor);
	}

	if (CostChip == nullptr)
	{
		return;
	}

	const FLinearColor& TextColor = bActive ? ChipAffordableTextColor : (bMaxed && ActionText ? ChipMaxedTextColor : ChipIdleTextColor);
	if (CostText)
	{
		CostText->SetColorAndOpacity(FSlateColor(TextColor));
	}
	// 스크랩 그림은 흰 그림이라 가격 글자와 같은 색을 곱한다
	if (UImage* ScrapImage = Cast<UImage>(ScrapIcon))
	{
		ScrapImage->SetColorAndOpacity(TextColor);
	}
	if (ActionText)
	{
		const FLinearColor& ActionColor = bActive ? ActionAffordableTextColor
			: CurrentState == EUSMetaNodeState::Unaffordable ? ActionShortTextColor : TextColor;
		ActionText->SetColorAndOpacity(FSlateColor(ActionColor));
	}
}

void UUSMetaNodeEntryWidget::RefreshPips(int32 Level, int32 MaxLevel)
{
	if (LevelPips == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	USPipRow::FStyle Style;
	Style.Size = PipSize;
	Style.Height = PipHeight;
	Style.CornerRadius = PipCornerRadius;
	Style.NextOutlineColor = PipNextOutlineColor;
	Style.Spacing = PipSpacing;
	Style.FilledColor = PipFilledColor;
	Style.EmptyColor = PipEmptyColor;
	USPipRow::Refresh(*WidgetTree, *LevelPips, Level, MaxLevel, Style);
}

void UUSMetaNodeEntryWidget::HandleButtonClicked()
{
	OnNodeClicked.ExecuteIfBound(Node);
}
