#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USCreditsWidget.generated.h"

class UButton;
class UTextBlock;

/** 크레딧 창을 닫음 */
DECLARE_DELEGATE(FDelegateOnCreditsClosed);

/**
 * 크레딧 창 (타이틀). 부모 WBP 안에 숨긴 채 넣어 두고 Open으로 연다 (설정 창과 같은 방식).
 * 문구는 WBP가 아니라 CreditsBody 기본값(C++)에 둔다 — .uasset은 git diff로 볼 수 없어서 출처·라이선스 변경 이력이 남지 않는다.
 * 루트는 화면 전체를 덮는 배경이라 뒤의 메뉴 버튼이 눌리지 않는다. 글이 길면 WBP의 ScrollBox가 휠 스크롤을 맡는다.
 * 위젯 이름: CreditsText, CloseButton.
 */
UCLASS()
class US_API UUSCreditsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UUSCreditsWidget(const FObjectInitializer& ObjectInitializer);

	/** 문구를 채우고 보인다 */
	void Open();

	/** 숨긴다 */
	void Close();

	/** 열려 있는가 */
	bool IsOpen() const;

	/** 닫힘 */
	FDelegateOnCreditsClosed OnClosed;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleCloseClicked();

	/** 크레딧 본문 (개발자, 외부 애셋 출처·라이선스, 엔진 고지) */
	UPROPERTY(EditDefaultsOnly, Category = "Credits|Text", meta = (MultiLine = true))
	FText CreditsBody;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CreditsText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> CloseButton;
};
