#include "UI/USDamageNumberWidget.h"
#include "US.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

void UUSDamageNumberWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 숫자가 마우스 조준·클릭을 가로채지 않게 한다
	SetVisibility(ESlateVisibility::HitTestInvisible);

	BuildPool();
}

void UUSDamageNumberWidget::BuildPool()
{
	if (NumberCanvas == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: NumberCanvas is missing; damage numbers are disabled"), *GetName());
		return;
	}

	// 위젯이 다시 Construct 되어도 풀을 두 번 만들지 않는다
	if (!Entries.IsEmpty())
	{
		return;
	}

	Entries.Reserve(PoolSize);
	for (int32 Index = 0; Index < PoolSize; ++Index)
	{
		UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Text->SetVisibility(ESlateVisibility::Collapsed);
		Text->SetColorAndOpacity(FSlateColor(TextColor));

		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Font.OutlineSettings.OutlineSize = 2;
		Font.OutlineSettings.OutlineColor = FLinearColor::Black;
		Text->SetFont(Font);

		UCanvasPanelSlot* CanvasSlot = NumberCanvas->AddChildToCanvas(Text);
		CanvasSlot->SetAutoSize(true);
		// 위치가 글자 가운데를 가리키게
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));

		FUSDamageNumberEntry& Entry = Entries.AddDefaulted_GetRef();
		Entry.Text = Text;
		Entry.Slot = CanvasSlot;
	}
}

void UUSDamageNumberWidget::ShowDamageNumber(const FVector& WorldLocation, float Damage)
{
	// 소수점 없이 표시하므로 0.5 미만 피해(방어·감쇠로 깎인 피해)는 "0"으로 떠 버린다
	if (Entries.IsEmpty() || FMath::RoundToInt(Damage) <= 0)
	{
		return;
	}

	FUSDamageNumberEntry& Entry = Entries[AcquireEntryIndex()];
	if (!Entry.bActive)
	{
		++ActiveCount;
	}

	FNumberFormattingOptions Options;
	Options.MaximumFractionalDigits = 0;
	Entry.Text->SetText(FText::AsNumber(Damage, &Options));
	Entry.Text->SetRenderOpacity(1.0f);
	Entry.Text->SetVisibility(ESlateVisibility::HitTestInvisible);

	Entry.WorldLocation = WorldLocation;
	Entry.JitterX = FMath::FRandRange(-MaxJitterX, MaxJitterX);
	Entry.Age = 0.0f;
	Entry.bActive = true;
}

int32 UUSDamageNumberWidget::AcquireEntryIndex() const
{
	int32 OldestIndex = 0;
	float OldestAge = -1.0f;

	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const FUSDamageNumberEntry& Entry = Entries[Index];
		if (!Entry.bActive)
		{
			return Index;
		}

		if (Entry.Age > OldestAge)
		{
			OldestAge = Entry.Age;
			OldestIndex = Index;
		}
	}

	return OldestIndex;
}

void UUSDamageNumberWidget::ReleaseEntry(FUSDamageNumberEntry& Entry)
{
	Entry.Text->SetVisibility(ESlateVisibility::Collapsed);
	Entry.bActive = false;
	--ActiveCount;
}

void UUSDamageNumberWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (ActiveCount <= 0)
	{
		return;
	}

	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController == nullptr)
	{
		return;
	}

	for (FUSDamageNumberEntry& Entry : Entries)
	{
		if (!Entry.bActive)
		{
			continue;
		}

		Entry.Age += InDeltaTime;
		if (Entry.Age >= Lifetime)
		{
			ReleaseEntry(Entry);
			continue;
		}

		// 카메라가 움직여도 숫자가 적 위에 붙어 있도록 매 프레임 다시 투영한다 (DPI 배율 반영된 위젯 좌표)
		FVector2D ScreenPosition;
		if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PlayerController, Entry.WorldLocation, ScreenPosition, false))
		{
			Entry.Text->SetRenderOpacity(0.0f);
			continue;
		}

		const float Alpha = Entry.Age / Lifetime;
		Entry.Slot->SetPosition(ScreenPosition + FVector2D(Entry.JitterX, -RiseDistance * Alpha));
		// 처음엔 또렷하게 있다가 끝에서 빠르게 사라지도록 제곱으로 줄인다
		Entry.Text->SetRenderOpacity(1.0f - Alpha * Alpha);
	}
}
