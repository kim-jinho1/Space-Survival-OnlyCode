#include "Core/USBulletPoolSubsystem.h"
#include "US.h"
#include "Equipments/USBullet.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

AUSBullet* UUSBulletPoolSubsystem::AcquireBullet(TSubclassOf<AUSBullet> BulletClass, const FVector& Location,
	const FRotator& Rotation, AActor* Owner, APawn* Instigator)
{
	if (BulletClass == nullptr)
	{
		return nullptr;
	}

	if (FUSBulletPool* Pool = Pools.Find(BulletClass))
	{
		// 레벨 전환 등으로 이미 파괴된 총알이 남아 있을 수 있어 유효한 것이 나올 때까지 버린다
		while (!Pool->Inactive.IsEmpty())
		{
			AUSBullet* Bullet = Pool->Inactive.Pop(EAllowShrinking::No);
			if (IsValid(Bullet))
			{
				Bullet->SetActorLocationAndRotation(Location, Rotation, false, nullptr, ETeleportType::TeleportPhysics);
				Bullet->SetOwner(Owner);
				Bullet->SetInstigator(Instigator);
				Bullet->ActivateFromPool();
				return Bullet;
			}
		}
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = Owner;
	SpawnParams.Instigator = Instigator;

	AUSBullet* Bullet = World->SpawnActor<AUSBullet>(BulletClass, Location, Rotation, SpawnParams);
	if (Bullet == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSBulletPoolSubsystem: failed to spawn %s"), *GetNameSafe(BulletClass));
	}
	return Bullet;
}

void UUSBulletPoolSubsystem::ReleaseBullet(AUSBullet* Bullet)
{
	if (!IsValid(Bullet) || Bullet->IsInPool())
	{
		return;
	}

	Bullet->DeactivateToPool();
	Pools.FindOrAdd(Bullet->GetClass()).Inactive.Add(Bullet);
}
