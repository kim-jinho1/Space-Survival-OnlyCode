#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USShopWidget.generated.h"

class UButton;
class UPanelWidget;
class UTextBlock;
class UWidget;
class USoundBase;
class UUSShopOfferEntryWidget;
struct FUSShopOffer;

/** 상품 선택 (목록 안 순번) */
DECLARE_DELEGATE_OneParam(FDelegateOnShopOfferSelected, int32);

/** 상점 닫기 */
DECLARE_DELEGATE(FDelegateOnShopClosed);

/**
 * 보급선 상점 창. 상품을 카드 위젯(OfferEntryClass)으로 그리고 클릭·닫기를 컨트롤러에 알린다.
 * 카드는 두 줄로 나눈다: 아군 줄(수리·부활·합류) = SquadOfferList, 강화 줄(반복 강화) = OfferList.
 * 아군 상품이 없으면 SquadGroup(머리글 + 줄)을 통째로 숨긴다. SquadOfferList가 없으면 전부 OfferList 한 줄에 넣는다.
 * 사고파는 판단은 하지 않는다 (UUSShopComponent 담당).
 * 위젯 이름: OfferList, GoldText, CloseButton, (선택) SquadOfferList, SquadGroup, EmptyText(살 것이 없을 때).
 */
UCLASS()
class US_API UUSShopWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSShopWidget(const FObjectInitializer& ObjectInitializer);

	/** 상품 목록과 보유 골드로 창을 다시 그린다 (열 때, 구매 후) */
	void SetupOffers(const TArray<FUSShopOffer>& Offers, int32 CurrentGold);

	/** 상품 선택 알림 */
	FDelegateOnShopOfferSelected OnOfferSelected;

	/** 닫기 알림 */
	FDelegateOnShopClosed OnShopClosed;

protected:
	virtual void NativeConstruct() override;

	/** 수리·부활·합류는 아군 줄, 반복 강화는 강화 줄 */
	UPanelWidget* GetListForOffer(const FUSShopOffer& Offer) const;

	/** 줄 위젯 클릭 → 컨트롤러에 전달 */
	void HandleOfferClicked(int32 Index);

	UFUNCTION()
	void HandleCloseClicked();

	/** 상품 한 줄 위젯 클래스 (WBP) */
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<UUSShopOfferEntryWidget> OfferEntryClass;

	/** 보유 골드 글자. {0} = 골드 */
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	FText GoldFormat;

	/** 창이 열릴 때 소리 */
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TObjectPtr<USoundBase> OpenSound;

	/** 강화 줄 (SquadOfferList가 없으면 모든 상품) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> OfferList;

	/** 아군 줄: 수리·부활·합류 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> SquadOfferList;

	/** 아군 머리글 + 아군 줄을 감싼 위젯 (아군 상품이 없으면 숨긴다) */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SquadGroup;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GoldText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EmptyText;
};
