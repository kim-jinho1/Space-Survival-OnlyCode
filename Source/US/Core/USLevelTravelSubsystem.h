#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "USLevelTravelSubsystem.generated.h"

class SWidget;
class UFont;
class UParticleSystemComponent;
class UWorld;

/**
 * 맵 전환 연출 (출시 전 체크 R-34·R-35, 사용자 결정: 페이드 + 로딩 화면).
 * 떠날 때: 화면 맨 위에 깐 검은 판을 FadeOutSeconds 동안 어둡게 → OpenLevel.
 * 로드 중: MoviePlayer 로딩 화면("로딩 중" + 팁). 패키징 빌드에서만 보인다(에디터 PIE는 MoviePlayer가 꺼져 있다).
 * 새 맵: 검은 판을 다시 깔고 HoldSeconds 동안 유지하며 이펙트를 대장 위치에서 한 번씩 틀어(R-35) 셰이더 상태를 준비한 뒤 FadeInSeconds 동안 밝힌다.
 * 페이드는 월드 시간이 아니라 실제 시간(FTSTicker)으로 돈다 — 결산 창처럼 게임이 멈춘 상태에서도 떠날 수 있게.
 * 검은 판은 UMG가 아니라 뷰포트에 직접 넣은 Slate라 HUD·결산 창 위에도 덮인다(카메라 페이드는 3D 화면만 덮는다).
 * 맵을 여는 곳(타이틀·격납고·전투 컨트롤러)은 OpenLevel 대신 Travel을 부른다.
 */
UCLASS()
class US_API UUSLevelTravelSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 페이드아웃 뒤 맵을 연다 (이미 떠나는 중이면 무시) */
	void Travel(const TSoftObjectPtr<UWorld>& Map);

	/** 같은 처리, 레벨 이름(긴 패키지 이름 또는 짧은 이름)으로 */
	void TravelByName(const FName& LevelName);

	/** 월드 컨텍스트에서 서브시스템을 찾아 Travel. 없으면(에디터 미리보기 등) 바로 OpenLevel */
	static void TravelFrom(const UObject* WorldContextObject, const TSoftObjectPtr<UWorld>& Map);
	static void TravelByNameFrom(const UObject* WorldContextObject, const FName& LevelName);

private:
	enum class EFadeState : uint8
	{
		None,
		FadingOut,
		Holding,
		FadingIn
	};

	void HandlePreLoadMap(const FString& MapName);
	void HandlePostLoadMap(UWorld* LoadedWorld);

	/** 실제 시간 페이드 진행 */
	bool HandleTicker(float DeltaTime);

	/** 검은 판을 지금 게임 뷰포트 맨 위에 넣는다 (맵이 바뀌면 엔진이 뷰포트 위젯을 다 지우므로 맵마다 다시) */
	void AddOverlay();
	void RemoveOverlay();

	/** 로딩 화면 Slate 위젯 */
	TSharedRef<SWidget> MakeLoadingScreenWidget() const;

	/** 새 맵이 검은 동안 이펙트를 한 번씩 튼다 */
	void WarmupEffects(UWorld* World);

	/** 밝아지기 전에 미리 튼 이펙트를 지운다 (폭발은 2~3초짜리라 그대로 두면 대장 자리에서 터지는 게 보인다) */
	void ClearWarmupEffects();

	void StartTicker();

	/**
	 * 로딩 화면 글꼴. UPROPERTY로 붙잡아 둔다 — FSlateFontInfo는 UFont를 날 포인터로만 들고 있어서,
	 * 맵 로드 중 GC가 이 글꼴을 지우면 로딩 화면 스레드가 지워진 객체를 읽다 꺼진다(v0.1.1 시작 크래시 "Pure virtual function being called").
	 */
	UPROPERTY()
	TObjectPtr<UFont> LoadingFont;

	TSharedPtr<SWidget> Overlay;

	FTSTicker::FDelegateHandle TickerHandle;

	EFadeState FadeState = EFadeState::None;

	/** 지금 상태가 시작된 실제 시각 */
	double StateStartTime = 0.0;

	/** 검은 판 불투명도 (0 = 투명, 1 = 검정) */
	float OverlayAlpha = 0.0f;

	/** 페이드아웃이 끝나면 열 레벨 */
	FName PendingLevel;

	/** 검은 동안 미리 튼 이펙트 (밝아지기 직전에 지운다) */
	TArray<TWeakObjectPtr<UParticleSystemComponent>> WarmupComponents;
};
