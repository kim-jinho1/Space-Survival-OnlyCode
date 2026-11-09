#include "Equipments/USEquipment.h"
#include "US.h"
#include "Agents/USAgent.h"
#include "GameFramework/Character.h"

AUSEquipment::AUSEquipment()
{
	// 매 프레임 할 일이 없다. BP에서 Event Tick을 쓰면 BP 컴파일러가 다시 켠다
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EquipmentMesh"));
	SetRootComponent(Mesh);

	Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

void AUSEquipment::BeginPlay()
{
	Super::BeginPlay();

	// 스폰 직후에도 메시가 확실히 적용되도록 한 번 더 반영
	if (Mesh && MeshAsset)
	{
		Mesh->SetStaticMesh(MeshAsset);
	}
}

void AUSEquipment::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 에디터 프리뷰 및 스폰 시 MeshAsset을 실제 메시 컴포넌트에 적용
	if (Mesh && MeshAsset)
	{
		Mesh->SetStaticMesh(MeshAsset);
	}
}

void AUSEquipment::EquipItem()
{

}

void AUSEquipment::UnquipItem()
{

}

void AUSEquipment::AttachToOwner(const FName SocketName)
{
	// 어디에 붙을지는 소유자가 정한다 (기본 = 스켈레탈 메시 소켓, 드론 = WeaponMount)
	const AUSAgent* OwnerAgent = Cast<AUSAgent>(GetOwner());
	if (OwnerAgent == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("AUSEquipment::AttachToOwner: owner %s is not an AUSAgent"), *GetNameSafe(GetOwner()));
		return;
	}

	USceneComponent* AttachParent = OwnerAgent->GetEquipmentAttachParent();
	if (AttachParent == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("AUSEquipment::AttachToOwner: %s has no equipment attach parent"), *GetNameSafe(OwnerAgent));
		return;
	}

	// 위치/회전은 소켓에 스냅, 스케일은 무기 자체 상대 스케일을 유지(소켓 스케일에 덮이지 않게)
	const FAttachmentTransformRules Rules
	(
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::KeepRelative,
		true
	);

	// 소켓은 붙을 곳에 실제로 있을 때만 쓴다 (빈 기준점에는 소켓이 없다)
	const FName AttachSocket = AttachParent->DoesSocketExist(SocketName) ? SocketName : NAME_None;
	AttachToComponent(AttachParent, Rules, AttachSocket);
}
