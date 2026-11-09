#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USQuickInfoUI.generated.h"

/**
 * 빠른 정보를 표시하는 UI (골드, 처치 수, 편대 DPS)
 */
UCLASS()
class US_API UUSQuickInfoUI : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetGoldAmount(const int32 Value) const;

	/** 누적 처치 수 */
	void SetKillCount(int32 Value) const;

	/** 편대 초당 피해와 그중 부하 비율(0~1) */
	void SetSquadDps(float Dps, float AllyShare) const;

protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> GoldAmount;

	/** 처치 수 글자 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> KillCountText;

	/** 편대 DPS 글자 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> SquadDpsText;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Text")
	FText GoldAmountTextFormat = NSLOCTEXT("US.QuickInfo", "GoldFormat", "골드 {0}");

	/** 처치 수 문구. {0} = 처치 수. "처치" 글자 대신 WBP의 조준경 아이콘이 붙는다 */
	UPROPERTY(EditDefaultsOnly, Category = "Text")
	FText KillCountTextFormat = NSLOCTEXT("US.QuickInfo", "KillCountFormat", "{0}");

	/** 편대 DPS 문구. {0} = 초당 피해, {1} = 부하 비율(%) */
	UPROPERTY(EditDefaultsOnly, Category = "Text")
	FText SquadDpsTextFormat = NSLOCTEXT("US.QuickInfo", "SquadDpsFormat", "편대 DPS {0} · 아군 {1}%");
};
