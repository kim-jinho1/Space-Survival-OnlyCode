#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USExperienceBar.generated.h"

/**
 * 플레이어의 경험치와 레벨을 표시하는 UI
 */
UCLASS()
class US_API UUSExperienceBar : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetRatio(const float Value) const;
	void SetExperience(const float CurrentExperience, const float TargetExperience) const;
	void SetLevel(const int Level) const;
	
protected:
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UProgressBar> ExperienceBar;
	
	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	TObjectPtr<class UTextBlock> ExperienceText;
	
	/** 바 안 레벨 글자. HUD가 레벨 배지를 따로 두면 비워 둔다 */
	UPROPERTY(BlueprintReadWrite, meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> LevelText;
	
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Text")
	FText ExperienceTextFormat = NSLOCTEXT("US.ExperienceBar", "ExperienceFormat", "{0} / {1}");

	UPROPERTY(EditDefaultsOnly, Category = "Text")
	FText LevelTextFormat = NSLOCTEXT("US.ExperienceBar", "LevelFormat", "Lv. {0}");
	
};
