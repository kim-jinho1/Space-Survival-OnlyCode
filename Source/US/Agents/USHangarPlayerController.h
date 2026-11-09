#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "USHangarPlayerController.generated.h"

class AUSDroneCharacter;
class AUSHangarModelStage;
class UUSAllyTypeData;
class UUSHangarWidget;
class UUSMetaTreeData;
class UUSZoneData;
class UWorld;

/**
 * 격납고 맵 전용 컨트롤러. 조종할 폰 없이 격납고 위젯을 띄우고 기체 고르기·출격·타이틀 이동을 맡는다.
 * 기체는 대장기(0번) + CraftTypes 순서이고, 위젯 위쪽 기종 아이콘으로 고르면 설계도 화면에 그 기체의 도면과 강화 카드를 띄운다.
 * 늘 한 대가 골라져 있다: 들어오면 대장기를 고른다 (패널이 비는 순간이 없게).
 * 출격하면 선택한 구역의 맵을, 타이틀로 가면 타이틀 맵을 연다. 격납고 맵의 UI는 여기 한 곳에서 만든다.
 * 고른 기체는 맵에 놓인 3D 무대(AUSHangarModelStage)에도 올린다. 무대는 렌더 타겟으로 찍어 설계도 화면 왼쪽에 띄운다 (M-2 격납고 B).
 * 예전 3D 진열대 + 월드 카메라 접근 방식은 D10-UI2 재작업(2026-10-07)에서 버렸다 (이유는 TASK.md).
 */
UCLASS()
class US_API AUSHangarPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AUSHangarPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	/** CraftIndex 기체를 고른다 (0 = 대장기, 1~ = CraftTypes): 설계도 패널, 기종 아이콘 강조 */
	void SelectCraft(int32 CraftIndex);

	/** CraftIndex 기체의 기종 (0 = 대장기 = nullptr) */
	const UUSAllyTypeData* GetCraftType(int32 CraftIndex) const;

	/** 고를 수 있는 기체 수 (대장기 포함) */
	int32 GetCraftCount() const;

	/** 고른 기체를 3D 무대에 올린다 (잠금 여부 포함) */
	void RefreshModelStage();

	/** 해금 등 메타 진행이 바뀜 → 무대의 잠김 모습 갱신 */
	void HandleMetaChanged();

	/** 위젯 기종 아이콘 클릭 */
	void HandleCraftSelectRequested(int32 CraftIndex);

	/** "출격" → Zone의 맵 열기 */
	void HandleLaunchRequested(const UUSZoneData* Zone);

	/** "타이틀로" → 타이틀 맵 열기 */
	void HandleTitleRequested();

	/** 격납고 위젯 (WBP_USHangar) */
	UPROPERTY(EditAnywhere, Category = "US|UI")
	TSubclassOf<UUSHangarWidget> HangarWidgetClass;

	/** 영구 업그레이드 트리 (게임 컨트롤러 BP_USPlayerController.MetaTree와 같은 애셋) */
	UPROPERTY(EditAnywhere, Category = "US|Hangar")
	TObjectPtr<UUSMetaTreeData> MetaTree;

	/** 대장기 뒤에 이어질 부하 기종 (위쪽 아이콘 줄 왼쪽부터) */
	UPROPERTY(EditAnywhere, Category = "US|Hangar")
	TArray<TObjectPtr<UUSAllyTypeData>> CraftTypes;

	/** 대장기 BP (부하는 기종 데이터의 DroneClass). 3D 무대가 이 클래스의 외형을 읽는다 */
	UPROPERTY(EditAnywhere, Category = "US|Hangar")
	TSubclassOf<AUSDroneCharacter> LeaderCraftClass;

	/** 출격 구역 목록 (순서대로) */
	UPROPERTY(EditAnywhere, Category = "US|Hangar")
	TArray<TObjectPtr<UUSZoneData>> Zones;

	/** "타이틀로"가 여는 맵 */
	UPROPERTY(EditAnywhere, Category = "US|Hangar")
	TSoftObjectPtr<UWorld> TitleMap;

private:
	UPROPERTY()
	TObjectPtr<UUSHangarWidget> HangarWidget;

	/** 맵에 놓인 3D 무대 (BeginPlay에서 한 번 찾는다. 없으면 무대 없이 동작) */
	UPROPERTY()
	TObjectPtr<AUSHangarModelStage> ModelStage;

	/** 메타 진행 변경 구독 핸들 */
	FDelegateHandle MetaChangedHandle;

	/** 지금 고른 기체 */
	int32 SelectedCraft = INDEX_NONE;
};
