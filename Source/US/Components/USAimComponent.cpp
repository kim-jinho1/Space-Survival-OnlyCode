#include "Components/USAimComponent.h"
#include "US.h"
#include "Agents/USEnemyCharacter.h"
#include "Core/USLocationUtils.h"
#include "Core/USTeamSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

UUSAimComponent::UUSAimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// 입력 처리 뒤, 무기 발사·애님 갱신 전에 조준점이 준비되어야 하므로 PrePhysics 그룹을 유지한다.
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UUSAimComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("UUSAimComponent: owner %s is not an ACharacter"), *GetNameSafe(GetOwner()));
		return;
	}

	PlayerController = Cast<APlayerController>(OwnerCharacter->GetController());

	// 적이 최대 100마리라 가장 가까운 적 검색은 매 프레임이 아니라 짧은 타이머로 한다. 조준 방향만 매 프레임 표적을 따라간다
	if (bAutoAim)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(RetargetTimerHandle, this, &UUSAimComponent::RefreshAutoTarget, RetargetInterval, true);
		}
	}
}

void UUSAimComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RetargetTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UUSAimComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (OwnerCharacter == nullptr)
	{
		return;
	}

	// 빙의가 BeginPlay보다 늦게 올 수 있으므로 컨트롤러가 비어 있으면 매 프레임 다시 시도한다.
	if (PlayerController == nullptr)
	{
		PlayerController = Cast<APlayerController>(OwnerCharacter->GetController());
		if (PlayerController == nullptr)
		{
			bHasValidAim = false;
			return;
		}
	}

	if (bAutoAim)
	{
		UpdateAimFromTarget();
	}
	else
	{
		UpdateAimFromCursor();
	}
	UpdateOwnerRotation(DeltaTime);
}

AActor* UUSAimComponent::GetTarget() const
{
	// 수동 표적이 잠깐 쓸 수 없으면(화면 밖, 날아가는 탄으로 죽을 예정) 고정은 두고 자동 표적을 쏜다
	if (AActor* Manual = ManualTarget.Get(); IsTargetUsable(Manual))
	{
		return Manual;
	}
	return AutoTarget.Get();
}

void UUSAimComponent::SelectTargetUnderCursor()
{
	if (!bAutoAim)
	{
		return;
	}

	const UUSTeamSubsystem* Team = GetWorld() ? GetWorld()->GetSubsystem<UUSTeamSubsystem>() : nullptr;
	FVector CursorLocation = FVector::ZeroVector;
	AActor* Picked = nullptr;
	if (Team && TryGetCursorPlaneLocation(CursorLocation))
	{
		Picked = Team->FindEnemyAtPoint(CursorLocation, ClickPickRadius);
	}

	// 빈 곳 클릭 = 수동 표적 해제. 쓸 수 없는 표적(사거리 밖)도 고정하지 않는다
	ManualTarget = IsTargetUsable(Picked, true) ? Picked : nullptr;
	UE_LOG(LogUS, Verbose, TEXT("UUSAimComponent: manual target %s"), *GetNameSafe(ManualTarget.Get()));
	NotifyAimTargetIfChanged();
}

void UUSAimComponent::RefreshAutoTarget()
{
	// 수동 표적은 죽었을 때만 푼다 (빈 곳 클릭은 SelectTargetUnderCursor). 화면 밖·IsDoomed로 풀면 다시 돌아오지 않아 "자꾸 풀린다"
	const AUSEnemyCharacter* ManualEnemy = Cast<AUSEnemyCharacter>(ManualTarget.Get());
	if (ManualEnemy == nullptr || !ManualEnemy->IsAlive())
	{
		ManualTarget = nullptr;
	}

	const UUSTeamSubsystem* Team = GetWorld() ? GetWorld()->GetSubsystem<UUSTeamSubsystem>() : nullptr;
	if (Team == nullptr || OwnerCharacter == nullptr)
	{
		AutoTarget = nullptr;
	}
	else
	{
		// 화면 밖 적을 먼저 쏘면 공격이 안 보인다 → 화면 안의 적만 노린다
		const FUSScreenBounds Screen(GetWorld());
		AutoTarget = Team->FindNearestEnemy(OwnerCharacter->GetActorLocation(), AutoAimRange, &Screen);
	}

	NotifyAimTargetIfChanged();
}

bool UUSAimComponent::IsTargetUsable(const AActor* Target, bool bIgnoreRange) const
{
	if (Target == nullptr || OwnerCharacter == nullptr)
	{
		return false;
	}

	// 죽은 적은 사망 연출 동안 액터가 남아 있으므로 생존 여부를 직접 본다
	const AUSEnemyCharacter* Enemy = Cast<AUSEnemyCharacter>(Target);
	if (Enemy == nullptr || !Enemy->IsAlive() || Enemy->IsDoomed())
	{
		return false;
	}

	// 거리·화면은 몸 가장자리 기준 — 큰 보스는 중심이 화면·사거리 밖이어도 몸이 보이면 계속 노린다 (FindNearestEnemy와 같은 규칙)
	const FVector OwnerLocation = OwnerCharacter->GetActorLocation();
	const FVector EdgePoint = Enemy->GetClosestPickPoint(OwnerLocation);
	const bool bSkipRange = bIgnoreRange || Target == ManualTarget.Get();
	if (!bSkipRange && FVector::DistSquared2D(EdgePoint, OwnerLocation) > FMath::Square(AutoAimRange))
	{
		return false;
	}

	// 표적이 화면 밖으로 나가면 놓는다 (자동 조준과 같은 규칙)
	return FUSScreenBounds(GetWorld()).Contains(EdgePoint);
}

void UUSAimComponent::NotifyAimTargetIfChanged()
{
	AActor* Current = GetTarget();
	if (!IsTargetUsable(Current))
	{
		Current = nullptr;
	}
	const bool bManual = Current != nullptr && Current == ManualTarget.Get();

	// 같은 표적이면 알리지 않는다 (RefreshAutoTarget이 0.1초마다 부른다).
	// 표적 액터가 사라져 약한 참조가 끊긴 경우(Get()은 nullptr이지만 IsExplicitlyNull()은 false)는 "바뀜"으로 알려 링을 거둔다
	const bool bUnchanged = Current == nullptr
		? NotifiedTarget.IsExplicitlyNull()
		: NotifiedTarget.Get() == Current && bNotifiedManual == bManual;
	if (bUnchanged)
	{
		return;
	}

	NotifiedTarget = Current;
	bNotifiedManual = bManual;
	OnAimTargetChanged.Broadcast(Current, bManual);
}

void UUSAimComponent::UpdateAimFromTarget()
{
	// 표적은 다음 재선택(최대 RetargetInterval) 전에 죽을 수 있으므로, 매 프레임 생존·거리만 다시 본다 (검색 없이 한 마리만)
	AActor* Target = GetTarget();
	if (!IsTargetUsable(Target))
	{
		// 한 발에 죽는 적이 많아 타이머까지 기다리면 처치마다 한 발씩 쉰다 (D6 측정: 대장 DPS가 명목치의 40%).
		// 표적을 "잃은" 순간(죽은 적은 사망 연출 동안 남아 있다)에만 바로 다시 고른다 — 검색은 처치당 한 번 늘어날 뿐이고,
		// 처음부터 표적이 없을 때(반경 안에 적 없음)는 매 프레임 검색하지 않고 타이머를 기다린다
		if (Target != nullptr)
		{
			RefreshAutoTarget();
			Target = GetTarget();
		}
		if (!IsTargetUsable(Target))
		{
			bHasValidAim = false;
			return;
		}
	}

	AimLocation = Target->GetActorLocation();
	bHasValidAim = true;
	UpdateAimDirection();
}

bool UUSAimComponent::TryGetCursorPlaneLocation(FVector& OutLocation) const
{
	if (PlayerController == nullptr || OwnerCharacter == nullptr)
	{
		return false;
	}

	FVector RayOrigin = FVector::ZeroVector;
	FVector RayDirection = FVector::ZeroVector;
	if (PlayerController->DeprojectMousePositionToWorld(RayOrigin, RayDirection) == false)
	{
		// 커서가 뷰포트 밖에 있거나 뷰포트가 아직 없다.
		return false;
	}

	// 커서 레이를 캐릭터 몸 높이의 수평면과 교차시킨다.
	// 바닥 트레이스 대신 수평면을 쓰는 이유: 커서가 적·벽 위에 있을 때 높이가 튀어 수평 사격 방향이 흔들리는 것을 막는다.
	if (FMath::Abs(RayDirection.Z) < KINDA_SMALL_NUMBER)
	{
		// 레이가 수평면과 평행 — 쿼터뷰 카메라에서는 발생하지 않지만 0 나눗셈 방지
		return false;
	}

	// 카메라 최대 거리(수천 uu)보다 충분히 길기만 하면 된다
	constexpr float RayLength = 100000.0f;
	const FVector RayEnd = RayOrigin + RayDirection * RayLength;
	OutLocation = FMath::LinePlaneIntersection(RayOrigin, RayEnd, OwnerCharacter->GetActorLocation(), FVector::UpVector);
	return true;
}

void UUSAimComponent::UpdateAimFromCursor()
{
	bHasValidAim = TryGetCursorPlaneLocation(AimLocation);
	if (bHasValidAim)
	{
		UpdateAimDirection();
	}
}

void UUSAimComponent::UpdateAimDirection()
{
	FVector ToAim = AimLocation - OwnerCharacter->GetActorLocation();
	ToAim.Z = 0.0f;
	if (ToAim.SizeSquared() < FMath::Square(MinAimDistance))
	{
		// 조준점이 캐릭터 바로 위 — 방향을 갱신하지 않고 마지막 방향을 유지한다.
		return;
	}

	AimDirection = ToAim.GetSafeNormal();
}

void UUSAimComponent::UpdateOwnerRotation(float DeltaTime)
{
	if (bHasValidAim == false)
	{
		return;
	}

	const float TargetYaw = AimDirection.Rotation().Yaw;
	const float CurrentYaw = OwnerCharacter->GetActorRotation().Yaw;
	const float NewYaw = (TurnRate > 0.0f)
		? FMath::FixedTurn(CurrentYaw, TargetYaw, TurnRate * DeltaTime)
		: TargetYaw;

	OwnerCharacter->SetActorRotation(FRotator(0.0f, NewYaw, 0.0f));

	// 컨트롤 회전은 목표 yaw로 즉시 맞춘다. 애님의 Aim Offset이 (컨트롤 회전 - 액터 회전)을 읽으므로
	// 몸이 따라오는 동안의 차이가 상체 조준 포즈로 나타난다. 스프링암은 bUsePawnControlRotation=false라 영향 없음.
	PlayerController->SetControlRotation(FRotator(0.0f, TargetYaw, 0.0f));
}
