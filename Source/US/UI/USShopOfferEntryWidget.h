#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USShopOfferEntryWidget.generated.h"

class UBorder;
class UButton;
class UImage;
class UTextBlock;
struct FUSShopOffer;

/** 상품 줄 클릭 (목록 안 순번) */
DECLARE_DELEGATE_OneParam(FDelegateOnOfferClicked, int32);

/**
 * 상점 상품 카드: 버튼 안에 아이콘 · 이름 · 효과 · 설명 · 가격.
 * 위젯 이름: OfferButton, OfferTitle, OfferCost, (선택) OfferEffect, OfferDetail, OfferIcon, CostChip. 상점 위젯이 상품 수만큼 만들어 줄에 넣는다.
 */
UCLASS()
class US_API UUSShopOfferEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSShopOfferEntryWidget(const FObjectInitializer& ObjectInitializer);

	/** 상품을 채운다. bAffordable이 false면 버튼을 비활성화한다 */
	void SetupOffer(int32 InIndex, const FUSShopOffer& Offer, bool bAffordable);

	/** 클릭 알림 (상점 위젯이 바인딩) */
	FDelegateOnOfferClicked OnOfferClicked;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleButtonClicked();

	/** 가격 글자. {0} = 골드 */
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	FText CostFormat;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> OfferButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> OfferTitle;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> OfferCost;

	/** 가격 칩 바탕색 (살 수 있음 / 골드 부족) */
	UPROPERTY(EditDefaultsOnly, Category = "Offer|Color")
	FLinearColor AffordableChipColor = FLinearColor(0.05f, 0.63f, 0.8f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Offer|Color")
	FLinearColor UnaffordableChipColor = FLinearColor(0.08f, 0.1f, 0.14f, 1.0f);

	/** 가격 글자색 (청록 칩 위는 어둡게 / 회색 칩 위는 흐리게) */
	UPROPERTY(EditDefaultsOnly, Category = "Offer|Color")
	FSlateColor AffordableCostColor = FLinearColor(0.0f, 0.02f, 0.03f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Offer|Color")
	FSlateColor UnaffordableCostColor = FLinearColor(0.45f, 0.5f, 0.58f, 1.0f);

	/** 가격을 감싼 칩 (없으면 글자만) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CostChip;

	/** 상품 아이콘 (없는 상품은 숨긴다) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> OfferIcon;

	/** 이름 아래 큰 효과 글자 ("+15초") */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OfferEffect;

	/** 맨 아래 작은 설명 ("구매 2회") */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OfferDetail;

private:
	/** 목록 안 순번 (클릭 시 그대로 돌려준다) */
	int32 OfferIndex = INDEX_NONE;
};
