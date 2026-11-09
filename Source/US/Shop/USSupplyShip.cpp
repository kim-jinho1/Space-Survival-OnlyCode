#include "Shop/USSupplyShip.h"
#include "US.h"
#include "Agents/USPlayerController.h"
#include "Components/USHoverVisualComponent.h"
#include "Components/USRingPulsePoolComponent.h"
#include "Core/USLocationUtils.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UI/USSupplyMarkerWidget.h"
#include "UObject/ConstructorHelpers.h"

AUSSupplyShip::AUSSupplyShip()
{
	// 워프 연출(0.35초) 동안만 켠다
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	SetRootComponent(InteractionSphere);
	InteractionSphere->InitSphereRadius(InteractRadius);
	// 상호작용은 거리로 판정하므로 충돌·겹침 이벤트가 필요 없다
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionSphere->SetGenerateOverlapEvents(false);

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(InteractionSphere);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(VisualRoot);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetCanEverAffectNavigation(false);

	// 고리 판은 엔진 기본 평면(100×100). 재질(가산·발광 고리)은 BP에서 정한다
	RangeRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RangeRing"));
	RangeRing->SetupAttachment(InteractionSphere);
	RangeRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RangeRing->SetCastShadow(false);
	RangeRing->SetCanEverAffectNavigation(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneFinder.Succeeded())
	{
		RangeRing->SetStaticMesh(PlaneFinder.Object);
	}

	// 화면 공간이라 카메라 각도와 상관없이 글자가 똑바로 보이고, 거리와 상관없이 크기가 같다
	MarkerWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("MarkerWidget"));
	MarkerWidget->SetupAttachment(InteractionSphere);
	MarkerWidget->SetWidgetSpace(EWidgetSpace::Screen);
	MarkerWidget->SetDrawAtDesiredSize(true);
	MarkerWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	WarpPulse = CreateDefaultSubobject<UUSRingPulsePoolComponent>(TEXT("WarpPulse"));
	WarpPulse->SetupAttachment(InteractionSphere);
	// 도착·출발 고리는 겹칠 일이 거의 없다
	WarpPulse->SetPoolSize(2);

	HoverVisual = CreateDefaultSubobject<UUSHoverVisualComponent>(TEXT("HoverVisual"));
}

void AUSSupplyShip::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 컴포넌트 BeginPlay가 액터 BeginPlay의 Super 안에서 먼저 돌기 때문에 대상 연결은 여기서 한다 (드론과 같은 이유)
	if (HoverVisual)
	{
		HoverVisual->SetVisualTarget(VisualRoot);
	}

	// 생성자는 C++ 기본값으로 실행되므로 BP에서 바꾼 반경은 여기서 반영한다
	if (InteractionSphere)
	{
		InteractionSphere->SetSphereRadius(InteractRadius);
	}

	// 고리 지름 = 상호작용 거리 × 2, 높이는 바닥(기체 중심에서 띄운 만큼 아래) 바로 위
	if (RangeRing)
	{
		const float RingScale = InteractRadius * 2.0f / 100.0f;
		RangeRing->SetRelativeLocation(FVector(0.0f, 0.0f, -FloorHeightOffset + 2.0f));
		RangeRing->SetRelativeScale3D(FVector(RingScale, RingScale, 1.0f));
	}

	if (MarkerWidget)
	{
		MarkerWidget->SetRelativeLocation(FVector(0.0f, 0.0f, MarkerHeight));
	}
}

void AUSSupplyShip::BeginPlay()
{
	Super::BeginPlay();

	// 위젯 컴포넌트는 자기 BeginPlay(위의 Super 안)에서 위젯을 만든다 → 여기서 보급선을 알려 준다
	if (MarkerWidget)
	{
		if (UUSSupplyMarkerWidget* Marker = Cast<UUSSupplyMarkerWidget>(MarkerWidget->GetUserWidgetObject()))
		{
			Marker->SetSupplyShip(this);
		}
		else
		{
			UE_LOG(LogUS, Warning, TEXT("%s: MarkerWidget has no UUSSupplyMarkerWidget. No marker above the supply ship."), *GetName());
		}
	}

	// 판 시작 직후에는 골드가 없어 살 것이 없으므로, 첫 도착을 늦춰 골드를 모은 뒤 오게 한다 (짧은 판 구조, D6)
	if (FirstArrivalDelay > 0.0f)
	{
		// 시작 상태(Present)에서 넘어가면 "떠나는" 워프 연출이 판 시작 화면에 보이므로, 처음부터 떠나 있던 것으로 둔다
		State = EUSSupplyShipState::Absent;
		EnterState(EUSSupplyShipState::Absent, FMath::Max(0.0f, FirstArrivalDelay - GetNoticeLeadTime()));
		return;
	}

	// 지연이 없으면 맵에 배치된 자리(시작 지점 앞)에서 바로 머무는 상태로 시작한다
	EnterState(EUSSupplyShipState::Present, PresentDuration - GetNoticeLeadTime());
}

void AUSSupplyShip::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StateTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

float AUSSupplyShip::GetNoticeLeadTime() const
{
	// 예고가 머무는·떠나 있는 시간보다 길면 순서가 꼬이므로 짧은 쪽의 절반으로 제한한다
	return FMath::Min(NoticeLeadTime, FMath::Min(PresentDuration, AbsentDuration) * 0.5f);
}

void AUSSupplyShip::EnterState(EUSSupplyShipState NewState, float Duration)
{
	const bool bWasAvailable = IsAvailable();
	State = NewState;

	// 떠나 있을 때는 숨긴다. 숨겨도 액터는 남아 있어 자리 이동·타이머가 그대로 돈다.
	// 막 떠나는 순간은 사라짐 연출이 끝난 뒤 숨긴다 (상점은 상태가 바뀐 지금부터 못 쓴다)
	if (bWasAvailable != IsAvailable() && HasActorBegunPlay())
	{
		PlayWarp(IsAvailable());
	}
	else if (!IsActorTickEnabled())
	{
		SetActorHiddenInGame(!IsAvailable());
	}
	// 화면 공간 위젯은 액터 숨김을 따르지 않으므로 직접 끈다
	if (MarkerWidget)
	{
		MarkerWidget->SetVisibility(IsAvailable());
	}

	OnSupplyStateChanged.Broadcast(State, Duration);

	if (UWorld* World = GetWorld())
	{
		// 0초 타이머는 걸리지 않으므로 아주 짧은 시간으로 대신한다
		World->GetTimerManager().SetTimer(StateTimerHandle, this, &AUSSupplyShip::AdvanceState,
			FMath::Max(Duration, KINDA_SMALL_NUMBER), false);
	}
}

void AUSSupplyShip::AdvanceState()
{
	const float LeadTime = GetNoticeLeadTime();

	switch (State)
	{
	case EUSSupplyShipState::Present:
		EnterState(EUSSupplyShipState::Departing, LeadTime);
		break;

	case EUSSupplyShipState::Departing:
		EnterState(EUSSupplyShipState::Absent, AbsentDuration - LeadTime);
		break;

	case EUSSupplyShipState::Absent:
		EnterState(EUSSupplyShipState::Arriving, LeadTime);
		break;

	case EUSSupplyShipState::Arriving:
		// 도착 순간의 대장 위치 기준으로 자리를 잡아야 방향 표시기가 가리키는 곳이 의미 있다
		Relocate();
		EnterState(EUSSupplyShipState::Present, PresentDuration - LeadTime);
		break;

	default:
		break;
	}
}

void AUSSupplyShip::PlayWarp(bool bIn)
{
	bWarpIn = bIn;
	WarpElapsed = 0.0f;

	if (WarpPulse && WarpPulse->HasMaterial())
	{
		WarpPulse->PlayPulse(GetActorLocation() - FVector(0.0f, 0.0f, FloorHeightOffset - 2.0f), InteractRadius * WarpPulseRadiusScale);
	}

	if (WarpDuration <= 0.0f)
	{
		SetActorHiddenInGame(!bIn);
		return;
	}

	SetActorHiddenInGame(false);
	if (VisualRoot)
	{
		VisualRoot->SetRelativeScale3D(FVector(bIn ? 0.0f : 1.0f));
	}
	SetActorTickEnabled(true);
}

void AUSSupplyShip::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	WarpElapsed += DeltaSeconds;
	const float Alpha = FMath::Clamp(WarpElapsed / FMath::Max(WarpDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);

	// 나타날 때는 살짝 넘쳤다 돌아오고(ease-out back), 사라질 때는 점점 빨리 줄어든다 — 워프 느낌
	float Scale;
	if (bWarpIn)
	{
		constexpr float Overshoot = 1.7f;
		const float T = Alpha - 1.0f;
		Scale = 1.0f + (Overshoot + 1.0f) * T * T * T + Overshoot * T * T;
	}
	else
	{
		Scale = 1.0f - Alpha * Alpha;
	}
	if (VisualRoot)
	{
		VisualRoot->SetRelativeScale3D(FVector(FMath::Max(Scale, 0.0f)));
	}

	if (Alpha >= 1.0f)
	{
		SetActorTickEnabled(false);
		if (!bWarpIn)
		{
			SetActorHiddenInGame(true);
			if (VisualRoot)
			{
				VisualRoot->SetRelativeScale3D(FVector::OneVector);
			}
		}
	}
}

float AUSSupplyShip::GetStayTimeRemaining() const
{
	const UWorld* World = GetWorld();
	if (World == nullptr || !IsAvailable())
	{
		return 0.0f;
	}

	// 머무는 중이면 이번 타이머 뒤에 출발 예고 시간이 더 남아 있다
	const float TimerRemaining = FMath::Max(0.0f, World->GetTimerManager().GetTimerRemaining(StateTimerHandle));
	return State == EUSSupplyShipState::Present ? TimerRemaining + GetNoticeLeadTime() : TimerRemaining;
}

bool AUSSupplyShip::IsAvailable() const
{
	return State == EUSSupplyShipState::Present || State == EUSSupplyShipState::Departing;
}

void AUSSupplyShip::Interact(APawn* InteractingPawn)
{
	if (InteractingPawn == nullptr || !IsAvailable())
	{
		return;
	}

	if (AUSPlayerController* PC = Cast<AUSPlayerController>(InteractingPawn->GetController()))
	{
		PC->RequestOpenShop();
	}
}

bool AUSSupplyShip::IsInInteractRange(const FVector& Location) const
{
	return IsAvailable() && FVector::DistSquared2D(GetActorLocation(), Location) <= FMath::Square(InteractRadius);
}

void AUSSupplyShip::Relocate()
{
	const APawn* Leader = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Leader == nullptr)
	{
		return;
	}

	FVector NewLocation;
	if (!USLocationUtils::TryFindFloorLocationInRing(GetWorld(), Leader->GetActorLocation(), RelocateRadiusMin, RelocateRadiusMax,
		FloorHeightOffset, NewLocation))
	{
		UE_LOG(LogUS, Warning, TEXT("%s: no floor location around the leader after %d attempts; staying in place"),
			*GetName(), USLocationUtils::MaxRingAttempts);
		return;
	}

	SetActorLocation(NewLocation);
}
