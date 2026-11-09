#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USMapOrbitWidget.generated.h"

class UTexture2D;

/** 궤도 하나: 이 궤도가 지나는 구역 자리와 상태 */
struct FUSMapOrbit
{
	/** 궤도 위 구역 자리 (지도 영역 비율 0~1) */
	FVector2D Position = FVector2D::ZeroVector;

	/** 잠긴 구역 궤도 (흐린 점선) */
	bool bLocked = false;

	/** 고른 구역 궤도 (강조색) */
	bool bSelected = false;
};

/**
 * 출격 지도의 궤도 그림 (B안 궤도 지도, 근거: FTL 섹터 지도·Stellaris 성계 지도처럼 중심에서 바깥으로 갈수록 어려운 구역).
 * 가운데 모함 그림과, 모함을 중심으로 각 구역을 지나는 타원 궤도를 그린다. 그리기만 하고 클릭은 받지 않는다(HitTestInvisible).
 * 출격 지도(UUSHangarMapWidget)가 섬 층(IslandLayer) 맨 아래에 넣으므로 섬 확대 이동을 같이 따라간다.
 * 궤도 반지름은 구역 위치에서 계산한다(데이터 MapPosition만 옮기면 궤도가 따라온다).
 */
UCLASS()
class US_API UUSMapOrbitWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 궤도 중심(지도 영역 비율), 세로/가로 비율, 궤도 목록 */
	void SetOrbits(const FVector2D& InCenter, float InAspect, const TArray<FUSMapOrbit>& InOrbits);

	/** 선 색·굵기, 모함 그림 */
	void SetStyle(const FLinearColor& InOrbitColor, const FLinearColor& InSelectedColor, const FLinearColor& InLockedColor,
		float InThickness, UTexture2D* InHubTexture, const FVector2D& InHubSize);

protected:
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	FVector2D Center = FVector2D(0.5f, 0.5f);
	float Aspect = 0.4f;
	TArray<FUSMapOrbit> Orbits;

	FLinearColor OrbitColor = FLinearColor::White;
	FLinearColor SelectedColor = FLinearColor::White;
	FLinearColor LockedColor = FLinearColor::White;
	float Thickness = 1.5f;
	FVector2D HubSize = FVector2D(96.0f, 96.0f);

	/** 모함 그림 브러시 (그림을 UPROPERTY로 붙잡아 GC를 막는다) */
	FSlateBrush HubBrush;

	UPROPERTY()
	TObjectPtr<UTexture2D> HubTexture;
};
