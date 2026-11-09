#include "Components/USTauntComponent.h"
#include "Components/USAttributeComponent.h"
#include "Core/USTeamSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UUSTauntComponent::UUSTauntComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UUSTauntComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UUSTeamSubsystem* Team = GetWorld() ? GetWorld()->GetSubsystem<UUSTeamSubsystem>() : nullptr)
	{
		Team->RegisterTaunter(GetOwner(), TauntRadius);
		bTaunting = true;
	}

	// 소유자 스탯 컴포넌트는 시작할 때 한 번만 찾는다
	if (UUSAttributeComponent* Attribute = GetOwner() ? GetOwner()->FindComponentByClass<UUSAttributeComponent>() : nullptr)
	{
		Attribute->OnDeath.AddUObject(this, &UUSTauntComponent::HandleOwnerDeath);
	}
}

void UUSTauntComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopTaunt();

	Super::EndPlay(EndPlayReason);
}

void UUSTauntComponent::HandleOwnerDeath()
{
	StopTaunt();
}

void UUSTauntComponent::SetTauntRadius(float NewRadius)
{
	TauntRadius = FMath::Max(0.0f, NewRadius);

	// 등록부는 반경을 값으로 들고 있으므로 다시 등록해야 바뀐다 (TMap::Add는 같은 키를 덮어쓴다)
	UUSTeamSubsystem* Team = GetWorld() ? GetWorld()->GetSubsystem<UUSTeamSubsystem>() : nullptr;
	if (bTaunting && Team)
	{
		Team->RegisterTaunter(GetOwner(), TauntRadius);
	}
}

void UUSTauntComponent::StopTaunt()
{
	bTaunting = false;

	if (UUSTeamSubsystem* Team = GetWorld() ? GetWorld()->GetSubsystem<UUSTeamSubsystem>() : nullptr)
	{
		Team->UnregisterTaunter(GetOwner());
	}
}
