#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USHealthBar.generated.h"

/**
 * 플레이어의 체럭을 표시하는 UI
 */
UCLASS()
class US_API UUSHealthBar : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetRatio(const float Value) const;
	void SetText(const float Current, const float MaxValue) const;
	
protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UProgressBar> HealthBar;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> HealthText;
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Text")
	FText HealthTextFormat = NSLOCTEXT("US.HealthBar", "HealthFormat", "{0} / {1}");
	
};
