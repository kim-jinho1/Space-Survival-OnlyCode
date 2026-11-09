#include "Environment/USStageBackdrop.h"
#include "US.h"
#include "Components/StaticMeshComponent.h"
#include "Datas/USStageData.h"
#include "Engine/GameInstance.h"
#include "Materials/MaterialInterface.h"
#include "Progression/USMetaProgressionSubsystem.h"

AUSStageBackdrop::AUSStageBackdrop()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AUSStageBackdrop::BeginPlay()
{
	Super::BeginPlay();

	// 격납고 지역 지도에서 고른 스테이지 (전투 맵을 바로 열면 없음 → 미리보기 값)
	const UUSMetaProgressionSubsystem* Meta = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUSMetaProgressionSubsystem>() : nullptr;
	const UUSStageData* Stage = Meta ? Meta->GetSortieStage() : nullptr;

	PlacePlanet(Stage ? Stage->GetPlanetLocation() : PreviewPlanetLocation, Stage ? Stage->GetPlanetRadius() : PreviewPlanetRadius);

	UStaticMeshComponent* Plane = BackdropActor ? BackdropActor->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	if (Plane == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: BackdropActor has no static mesh. The backdrop stays as placed."), *GetName());
		return;
	}

	UMaterialInterface* Material = Stage ? Stage->GetBackdropMaterial() : PreviewMaterial.Get();
	if (Stage && Material == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("%s: stage %s has no BackdropMaterial. The backdrop stays as placed."), *GetName(), *Stage->GetName());
		return;
	}

	if (Material)
	{
		Plane->SetMaterial(0, Material);
	}
}

void AUSStageBackdrop::PlacePlanet(const FVector& Location, float Radius) const
{
	if (PlanetActor == nullptr)
	{
		return;
	}

	PlanetActor->SetActorHiddenInGame(Radius <= 0.0f);
	if (Radius > 0.0f)
	{
		PlanetActor->SetActorLocation(Location);
		PlanetActor->SetActorScale3D(FVector(Radius / PlanetMeshRadius));
	}
}
