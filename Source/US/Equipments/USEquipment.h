#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "USEquipment.generated.h"

UCLASS()
class US_API AUSEquipment : public AActor
{
	GENERATED_BODY()
	
public:	
	AUSEquipment();

protected:
	virtual void BeginPlay() override;

	/** MeshAsset을 Mesh 컴포넌트에 적용 (에디터/스폰 시) */
	virtual void OnConstruction(const FTransform& Transform) override;

public:
	FORCEINLINE UStaticMesh* GetMeshAsset() { return MeshAsset; }
	FORCEINLINE UTexture2D* GetEquipmentIcon() { return EquipmentIcon; }

public:
	virtual void EquipItem();
	virtual void UnquipItem();
	virtual void AttachToOwner(FName SocketName);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Equipment | Mesh")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, Category = "Equipment | Mesh")
	TObjectPtr<UStaticMesh> MeshAsset;

	UPROPERTY(EditAnywhere, Category = "Equipment | Icon")
	TObjectPtr<UTexture2D> EquipmentIcon;

};
