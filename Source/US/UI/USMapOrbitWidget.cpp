#include "UI/USMapOrbitWidget.h"
#include "Engine/Texture2D.h"

namespace
{
	/** 타원 하나를 몇 조각 선으로 그리나 (점선은 짝수 조각만) */
	constexpr int32 OrbitSegments = 96;
}

void UUSMapOrbitWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 그리기만 한다: 섬 클릭·빈 곳 클릭(선택 해제)을 막지 않게
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UUSMapOrbitWidget::SetOrbits(const FVector2D& InCenter, float InAspect, const TArray<FUSMapOrbit>& InOrbits)
{
	Center = InCenter;
	Aspect = FMath::Max(InAspect, 0.05f);
	Orbits = InOrbits;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void UUSMapOrbitWidget::SetStyle(const FLinearColor& InOrbitColor, const FLinearColor& InSelectedColor, const FLinearColor& InLockedColor,
	float InThickness, UTexture2D* InHubTexture, const FVector2D& InHubSize)
{
	OrbitColor = InOrbitColor;
	SelectedColor = InSelectedColor;
	LockedColor = InLockedColor;
	Thickness = InThickness;
	HubTexture = InHubTexture;
	HubSize = InHubSize;
	HubBrush.SetResourceObject(HubTexture);
	HubBrush.ImageSize = FVector2f(HubSize);
	HubBrush.DrawAs = ESlateBrushDrawType::Image;
	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 UUSMapOrbitWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	const FVector2f Size = AllottedGeometry.GetLocalSize();
	if (Size.X <= 0.0f || Size.Y <= 0.0f)
	{
		return MaxLayer;
	}

	const FVector2f CenterLocal = FVector2f(Center) * Size;
	const int32 OrbitLayer = MaxLayer + 1;

	// 고른 궤도는 다른 궤도 위에 그려 겹친 곳에서도 끊기지 않게
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		for (const FUSMapOrbit& Orbit : Orbits)
		{
			if (Orbit.bSelected != (Pass == 1))
			{
				continue;
			}

			// 구역 자리를 지나는 타원: (dx / Rx)² + (dy / (Rx × Aspect))² = 1 → Rx = √(dx² + (dy / Aspect)²)
			const FVector2f Delta = FVector2f(Orbit.Position) * Size - CenterLocal;
			const float RadiusX = FMath::Sqrt(FMath::Square(Delta.X) + FMath::Square(Delta.Y / Aspect));
			if (RadiusX < 1.0f)
			{
				continue;
			}
			const float RadiusY = RadiusX * Aspect;

			const FLinearColor& Color = Orbit.bSelected ? SelectedColor : (Orbit.bLocked ? LockedColor : OrbitColor);
			const float LineThickness = Orbit.bSelected ? Thickness * 1.5f : Thickness;

			TArray<FVector2f> Points;
			for (int32 Step = 0; Step <= OrbitSegments; ++Step)
			{
				const float Angle = UE_TWO_PI * Step / OrbitSegments;
				Points.Add(CenterLocal + FVector2f(FMath::Cos(Angle) * RadiusX, FMath::Sin(Angle) * RadiusY));
			}

			if (!Orbit.bLocked || Orbit.bSelected)
			{
				FSlateDrawElement::MakeLines(OutDrawElements, OrbitLayer + Pass, AllottedGeometry.ToPaintGeometry(), MoveTemp(Points),
					ESlateDrawEffect::None, Color, true, LineThickness);
				continue;
			}

			// 잠긴 궤도: 한 조각 걸러 그리는 점선
			for (int32 Step = 0; Step < OrbitSegments; Step += 2)
			{
				TArray<FVector2f> Dash = { Points[Step], Points[Step + 1] };
				FSlateDrawElement::MakeLines(OutDrawElements, OrbitLayer, AllottedGeometry.ToPaintGeometry(), MoveTemp(Dash),
					ESlateDrawEffect::None, Color, true, LineThickness);
			}
		}
	}

	const int32 HubLayer = OrbitLayer + 2;
	if (HubTexture)
	{
		const FVector2f HubExtent(HubSize);
		FSlateDrawElement::MakeBox(OutDrawElements, HubLayer,
			AllottedGeometry.ToPaintGeometry(HubExtent, FSlateLayoutTransform(CenterLocal - HubExtent * 0.5f)),
			&HubBrush, ESlateDrawEffect::None, FLinearColor::White);
	}
	return HubLayer;
}
