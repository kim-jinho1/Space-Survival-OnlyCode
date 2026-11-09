#include "Components/USThrusterFXComponent.h"
#include "US.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

// Paragon 이펙트는 카메라 2500 이상에서 LOD 1로 내려가 거의 안 보인다. 카메라가 늘 그 근처라 LOD 0으로 고정한다.
void UUSThrusterFXComponent::ForceTopLOD(UParticleSystemComponent* Component)
{
	if (Component == nullptr)
	{
		return;
	}

	Component->bOverrideLODMethod = true;
	Component->LODMethod = PARTICLESYSTEMLODMETHOD_DirectSet;
	Component->SetLODLevel(0);
}

UUSThrusterFXComponent::UUSThrusterFXComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// 이동이 끝난 뒤의 속도로 크기를 정해야 한 프레임 늦지 않는다.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UUSThrusterFXComponent::SetAttachParent(USceneComponent* InAttachParent)
{
	AttachParent = InAttachParent;
	if (AttachParent == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSThrusterFXComponent: null attach parent on %s"), *GetNameSafe(GetOwner()));
	}
}

void UUSThrusterFXComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AttachParent == nullptr || ThrusterFX == nullptr || EngineOffsets.IsEmpty())
	{
		UE_LOG(LogUS, Warning, TEXT("UUSThrusterFXComponent: %s is missing attach parent, ThrusterFX or EngineOffsets; no thruster FX"),
			*GetNameSafe(GetOwner()));
		SetComponentTickEnabled(false);
		return;
	}

	CurrentScale = IdleScale;

	// 불꽃은 한 번만 만들어 계속 켜 둔다. 끄고 켜는 대신 크기만 바꾸면 매 프레임 생성·파괴 비용이 없다.
	ThrusterComponents.Reserve(EngineOffsets.Num());
	for (const FVector& Offset : EngineOffsets)
	{
		UParticleSystemComponent* Thruster = UGameplayStatics::SpawnEmitterAttached(ThrusterFX, AttachParent, NAME_None,
			Offset, ThrusterRotation, FVector(CurrentScale), EAttachLocation::KeepRelativeOffset, false);
		if (Thruster)
		{
			ForceTopLOD(Thruster);
			ThrusterComponents.Add(Thruster);
		}
	}

	// 궤적은 크기를 바꾸지 않는다. 멈추면 새로 그릴 거리가 없어 저절로 짧아지고, 빠를수록 길게 남는다.
	if (TrailFX)
	{
		TrailComponents.Reserve(TrailOffsets.Num());
		for (const FVector& Offset : TrailOffsets)
		{
			UParticleSystemComponent* Trail = UGameplayStatics::SpawnEmitterAttached(TrailFX, AttachParent, NAME_None,
				Offset, ThrusterRotation, FVector(TrailScale), EAttachLocation::KeepRelativeOffset, false);
			if (Trail)
			{
				ForceTopLOD(Trail);
				TrailComponents.Add(Trail);
			}
		}
	}
}

void UUSThrusterFXComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (Owner == nullptr || ThrusterComponents.IsEmpty())
	{
		return;
	}

	// 화면에 안 보이는 유닛은 크기를 계산하지 않는다 (HoverVisual과 같은 기준).
	if (!Owner->WasRecentlyRendered(RenderSkipTime))
	{
		return;
	}

	// 꼬리 불꽃은 앞으로 미는 주 엔진이라 몸 기준 전진 성분만 본다.
	// 후진·옆 이동은 호버로 미끄러지는 것으로 보고 대기 불꽃만 낸다 (움직임은 HoverVisual의 기울기가 보여 준다).
	float SpeedRatio = 1.0f;
	if (ReferenceSpeed > 0.0f)
	{
		const FVector LocalVelocity = Owner->GetActorRotation().UnrotateVector(Owner->GetVelocity());
		SpeedRatio = FMath::Clamp(LocalVelocity.X / ReferenceSpeed, 0.0f, 1.0f);
	}
	const float TargetScale = FMath::Lerp(IdleScale, MaxScale, SpeedRatio) * SizeMultiplier;
	CurrentScale = FMath::FInterpTo(CurrentScale, TargetScale, DeltaTime, ScaleInterpSpeed);

	for (UParticleSystemComponent* Thruster : ThrusterComponents)
	{
		if (Thruster)
		{
			Thruster->SetRelativeScale3D(FVector(CurrentScale));
		}
	}
}

void UUSThrusterFXComponent::StopThrusters()
{
	// 즉시 지우지 않고 새 입자만 멈춰서 불꽃 꼬리가 자연스럽게 꺼지게 한다.
	for (UParticleSystemComponent* Thruster : ThrusterComponents)
	{
		if (Thruster)
		{
			Thruster->DeactivateSystem();
		}
	}
	for (UParticleSystemComponent* Trail : TrailComponents)
	{
		if (Trail)
		{
			Trail->DeactivateSystem();
		}
	}

	SetComponentTickEnabled(false);
}
