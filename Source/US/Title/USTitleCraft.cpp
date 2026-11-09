#include "Title/USTitleCraft.h"
#include "US.h"
#include "Agents/USDroneCharacter.h"
#include "Components/USHoverVisualComponent.h"
#include "Components/USThrusterFXComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

AUSTitleCraft::AUSTitleCraft()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(SceneRoot);

	// 배경 장식이라 충돌·내비 영향 없음
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(VisualRoot);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetCanEverAffectNavigation(false);
}

void AUSTitleCraft::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyCraftLook();
}

void AUSTitleCraft::BeginPlay()
{
	// 연출 컴포넌트는 Super::BeginPlay가 컴포넌트 BeginPlay를 돌리기 전에 만들고 대상을 넘겨야 한다
	CreateEffectComponents();

	Super::BeginPlay();

	BaseLocation = GetActorLocation();
	SwayPhase = FVector2D(FMath::FRandRange(0.0f, UE_TWO_PI), FMath::FRandRange(0.0f, UE_TWO_PI));
}

void AUSTitleCraft::ApplyCraftLook()
{
	const AUSDroneCharacter* CraftDefaults = CraftClass ? CraftClass->GetDefaultObject<AUSDroneCharacter>() : nullptr;
	const UStaticMeshComponent* SourceMesh = CraftDefaults ? CraftDefaults->GetVisualMesh() : nullptr;
	if (SourceMesh == nullptr)
	{
		VisualMesh->SetStaticMesh(nullptr);
		return;
	}

	// 판 안의 모습과 같게: BP에서 보정한 위치·회전·크기와 슬롯 재질을 그대로 옮긴다 (보정 위치 덕에 액터 원점 = 기체 가운데)
	VisualMesh->SetStaticMesh(SourceMesh->GetStaticMesh());
	VisualMesh->SetRelativeTransform(SourceMesh->GetRelativeTransform());
	VisualMesh->EmptyOverrideMaterials();
	for (int32 Slot = 0; Slot < VisualMesh->GetNumMaterials(); ++Slot)
	{
		VisualMesh->SetMaterial(Slot, SourceMesh->GetMaterial(Slot));
	}
	for (const TPair<FName, TObjectPtr<UMaterialInterface>>& Pair : CraftDefaults->GetSlotMaterials())
	{
		const int32 Slot = VisualMesh->GetMaterialIndex(Pair.Key);
		if (Pair.Value && Slot != INDEX_NONE)
		{
			VisualMesh->SetMaterial(Slot, Pair.Value);
		}
	}
}

void AUSTitleCraft::CreateEffectComponents()
{
	const AUSDroneCharacter* CraftDefaults = CraftClass ? CraftClass->GetDefaultObject<AUSDroneCharacter>() : nullptr;
	if (CraftDefaults == nullptr)
	{
		UE_LOG(LogUS, Warning, TEXT("AUSTitleCraft: %s has no CraftClass. It does not hover or show thrusters."), *GetName());
		return;
	}

	// CDO의 컴포넌트를 템플릿으로 만들면 BP에서 잡은 값(둥실 높이, 엔진 위치, 불꽃 이펙트)이 그대로 복사된다
	if (const UUSHoverVisualComponent* Template = CraftDefaults->GetHoverVisual())
	{
		HoverVisual = NewObject<UUSHoverVisualComponent>(this, TEXT("HoverVisual"), RF_NoFlags, const_cast<UUSHoverVisualComponent*>(Template));
		HoverVisual->SetVisualTarget(VisualRoot);
		HoverVisual->RegisterComponent();
	}

	// 불꽃도 VisualRoot에 붙여야 둥실·기울기를 따라간다 (드론과 같다)
	if (const UUSThrusterFXComponent* Template = CraftDefaults->GetThruster())
	{
		Thruster = NewObject<UUSThrusterFXComponent>(this, TEXT("Thruster"), RF_NoFlags, const_cast<UUSThrusterFXComponent*>(Template));
		Thruster->SetAttachParent(VisualRoot);
		Thruster->RegisterComponent();
	}
}

void AUSTitleCraft::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 출렁임: 위상을 누적해 한 바퀴마다 되돌린다 (float 정밀도)
	const FVector2D AngularSpeed(UE_TWO_PI / SwayPeriod.X, UE_TWO_PI / SwayPeriod.Y);
	SwayPhase.X = FMath::Fmod(SwayPhase.X + DeltaSeconds * AngularSpeed.X, UE_TWO_PI);
	SwayPhase.Y = FMath::Fmod(SwayPhase.Y + DeltaSeconds * AngularSpeed.Y, UE_TWO_PI);

	const FVector LocalOffset(FMath::Sin(SwayPhase.X) * SwayAmplitude.X, FMath::Sin(SwayPhase.Y) * SwayAmplitude.Y, 0.0f);
	SetActorLocation(BaseLocation + GetActorRotation().RotateVector(LocalOffset));

	// 연출 컴포넌트는 GetVelocity(= 루트 ComponentVelocity)를 읽는다: 순항 속도 + 출렁임 속도(위치의 미분)를 적어 준다.
	// SetActorLocation은 속도를 남기지 않아 직접 적어야 기울기·불꽃이 반응한다
	const FVector LocalVelocity(
		CruiseSpeed + FMath::Cos(SwayPhase.X) * SwayAmplitude.X * AngularSpeed.X,
		FMath::Cos(SwayPhase.Y) * SwayAmplitude.Y * AngularSpeed.Y,
		0.0f);
	SceneRoot->ComponentVelocity = GetActorRotation().RotateVector(LocalVelocity);
}
