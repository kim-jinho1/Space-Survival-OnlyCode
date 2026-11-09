#include "Components/USStageTransitionComponent.h"
#include "US.h"
#include "Agents/USAllyDroneCharacter.h"
#include "Agents/USPlayerCharacter.h"
#include "Components/USSquadComponent.h"
#include "Core/USCameraShake.h"
#include "Environment/USSpaceDust.h"
#include "UI/USStageBannerWidget.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** 0 → 1을 끝으로 갈수록 빠르게 / 느리게 */
	float EaseIn(float Alpha) { return Alpha * Alpha; }
	float EaseOut(float Alpha) { return 1.0f - FMath::Square(1.0f - Alpha); }
}

UUSStageTransitionComponent::UUSStageTransitionComponent()
{
	// 연출할 때만 켠다 (BeginSequence ~ FinishSequence)
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// 프로젝트에 있는 휙 소리 (Lyra 점프 패드). BP에서 바꿀 수 있다
	static ConstructorHelpers::FObjectFinder<USoundBase> JumpFinder(TEXT("/Game/Audio/Sounds/SFX/Lyra_JumpPad_Whoosh_01.Lyra_JumpPad_Whoosh_01"));
	if (JumpFinder.Succeeded())
	{
		JumpSound = JumpFinder.Object;
	}
	static ConstructorHelpers::FObjectFinder<USoundBase> ArrivalFinder(TEXT("/Game/Audio/Sounds/SFX/Lyra_JumpPad_Whoosh_02.Lyra_JumpPad_Whoosh_02"));
	if (ArrivalFinder.Succeeded())
	{
		ArrivalSound = ArrivalFinder.Object;
	}
}

void UUSStageTransitionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	StepSequence();
}

void UUSStageTransitionComponent::SetBanner(UUSStageBannerWidget* InBanner)
{
	Banner = InBanner;
}

void UUSStageTransitionComponent::PlayDeparture(AUSPlayerCharacter* Leader, FDelegateOnTransitionFinished OnFinished)
{
	if (!BeginSequence(Leader, OnFinished))
	{
		return;
	}

	EnterPhase(EPhase::Charge);
}

void UUSStageTransitionComponent::PlayArrival(AUSPlayerCharacter* Leader, FDelegateOnTransitionFinished OnFinished)
{
	if (!BeginSequence(Leader, OnFinished))
	{
		return;
	}

	// 카메라는 BeginSequence가 도착 자리에서 세웠다 → 이제 편대를 화면 아래 밖으로 빼고 늘인 채 기다린다
	for (int32 Index = 0; Index < Flyers.Num(); ++Index)
	{
		PlaceFlyer(Index, -ArrivalDistance, ShipStreakStretch, DirectionYaw);
	}
	ApplyWarpLook(DustStreakStretch, FovOffset);

	EnterPhase(EPhase::ArrivalWait);
}

float UUSStageTransitionComponent::GetArrivalTotalSeconds() const
{
	return ArrivalDelay + ArrivalBeamSeconds + ArrivalSettleSeconds;
}

bool UUSStageTransitionComponent::BeginSequence(AUSPlayerCharacter* Leader, FDelegateOnTransitionFinished OnFinished)
{
	UWorld* World = GetWorld();
	if (Leader == nullptr || World == nullptr || IsPlaying())
	{
		UE_LOG(LogUS, Warning, TEXT("[%s] Warp skipped (leader %s, already playing %d)."), *GetNameSafe(GetOwner()), *GetNameSafe(Leader), IsPlaying() ? 1 : 0);
		OnFinished.ExecuteIfBound();
		return false;
	}

	PendingFinished = OnFinished;
	FlyingLeader = Leader;
	Direction = Leader->GetScreenUpDirection();
	DirectionYaw = Direction.Rotation().Yaw;
	Dust = FindDust();

	Flyers.Reset();
	BaseLocations.Reset();
	StartYaws.Reset();
	BaseScales.Reset();
	auto AddFlyer = [this](AActor* Flyer)
	{
		Flyers.Add(Flyer);
		BaseLocations.Add(Flyer->GetActorLocation());
		StartYaws.Add(Flyer->GetActorRotation().Yaw);
		BaseScales.Add(Flyer->GetActorScale3D());
	};
	AddFlyer(Leader);
	if (const UUSSquadComponent* Squad = Leader->GetSquadComponent())
	{
		for (const FUSSquadMember& Member : Squad->GetMembers())
		{
			if (Member.IsActive() && Member.Ally)
			{
				AddFlyer(Member.Ally);
			}
		}
	}

	if (AController* Controller = Leader->GetController())
	{
		Controller->SetIgnoreMoveInput(true);
	}
	Leader->SetCameraFrozen(true);

	SetComponentTickEnabled(true);
	return true;
}

AUSSpaceDust* UUSStageTransitionComponent::FindDust()
{
	// 연출 시작 때 한 번만 찾는다 (맵에 한 개)
	for (TActorIterator<AUSSpaceDust> It(GetWorld()); It; ++It)
	{
		return *It;
	}
	UE_LOG(LogUS, Warning, TEXT("[%s] No AUSSpaceDust in the level. Star streaks are skipped."), *GetNameSafe(GetOwner()));
	return nullptr;
}

float UUSStageTransitionComponent::GetPhaseSeconds(EPhase InPhase) const
{
	switch (InPhase)
	{
	case EPhase::Charge: return ChargeSeconds;
	case EPhase::Streak: return StreakSeconds;
	case EPhase::Jump: return JumpSeconds;
	case EPhase::Tail: return TailSeconds;
	case EPhase::ArrivalWait: return ArrivalDelay;
	case EPhase::ArrivalBeam: return ArrivalBeamSeconds;
	case EPhase::ArrivalSettle: return ArrivalSettleSeconds;
	default: return 0.0f;
	}
}

void UUSStageTransitionComponent::EnterPhase(EPhase NewPhase)
{
	Phase = NewPhase;
	PhaseStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	NextShakeTime = 0.0f;

	switch (NewPhase)
	{
	case EPhase::Jump:
		// 섬광은 점프 동안 밝아지고 대기(Tail) 끝 전에 걷힌다 (결산 창이 게임을 멈추면 섬광도 그 자리에서 멈추므로)
		if (UUSStageBannerWidget* Flash = Banner.Get())
		{
			Flash->ShowFlash(JumpFlashOpacity, JumpSeconds * 0.6f, JumpSeconds * 0.4f + TailSeconds * 0.8f);
		}
		if (JumpSound)
		{
			UGameplayStatics::PlaySound2D(this, JumpSound);
		}
		break;
	case EPhase::ArrivalBeam:
		if (UUSStageBannerWidget* Flash = Banner.Get())
		{
			Flash->ShowFlash(ArrivalFlashOpacity, 0.05f, ArrivalBeamSeconds + 0.2f);
		}
		if (ArrivalSound)
		{
			UGameplayStatics::PlaySound2D(this, ArrivalSound);
		}
		break;
	default:
		break;
	}
}

void UUSStageTransitionComponent::PlaceFlyer(int32 Index, float Offset, float Stretch, float Yaw) const
{
	AActor* Flyer = Flyers.IsValidIndex(Index) ? Flyers[Index].Get() : nullptr;
	if (Flyer == nullptr)
	{
		return;
	}

	Flyer->SetActorLocationAndRotation(BaseLocations[Index] + Direction * Offset, FRotator(0.0f, Yaw, 0.0f));
	// 기수가 진행 방향이므로 액터 X(앞)만 늘이면 진행 방향으로 길어진다
	const FVector& Base = BaseScales[Index];
	Flyer->SetActorScale3D(FVector(Base.X * Stretch, Base.Y, Base.Z));

	// 위치를 직접 옮기는 동안 이동 컴포넌트에 남은 속도·AI 이동이 엇나가게 밀지 않게
	if (const ACharacter* Character = Cast<ACharacter>(Flyer))
	{
		Character->GetCharacterMovement()->StopMovementImmediately();
	}
}

void UUSStageTransitionComponent::ApplyWarpLook(float DustStretch, float InFovOffset) const
{
	if (AUSSpaceDust* SpaceDust = Dust.Get())
	{
		SpaceDust->SetWarpStretch(DustStretch, Direction);
	}
	if (AUSPlayerCharacter* Leader = FlyingLeader.Get())
	{
		Leader->SetCameraFovOffset(InFovOffset);
	}
}

void UUSStageTransitionComponent::StepSequence()
{
	const UWorld* World = GetWorld();
	if (World == nullptr || Phase == EPhase::None)
	{
		return;
	}

	const float Elapsed = World->GetTimeSeconds() - PhaseStartTime;
	const float Duration = GetPhaseSeconds(Phase);
	const float Alpha = Duration > 0.0f ? FMath::Clamp(Elapsed / Duration, 0.0f, 1.0f) : 1.0f;

	switch (Phase)
	{
	case EPhase::Charge:
	{
		// 제자리에서 기수를 화면 위로 돌리고, 떨림이 점점 세진다
		for (int32 Index = 0; Index < Flyers.Num(); ++Index)
		{
			const float Yaw = FMath::Lerp(StartYaws[Index], StartYaws[Index] + FMath::FindDeltaAngleDegrees(StartYaws[Index], DirectionYaw), EaseOut(Alpha));
			PlaceFlyer(Index, 0.0f, 1.0f, Yaw);
		}
		ApplyWarpLook(FMath::Lerp(1.0f, DustChargeStretch, Alpha), 0.0f);
		if (Elapsed >= NextShakeTime)
		{
			NextShakeTime += ChargeShakeInterval;
			if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
			{
				PC->ClientStartCameraShake(UUSCameraShake::StaticClass(), FMath::Lerp(ChargeShakeScale.X, ChargeShakeScale.Y, Alpha));
			}
		}
		break;
	}
	case EPhase::Streak:
		for (int32 Index = 0; Index < Flyers.Num(); ++Index)
		{
			PlaceFlyer(Index, -PullBackDistance * EaseOut(Alpha), 1.0f, DirectionYaw);
		}
		ApplyWarpLook(FMath::Lerp(DustChargeStretch, DustStreakStretch, EaseIn(Alpha)), FovOffset * EaseOut(Alpha));
		break;
	case EPhase::Jump:
		// 웅크린 자리에서 화면 위 밖으로 제곱 가속, 동시에 빛줄기처럼 길어진다
		for (int32 Index = 0; Index < Flyers.Num(); ++Index)
		{
			PlaceFlyer(Index, -PullBackDistance + (DepartureDistance + PullBackDistance) * EaseIn(Alpha),
				FMath::Lerp(1.0f, ShipStreakStretch, Alpha), DirectionYaw);
		}
		break;
	case EPhase::ArrivalBeam:
		// 화면 아래 밖에서 날아와 제자리를 조금 지나친다. 빛줄기는 줄어들고 별도 짧아진다
		for (int32 Index = 0; Index < Flyers.Num(); ++Index)
		{
			PlaceFlyer(Index, FMath::Lerp(-ArrivalDistance, ArrivalOvershoot, EaseOut(Alpha)),
				FMath::Lerp(ShipStreakStretch, 1.5f, Alpha), DirectionYaw);
		}
		ApplyWarpLook(FMath::Lerp(DustStreakStretch, 3.0f, EaseOut(Alpha)), FovOffset);
		break;
	case EPhase::ArrivalSettle:
	{
		// 되밀리며 멈춘다: 지나친 만큼 부드럽게 돌아오고 원래 모양·시야로
		const float Smooth = FMath::SmoothStep(0.0f, 1.0f, Alpha);
		for (int32 Index = 0; Index < Flyers.Num(); ++Index)
		{
			PlaceFlyer(Index, ArrivalOvershoot * (1.0f - Smooth), FMath::Lerp(1.5f, 1.0f, EaseOut(Alpha)), DirectionYaw);
		}
		ApplyWarpLook(FMath::Lerp(3.0f, 1.0f, EaseOut(Alpha)), FovOffset * (1.0f - Smooth));
		break;
	}
	default:
		// Tail, ArrivalWait: 그대로 기다린다
		break;
	}

	if (Alpha < 1.0f)
	{
		return;
	}

	switch (Phase)
	{
	case EPhase::Charge: EnterPhase(EPhase::Streak); break;
	case EPhase::Streak: EnterPhase(EPhase::Jump); break;
	case EPhase::Jump: EnterPhase(EPhase::Tail); break;
	case EPhase::ArrivalWait: EnterPhase(EPhase::ArrivalBeam); break;
	case EPhase::ArrivalBeam: EnterPhase(EPhase::ArrivalSettle); break;
	default: FinishSequence(); break;
	}
}

void UUSStageTransitionComponent::FinishSequence()
{
	SetComponentTickEnabled(false);

	// 진입이 끝나면 모양·카메라·조작을 돌려준다. 이탈 뒤에는 결산·맵 이동이라 화면을 그대로 둔다
	const bool bArrival = Phase == EPhase::ArrivalSettle;
	Phase = EPhase::None;
	if (bArrival)
	{
		for (int32 Index = 0; Index < Flyers.Num(); ++Index)
		{
			PlaceFlyer(Index, 0.0f, 1.0f, DirectionYaw);
		}
		ApplyWarpLook(1.0f, 0.0f);
		if (AUSPlayerCharacter* Leader = FlyingLeader.Get())
		{
			Leader->SetCameraFrozen(false);
			if (AController* Controller = Leader->GetController())
			{
				Controller->SetIgnoreMoveInput(false);
			}
		}
	}

	Flyers.Reset();
	BaseLocations.Reset();
	StartYaws.Reset();
	BaseScales.Reset();

	FDelegateOnTransitionFinished Finished = PendingFinished;
	PendingFinished.Unbind();
	Finished.ExecuteIfBound();
}
