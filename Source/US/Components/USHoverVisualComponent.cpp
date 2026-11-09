#include "Components/USHoverVisualComponent.h"
#include "US.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"

UUSHoverVisualComponent::UUSHoverVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// 이동이 끝난 뒤의 속도로 기울여야 한 프레임 늦는 흔들림이 없다.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UUSHoverVisualComponent::BeginPlay()
{
	Super::BeginPlay();

	BobPhase = FMath::FRandRange(0.0f, UE_TWO_PI);

	if (const AActor* Owner = GetOwner())
	{
		PreviousYaw = Owner->GetActorRotation().Yaw;
	}

	if (VisualTarget == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSHoverVisualComponent: %s has no visual target; call SetVisualTarget"), *GetNameSafe(GetOwner()));
	}
}

void UUSHoverVisualComponent::SetVisualTarget(USceneComponent* InVisualTarget)
{
	VisualTarget = InVisualTarget;
	if (VisualTarget == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSHoverVisualComponent: null visual target on %s"), *GetNameSafe(GetOwner()));
		return;
	}

	BaseRelativeTransform = VisualTarget->GetRelativeTransform();
}

void UUSHoverVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (VisualTarget == nullptr || Owner == nullptr)
	{
		return;
	}

	// 회전 속도는 화면 밖에서도 yaw를 갱신해 둬야 다시 보일 때 쌓인 회전이 한 번에 튀지 않는다.
	const float CurrentYaw = Owner->GetActorRotation().Yaw;
	const float YawRate = DeltaTime > UE_SMALL_NUMBER
		? FMath::FindDeltaAngleDegrees(PreviousYaw, CurrentYaw) / DeltaTime
		: 0.0f;
	PreviousYaw = CurrentYaw;

	// 화면에 안 보이는 유닛은 연출을 계산하지 않는다 (적·부하가 많아져도 비용이 화면 안 유닛 수에만 비례).
	if (!Owner->WasRecentlyRendered(RenderSkipTime))
	{
		return;
	}

	// 둥실: 위상을 누적해 주기를 바꿔도 튀지 않게 하고, 한 바퀴마다 되돌려 float 정밀도를 지킨다.
	BobPhase = FMath::Fmod(BobPhase + DeltaTime * BobFrequency * UE_TWO_PI, UE_TWO_PI);
	const float Bob = FMath::Sin(BobPhase) * BobAmplitude;

	// 기울기: 몸 기준 속도로 앞으로 가면 코를 숙이고(pitch −), 오른쪽으로 가면 오른쪽으로 기운다(roll +).
	FVector2D TargetTilt = FVector2D::ZeroVector;
	if (TiltReferenceSpeed > 0.0f)
	{
		const FVector LocalVelocity = Owner->GetActorRotation().UnrotateVector(Owner->GetVelocity());
		TargetTilt.X = FMath::Clamp(-LocalVelocity.X / TiltReferenceSpeed, -1.0f, 1.0f) * MaxPitchAngle;
		TargetTilt.Y = FMath::Clamp(LocalVelocity.Y / TiltReferenceSpeed, -1.0f, 1.0f) * MaxTiltAngle;
	}

	// 뱅킹: 오른쪽으로 선회하면(yaw +) 오른쪽으로 눕는다(roll +). 비행기처럼 회전 속도에 비례.
	TargetTilt.Y += FMath::Clamp(YawRate / BankReferenceYawRate, -1.0f, 1.0f) * MaxBankAngle;
	CurrentTilt.X = FMath::FInterpTo(CurrentTilt.X, TargetTilt.X, DeltaTime, TiltInterpSpeed);
	CurrentTilt.Y = FMath::FInterpTo(CurrentTilt.Y, TargetTilt.Y, DeltaTime, TiltInterpSpeed);

	// 기울기는 액터 공간에서 먼저 적용하고 BP에서 잡은 메시 기준 회전을 그 위에 얹는다 (모델 앞 방향이 달라도 기울기 방향은 액터 기준).
	const FQuat TiltQuat = FRotator(CurrentTilt.X, 0.0f, CurrentTilt.Y).Quaternion();
	const FVector Location = BaseRelativeTransform.GetLocation() + FVector(0.0f, 0.0f, HoverHeight + Bob);
	const FQuat Rotation = TiltQuat * BaseRelativeTransform.GetRotation();

	// 스케일은 건드리지 않는다: 위치·회전만 갱신.
	VisualTarget->SetRelativeLocationAndRotation(Location, Rotation);
}
