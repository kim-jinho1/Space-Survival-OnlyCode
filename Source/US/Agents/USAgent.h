#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "USAgent.generated.h"

UCLASS()
class US_API AUSAgent : public ACharacter
{
	GENERATED_BODY()

public:
	AUSAgent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
public:
	FORCEINLINE class UUSAttributeComponent* GetAttributeComponent() const { return AttributeComponent; }

	/** 장비(무기)를 붙일 컴포넌트. 기본은 스켈레탈 메시(소켓 사용), 드론처럼 외형이 다르면 자식이 오버라이드한다 */
	virtual USceneComponent* GetEquipmentAttachParent() const;

	/**
	 * ShotIndex번째 발사의 포구 월드 위치. 포구가 여러 개면 순환해 번갈아 쏜다.
	 * 기본은 포구 정보가 없어 false — 무기가 자기 머즐 위치를 쓴다.
	 */
	virtual bool GetMuzzleWorldLocation(int32 ShotIndex, FVector& OutLocation) const;

protected:
	/** 사망 처리 (AttributeComponent::OnDeath에 바인딩) */
	virtual void HandleDeath() ;
	
protected:
	/** 체력/속성 관리 컴포넌트 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UUSAttributeComponent> AttributeComponent;
	
};
