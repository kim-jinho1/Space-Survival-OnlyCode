#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "USRadarWidget.generated.h"

class AUSSupplyShip;
class UImage;
class UMaterialInstanceDynamic;
class UWidget;

/**
 * 레이더형 미니맵. 대장을 가운데 두고 부하·적·보급선을 점으로 그린다 (화면 위쪽 = 카메라 앞쪽).
 * 점마다 위젯을 만들지 않고 NativePaint에서 사각형만 직접 그려 비용이 작다.
 * 위치는 팀 등록부(UUSTeamSubsystem)와 보급선 참조에서 읽으므로 액터 검색이 없다.
 * 원형이며, 배경은 바닥과 같은 격자 머티리얼이 대장 위치를 따라 흘러가 "바닥 위를 나는" 느낌을 준다.
 * 위젯 이름: RadarFrame — 화면 전체 캔버스 안에서 레이더 자리(크기·위치)를 잡는 위젯. 점은 이 영역 안에 그린다.
 *           BackgroundImage — 격자 머티리얼(GridOffset·RadarRange 파라미터)을 쓰는 이미지.
 */
UCLASS()
class US_API UUSRadarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 표시할 보급선 (없으면 보급선 점을 그리지 않는다) */
	void SetSupplyShip(AUSSupplyShip* InSupplyShip);

	/** 보스 (보스전 동안만, nullptr이면 그리지 않는다). 크게 깜빡이는 점으로 그리고 범위 밖이면 가장자리에 붙인다 */
	void SetBoss(AActor* InBoss);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	/** 레이더 자리 (WBP에서 크기·위치를 잡는다) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> RadarFrame;

	/** 바닥 격자 배경 (머티리얼 파라미터를 C++이 매 프레임 넘긴다) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> BackgroundImage;

	/** 레이더 가장자리까지의 월드 거리 (cm). 이보다 먼 적은 그리지 않고, 보급선은 가장자리에 붙여 그린다 */
	UPROPERTY(EditDefaultsOnly, Category = "Radar", meta = (ClampMin = "100.0"))
	float RadarRange = 3000.0f;

	/** 가장자리에 붙는 점(보급선)과 원 밖으로 빼는 점의 기준을 테두리에서 이만큼 안쪽으로 둔다 (화면 좌표) */
	UPROPERTY(EditDefaultsOnly, Category = "Radar", meta = (ClampMin = "0.0"))
	float EdgePadding = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Radar|Style")
	FLinearColor LeaderColor = FLinearColor(0.0f, 0.9f, 1.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Radar|Style")
	FLinearColor AllyColor = FLinearColor(0.0f, 0.55f, 0.75f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Radar|Style")
	FLinearColor EnemyColor = FLinearColor(1.0f, 0.15f, 0.1f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Radar|Style")
	FLinearColor SupplyShipColor = FLinearColor(0.2f, 1.0f, 0.35f, 1.0f);

	/** 점 크기 (화면 좌표) */
	UPROPERTY(EditDefaultsOnly, Category = "Radar|Style")
	float LeaderDotSize = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Radar|Style")
	float UnitDotSize = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Radar|Style")
	float SupplyShipDotSize = 10.0f;

	/** 보스 점 (일반 적과 다른 주황, 크게) */
	UPROPERTY(EditDefaultsOnly, Category = "Radar|Style")
	FLinearColor BossColor = FLinearColor(1.0f, 0.55f, 0.1f, 1.0f);

	/** 보스 점 크기 (깜빡이며 이 크기와 0.7배 사이를 오간다) */
	UPROPERTY(EditDefaultsOnly, Category = "Radar|Style")
	float BossDotSize = 16.0f;

private:
	/** 대장 기준 월드 오프셋 → 레이더 중심 기준 화면 오프셋 (카메라 yaw로 돌린다). bClampToEdge면 범위 밖도 가장자리에 붙인다 */
	bool ToRadarOffset(const FVector& WorldOffset, float CameraYaw, float HalfSize, bool bClampToEdge, FVector2D& OutOffset) const;

	/** 중심 기준 오프셋 위치에 정사각형 점 하나 */
	static void DrawDot(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry,
		const FVector2D& Center, const FVector2D& Offset, float Size, const FLinearColor& Color);

	/** 보급선 (파괴되지 않지만 레벨이 바뀌면 사라지므로 약한 참조) */
	TWeakObjectPtr<AUSSupplyShip> SupplyShip;
	TWeakObjectPtr<AActor> Boss;

	/** 배경 격자 머티리얼 인스턴스 (NativeConstruct에서 한 번 만든다) */
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BackgroundMaterial;
};
