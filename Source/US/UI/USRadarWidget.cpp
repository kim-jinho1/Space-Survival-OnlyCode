#include "UI/USRadarWidget.h"
#include "Core/USTeamSubsystem.h"
#include "Shop/USSupplyShip.h"
#include "US.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
	/** 레이더 배경 머티리얼 파라미터 이름 (M_USRadarBackground) */
	const FName GridOffsetParam(TEXT("GridOffset"));
	const FName RadarRangeParam(TEXT("RadarRange"));
}

void UUSRadarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BackgroundImage == nullptr)
	{
		return;
	}

	// 브러시의 머티리얼로 동적 인스턴스를 한 번 만들어 둔다. 점과 격자가 같은 범위를 쓰도록 범위도 여기서 맞춘다
	BackgroundMaterial = BackgroundImage->GetDynamicMaterial();
	if (BackgroundMaterial == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: BackgroundImage has no material; radar floor grid will not move"), *GetName());
		return;
	}

	BackgroundMaterial->SetScalarParameterValue(RadarRangeParam, RadarRange);
}

void UUSRadarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const APlayerController* PlayerController = GetOwningPlayer();
	const APawn* Leader = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (BackgroundMaterial == nullptr || Leader == nullptr)
	{
		return;
	}

	// 격자를 대장 위치만큼 밀어 바닥 격자와 같이 흘러가게 한다 (파라미터 하나라 매 프레임 넘겨도 비용이 작다).
	// 카메라 yaw가 고정(0)인 쿼터뷰라 격자는 회전시키지 않는다. 카메라를 돌리게 되면 회전 파라미터가 필요하다
	const FVector LeaderLocation = Leader->GetActorLocation();
	BackgroundMaterial->SetVectorParameterValue(GridOffsetParam, FLinearColor(LeaderLocation.X, LeaderLocation.Y, 0.0f, 0.0f));
}

void UUSRadarWidget::SetBoss(AActor* InBoss)
{
	Boss = InBoss;
}

void UUSRadarWidget::SetSupplyShip(AUSSupplyShip* InSupplyShip)
{
	SupplyShip = InSupplyShip;
}

int32 UUSRadarWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// 배경(WBP 이미지)을 먼저 그리고 그 위 층에 점을 그린다
	int32 MaxLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const APlayerController* PlayerController = GetOwningPlayer();
	const APawn* Leader = PlayerController ? PlayerController->GetPawn() : nullptr;
	const UWorld* World = GetWorld();
	if (Leader == nullptr || World == nullptr || RadarFrame == nullptr)
	{
		return MaxLayer;
	}

	// 레이더 자리의 크기·중심을 이 위젯 좌표로 바꾼다 (지난 프레임 배치 결과라 한 프레임 늦을 수 있지만 고정 위치라 문제없다)
	const FGeometry& FrameGeometry = RadarFrame->GetCachedGeometry();
	const FVector2D Size = FrameGeometry.GetLocalSize();
	const FVector2D Center = AllottedGeometry.AbsoluteToLocal(FrameGeometry.GetAbsolutePositionAtCoordinates(FVector2D(0.5f, 0.5f)));

	const int32 DotLayer = MaxLayer + 1;
	// 격자 머티리얼과 같은 축척: 원 반지름 = RadarRange
	const float HalfSize = FMath::Min(Size.X, Size.Y) * 0.5f;
	const FVector LeaderLocation = Leader->GetActorLocation();

	// 쿼터뷰 카메라 기준으로 돌려야 화면에서 보는 방향과 레이더 방향이 같다
	const float CameraYaw = PlayerController->PlayerCameraManager ? PlayerController->PlayerCameraManager->GetCameraRotation().Yaw : 0.0f;

	FVector2D Offset;

	// 적 → 부하 → 보급선 → 대장 순서로 그려 중요한 점이 위에 오게 한다
	if (const UUSTeamSubsystem* Team = World->GetSubsystem<UUSTeamSubsystem>())
	{
		for (const TObjectPtr<AActor>& Enemy : Team->GetEnemies())
		{
			if (IsValid(Enemy) && ToRadarOffset(Enemy->GetActorLocation() - LeaderLocation, CameraYaw, HalfSize, false, Offset))
			{
				DrawDot(OutDrawElements, DotLayer, AllottedGeometry, Center, Offset, UnitDotSize, EnemyColor);
			}
		}

		for (const TObjectPtr<AActor>& Ally : Team->GetAllies())
		{
			// 대장도 아군 등록부에 있으므로 따로 그리기 위해 뺀다
			if (IsValid(Ally) && Ally != Leader
				&& ToRadarOffset(Ally->GetActorLocation() - LeaderLocation, CameraYaw, HalfSize, false, Offset))
			{
				DrawDot(OutDrawElements, DotLayer, AllottedGeometry, Center, Offset, UnitDotSize, AllyColor);
			}
		}
	}

	// 보급선은 멀어도 방향을 알 수 있게 가장자리에 붙여 그린다. 떠나 있을 때는 그리지 않는다
	const AUSSupplyShip* Ship = SupplyShip.Get();
	if (Ship && Ship->IsAvailable() && ToRadarOffset(Ship->GetActorLocation() - LeaderLocation, CameraYaw, HalfSize, true, Offset))
	{
		DrawDot(OutDrawElements, DotLayer, AllottedGeometry, Center, Offset, SupplyShipDotSize, SupplyShipColor);
	}

	// 보스는 적 점 위에 크게, 범위 밖이어도 가장자리에 붙여 방향을 보인다. 크기를 맥박처럼 바꿔 다른 점과 구분한다
	const AActor* BossActor = Boss.Get();
	if (BossActor && ToRadarOffset(BossActor->GetActorLocation() - LeaderLocation, CameraYaw, HalfSize, true, Offset))
	{
		const float Pulse = FMath::Sin(static_cast<float>(World->GetRealTimeSeconds()) * 2.0f * PI * 1.5f) * 0.5f + 0.5f;
		DrawDot(OutDrawElements, DotLayer, AllottedGeometry, Center, Offset, BossDotSize * FMath::Lerp(0.7f, 1.0f, Pulse), BossColor);
	}

	DrawDot(OutDrawElements, DotLayer, AllottedGeometry, Center, FVector2D::ZeroVector, LeaderDotSize, LeaderColor);

	return DotLayer;
}

bool UUSRadarWidget::ToRadarOffset(const FVector& WorldOffset, float CameraYaw, float HalfSize, bool bClampToEdge, FVector2D& OutOffset) const
{
	// 카메라 yaw만큼 되돌리면 X = 카메라 앞, Y = 카메라 오른쪽. 레이더에서는 앞이 위(-Y), 오른쪽이 오른쪽(+X)
	const FVector Local = FRotator(0.0f, -CameraYaw, 0.0f).RotateVector(FVector(WorldOffset.X, WorldOffset.Y, 0.0f));
	FVector2D Radar(Local.Y, -Local.X);
	Radar *= HalfSize / RadarRange;

	// 원형 레이더라 중심에서의 거리로 범위를 본다
	// 가장자리에 붙일 때는 테두리 선에 가리지 않게 여백만큼 안쪽에 둔다
	const float Overflow = Radar.Size() / FMath::Max(HalfSize - EdgePadding, 1.0f);
	if (Overflow > 1.0f)
	{
		if (!bClampToEdge)
		{
			return false;
		}
		Radar /= Overflow;
	}

	OutOffset = Radar;
	return true;
}

void UUSRadarWidget::DrawDot(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry,
	const FVector2D& Center, const FVector2D& Offset, float Size, const FLinearColor& Color)
{
	const FVector2D TopLeft = Center + Offset - FVector2D(Size * 0.5f);
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		Geometry.ToPaintGeometry(FVector2f(Size, Size), FSlateLayoutTransform(FVector2f(TopLeft))),
		FCoreStyle::Get().GetBrush("GenericWhiteBox"), ESlateDrawEffect::None, Color);
}
