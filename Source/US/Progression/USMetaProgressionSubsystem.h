#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "USMetaProgressionSubsystem.generated.h"

class APawn;
class UUSAllyTypeData;
class UUSMetaSaveGame;
class UUSMetaTreeData;
class UUSMetaUpgradeData;
class UUSStageData;
class UUSZoneData;
struct FUSRunResult;
enum class EUSMetaEffect : uint8;

/** 스크랩·칸 단계·기록이 바뀜 (트리 위젯이 구독) */
DECLARE_MULTICAST_DELEGATE(FDelegateOnMetaChanged);

/**
 * 판 사이에 남는 메타 진행(스크랩·영구 업그레이드·최고 기록)을 들고 저장하는 서브시스템.
 * GameInstance 서브시스템이라 게임을 켜서 끌 때까지 하나만 살아 있고, OpenLevel로 맵을 옮겨도 사라지지 않는다.
 * 시작할 때 저장 슬롯을 읽고, 값이 바뀔 때마다 바로 저장한다 (데이터가 작아 동기 저장).
 * 칸 데이터(UUSMetaUpgradeData)는 들고 있지 않고 단계만 NodeId로 기록한다. 효과 계산은 호출하는 쪽이 트리를 넘긴다.
 */
UCLASS()
class US_API UUSMetaProgressionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** 보유 스크랩 */
	int32 GetScrap() const;

	/** Node의 지금 단계 (없으면 0) */
	int32 GetLevel(const UUSMetaUpgradeData* Node) const;

	/** Node를 지금 살 수 있게 열려 있는가 (선행 칸이 없거나 1단계 이상) */
	bool IsNodeOpen(const UUSMetaUpgradeData* Node) const;

	/** Node가 최대 단계인가 */
	bool IsNodeMaxed(const UUSMetaUpgradeData* Node) const;

	/** 끝낸 판 수 */
	int32 GetRunCount() const;

	/** 클리어 횟수 */
	int32 GetClearCount() const;

	/** 최고 생존 시간(초) */
	float GetBestSurvivedSeconds() const;

	/** 한 판 최다 처치 */
	int32 GetBestKillCount() const;

	/** Tree에서 Effect 종류 칸들의 단계 합 (격납고 진열: 시작 부하 수) */
	int32 GetTotalLevel(const UUSMetaTreeData* Tree, EUSMetaEffect Effect) const;

	/** AllyType을 쓸 수 있는가 (Tree에 그 기종을 여는 해금 칸이 없거나, 있으면 1단계 이상) */
	bool IsAllyTypeUnlocked(const UUSMetaTreeData* Tree, const UUSAllyTypeData* AllyType) const;

	/** 출격 편성 칸 수 = 기본 칸 + 편성 칸 업그레이드 단계 */
	int32 GetLoadoutSlotCount(const UUSMetaTreeData* Tree) const;

	/** 편성 칸들 (칸 수만큼, 비운 칸은 nullptr). 고르지 않은 칸·잠긴 기종은 트리의 기본 기종으로 채운다 */
	TArray<const UUSAllyTypeData*> GetLoadoutSlots(const UUSMetaTreeData* Tree) const;

	/** 출격할 부하들 = 편성 칸에서 빈 칸을 뺀 것 (판 시작 때 분대에 넘긴다) */
	TArray<const UUSAllyTypeData*> GetLoadout(const UUSMetaTreeData* Tree) const;

	/** 편성 SlotIndex 칸의 기종을 바꾸고 저장한다 (AllyType이 nullptr이면 칸을 비운다). 잠긴 기종이나 없는 칸이면 false */
	bool SetLoadoutSlot(const UUSMetaTreeData* Tree, int32 SlotIndex, const UUSAllyTypeData* AllyType);

	/** Node의 다음 단계 가격 */
	int32 GetNextCost(const UUSMetaUpgradeData* Node) const;

	/** Node를 한 단계 산다. 잠김·최대 단계·스크랩 부족이면 false */
	bool TryBuy(const UUSMetaUpgradeData* Node);

	/** Tree에서 Effect 종류 칸들이 주는 배율 = Π (1 + 단계당 값)^단계 */
	float GetMultiplier(const UUSMetaTreeData* Tree, EUSMetaEffect Effect) const;

	/** Tree에서 Effect 종류 칸들이 더해 주는 값 = Σ 단계당 값 × 단계 (출격 시간처럼 더해서 쌓이는 칸) */
	float GetAdditiveTotal(const UUSMetaTreeData* Tree, EUSMetaEffect Effect) const;

	/** 판 시작 때 Tree의 영구 효과와 출격 편성을 대장(폰)과 분대에 적용한다. 폰의 BeginPlay 전에 불러야 편성이 시작 부하로 스폰된다 */
	void ApplyToRun(const UUSMetaTreeData* Tree, APawn* Leader) const;

	/** 끝난 판을 기록한다: 결산 스크랩 적립, 판 수·클리어 수·최고 기록 갱신, 저장 */
	void RecordRun(const FUSRunResult& Result);

	/** 스크랩을 더한다 (음수면 빼되 0 아래로는 안 간다) */
	void AddScrap(int32 Amount);

	/** NodeId 칸의 단계를 직접 정한다 (시험용 콘솔 명령) */
	void SetLevel(FName NodeId, int32 Level);

	/** 저장을 처음 상태로 되돌린다 (설정 창 "진행 초기화", 시험용 콘솔 명령) */
	void ResetProgress();

	/** 현재 상태를 LogUS로 출력한다 (시험용 콘솔 명령) */
	void LogStatus() const;

	// ---- 스테이지 (행성 지역 지도) ----
	/** Stage를 깼는가 */
	bool IsStageCleared(const UUSStageData* Stage) const;

	/**
	 * Stage를 고를 수 있는가: 깬 곳(다시 하기), 첫 스테이지, 깬 지역과 맞닿은 곳(Neighbors를 양쪽으로 본다).
	 * 보스 스테이지는 나머지를 다 깨야 열린다
	 */
	bool IsStageOpen(const UUSZoneData* Zone, const UUSStageData* Stage) const;

	/** "다음 스테이지": 열려 있고 아직 안 깬 곳 중 번호가 가장 작은 곳 (Current 제외). 없으면 nullptr */
	const UUSStageData* FindNextStage(const UUSZoneData* Zone, const UUSStageData* Current) const;

	/** Zone에서 깬 스테이지 수 */
	int32 GetClearedStageCount(const UUSZoneData* Zone) const;

	/** Stage를 깼다고 기록하고 저장한다 (클리어 순간에 — 이어서 가다 격추돼도 남는다) */
	void MarkStageCleared(const UUSStageData* Stage);

	/** 이번 출격의 구역·스테이지 (격납고가 정하고 전투 맵이 읽는다, 저장하지 않는다). 스테이지가 없는 구역이면 Stage = nullptr */
	void SetSortie(const UUSZoneData* Zone, const UUSStageData* Stage);
	FORCEINLINE const UUSZoneData* GetSortieZone() const { return SortieZone; }
	FORCEINLINE const UUSStageData* GetSortieStage() const { return SortieStage; }

	/** 전투 맵 → 격납고: 행성을 깨는 중(스테이지를 깼고 보스는 아직)이면 격납고 "출격"이 그 행성 지역 지도로 바로 간다 */
	FORCEINLINE void SetReturnToStageMap(bool bReturn) { bReturnToStageMap = bReturn; }

	/** 격납고 시작 때 한 번 읽고 지운다 (다음 격납고 진입에 남지 않게) */
	bool ConsumeReturnToStageMap();

	/** 스크랩·단계·기록 변경 */
	FDelegateOnMetaChanged OnMetaChanged;

private:
	/** 지금 상태를 슬롯에 쓰고 OnMetaChanged를 알린다 */
	void SaveAndNotify();

	/** 읽지 못한 세이브 파일의 원본 바이트를 SlotName_Broken_날짜시각 슬롯으로 복사한다 (새 세이브가 덮어쓰기 전에) */
	void BackupUnreadableSave() const;

	/** 저장 슬롯 이름 */
	static const FString SlotName;

	/** 메모리에 들고 있는 저장 데이터 (Initialize에서 불러오거나 새로 만든다) */
	UPROPERTY()
	TObjectPtr<UUSMetaSaveGame> SaveData;

	/** 이번 출격 (SetSortie). 데이터 애셋이라 맵을 옮겨도 남는다 */
	UPROPERTY()
	TObjectPtr<const UUSZoneData> SortieZone;

	UPROPERTY()
	TObjectPtr<const UUSStageData> SortieStage;

	/** SetReturnToStageMap. 저장하지 않는다 */
	bool bReturnToStageMap = false;
};
