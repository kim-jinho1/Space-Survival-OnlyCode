#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USDamageNumberWidget.generated.h"

class UCanvasPanel;
class UCanvasPanelSlot;
class UTextBlock;

/** 풀에 든 데미지 숫자 하나 */
USTRUCT()
struct FUSDamageNumberEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UTextBlock> Text;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> Slot;

	/** 숫자가 붙어 있는 월드 위치 (매 프레임 화면 좌표로 바꾼다) */
	FVector WorldLocation = FVector::ZeroVector;

	/** 겹침을 줄이는 가로 흔들림(화면 픽셀) */
	float JitterX = 0.0f;

	/** 표시된 뒤 지난 시간 */
	float Age = 0.0f;

	bool bActive = false;
};

/**
 * 대장이 준 데미지 숫자. 텍스트를 미리 만들어 두고 돌려 쓴다(풀링) — 맞을 때마다 위젯을 만들지 않는다.
 * 숫자는 월드 위치를 따라가며 위로 떠오르고 흐려진다. 떠 있는 숫자가 없으면 Tick은 바로 빠져나온다.
 * 캔버스 위젯 이름은 NumberCanvas여야 한다.
 */
UCLASS()
class US_API UUSDamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** WorldLocation 위에 데미지 숫자를 띄운다. 풀이 다 쓰였으면 가장 오래된 것을 재사용한다 */
	void ShowDamageNumber(const FVector& WorldLocation, float Damage);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 미리 만들어 둘 숫자 개수 */
	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber", meta = (ClampMin = "1"))
	int32 PoolSize = 40;

	/** 숫자가 떠 있는 시간(초) */
	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber", meta = (ClampMin = "0.05"))
	float Lifetime = 0.6f;

	/** 사라질 때까지 떠오르는 높이(화면 픽셀) */
	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	float RiseDistance = 50.0f;

	/** 가로 흔들림 최대치(화면 픽셀). 같은 적의 연속 숫자가 겹치지 않게 */
	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	float MaxJitterX = 15.0f;

	/** 글자 크기 */
	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber", meta = (ClampMin = "1"))
	int32 FontSize = 34;

	/** 글자 색 */
	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	FLinearColor TextColor = FLinearColor(1.0f, 0.9f, 0.3f, 1.0f);

	/** 숫자를 올릴 캔버스 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> NumberCanvas;

private:
	/** 풀을 채운다 (NativeConstruct에서 한 번) */
	void BuildPool();

	/** 쓸 숫자 하나를 고른다: 빈 것, 없으면 가장 오래된 것 */
	int32 AcquireEntryIndex() const;

	/** 숫자를 숨기고 풀로 돌린다 */
	void ReleaseEntry(FUSDamageNumberEntry& Entry);

	UPROPERTY()
	TArray<FUSDamageNumberEntry> Entries;

	/** 떠 있는 숫자 수. 0이면 Tick에서 바로 빠진다 */
	int32 ActiveCount = 0;
};
