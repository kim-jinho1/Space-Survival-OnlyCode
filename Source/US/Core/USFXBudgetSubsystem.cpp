#include "Core/USFXBudgetSubsystem.h"
#include "Core/USFXBudgetSettings.h"
#include "Components/USThrusterFXComponent.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

namespace
{
	/** 평소 판은 폭발이 동시에 12개 안팎이라 24면 무리·연쇄 폭발이 몰릴 때만 걸린다 */
	TAutoConsoleVariable<int32> CVarMaxConcurrentPerSystem(
		TEXT("US.FX.MaxConcurrentPerSystem"),
		24,
		TEXT("Max simultaneous instances of one burst effect (death / chain explosions). Extra spawns are skipped."),
		ECVF_Default);

	/** 가벼운 판은 불꽃이 원본의 20%라 두 배를 둬도 원본 상한 하나보다 싸다 */
	TAutoConsoleVariable<int32> CVarMaxConcurrentProxyPerSystem(
		TEXT("US.FX.MaxConcurrentProxyPerSystem"),
		48,
		TEXT("Max simultaneous instances of a cull proxy (light version) played when the full effect is over its limit."),
		ECVF_Default);
}

UParticleSystemComponent* UUSFXBudgetSubsystem::SpawnBudgetedEmitter(UParticleSystem* FX, const FVector& Location, const FVector& Scale)
{
	if (FX == nullptr)
	{
		return nullptr;
	}

	if (UParticleSystemComponent* Component = SpawnWithinLimit(FX, Location, Scale, CVarMaxConcurrentPerSystem.GetValueOnGameThread()))
	{
		return Component;
	}

	// 상한을 넘었다고 생략하면 무리를 잡을 때 적이 그냥 사라진다 → 불꽃 수를 줄인 판으로 대신 터뜨린다
	UParticleSystem* Proxy = FindCullProxy(FX);
	return Proxy ? SpawnWithinLimit(Proxy, Location, Scale, CVarMaxConcurrentProxyPerSystem.GetValueOnGameThread()) : nullptr;
}

UParticleSystem* UUSFXBudgetSubsystem::FindCullProxy(UParticleSystem* FX)
{
	if (const TObjectPtr<UParticleSystem>* Cached = CullProxyCache.Find(FX))
	{
		return *Cached;
	}

	// 전투 중 처음 넘칠 때 한 번만 동기 로드된다 (워밍업 목록에도 넣어 두면 이미 메모리에 있다)
	const TSoftObjectPtr<UParticleSystem>* ProxyPtr = GetDefault<UUSFXBudgetSettings>()->CullProxies.Find(TSoftObjectPtr<UParticleSystem>(FX));
	UParticleSystem* Proxy = ProxyPtr ? ProxyPtr->LoadSynchronous() : nullptr;
	CullProxyCache.Add(FX, Proxy);
	return Proxy;
}

UParticleSystemComponent* UUSFXBudgetSubsystem::SpawnWithinLimit(UParticleSystem* FX, const FVector& Location, const FVector& Scale, int32 Limit)
{
	TArray<TWeakObjectPtr<UParticleSystemComponent>>& Active = ActiveByFX.FindOrAdd(FX);
	Active.RemoveAll([](const TWeakObjectPtr<UParticleSystemComponent>& Component)
	{
		return !Component.IsValid() || !Component->IsActive();
	});

	if (Active.Num() >= Limit)
	{
		return nullptr;
	}

	// 연달아 터지므로 월드 파티클 풀에서 빌리고 재생이 끝나면 자동 반납한다.
	// 카메라 거리(2000) 근처에서 LOD 1로 내려가 작고 흐려지지 않게 LOD 0으로 고정한다 (불꽃과 같은 문제)
	UParticleSystemComponent* Component = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FX, Location, FRotator::ZeroRotator,
		Scale, true, EPSCPoolMethod::AutoRelease);
	UUSThrusterFXComponent::ForceTopLOD(Component);

	if (Component)
	{
		// 풀에서 같은 컴포넌트를 다시 빌려 올 수 있어 중복 없이 담는다
		Active.AddUnique(Component);
	}
	return Component;
}
