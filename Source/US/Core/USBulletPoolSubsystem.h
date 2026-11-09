#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "USBulletPoolSubsystem.generated.h"

class AUSBullet;
class APawn;

/** 한 총알 클래스의 쉬고 있는 총알 목록. TMap 값으로 TArray를 UPROPERTY에 담으려면 구조체로 감싸야 한다 */
USTRUCT()
struct FUSBulletPool
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AUSBullet>> Inactive;
};

/**
 * 월드 단위 코스메틱 총알 풀.
 * 발사마다 SpawnActor/Destroy 하는 대신 쉬고 있는 총알을 꺼내 쓰고, 도착하면 돌려받는다.
 * 액터 생성 비용과, 초당 수십 개씩 버려진 액터를 GC가 한꺼번에 치울 때의 프레임 끊김을 없앤다.
 * 풀은 필요할 때 늘어난다 (미리 만들어 두지 않음).
 */
UCLASS()
class US_API UUSBulletPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 쉬는 총알을 꺼내 켜거나, 없으면 새로 만든다. 실패하면 nullptr */
	AUSBullet* AcquireBullet(TSubclassOf<AUSBullet> BulletClass, const FVector& Location, const FRotator& Rotation,
		AActor* Owner, APawn* Instigator);

	/** 총알을 끄고 풀에 돌려놓는다 */
	void ReleaseBullet(AUSBullet* Bullet);

private:
	/** 총알 클래스별 풀. 다른 총알 BP끼리 섞이지 않게 나눈다 */
	UPROPERTY()
	TMap<TSubclassOf<AUSBullet>, FUSBulletPool> Pools;
};
