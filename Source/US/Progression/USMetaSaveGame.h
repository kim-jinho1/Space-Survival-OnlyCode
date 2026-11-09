#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "USMetaSaveGame.generated.h"

/**
 * 판 사이에 남는 것 전부: 스크랩, 영구 업그레이드 단계, 최고 기록.
 * USaveGame은 UPROPERTY만 직렬화하므로 저장할 값은 전부 UPROPERTY여야 한다.
 */
UCLASS()
class US_API UUSMetaSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** 지금 코드가 쓰는 저장 형식 버전. 형식을 바꾸면 올리고 불러올 때 옛 버전을 변환한다 */
	static constexpr int32 CurrentVersion = 4;

	/** 이 파일의 저장 형식 버전 */
	UPROPERTY()
	int32 Version = CurrentVersion;

	/** 보유 스크랩 */
	UPROPERTY()
	int32 Scrap = 0;

	/** 칸별 단계 (키 = UUSMetaUpgradeData::NodeId). 없는 칸은 0단계 */
	UPROPERTY()
	TMap<FName, int32> NodeLevels;

	/** 출격 편성 (칸 순서대로 부하 종류 애셋 경로). 버전 2에서 추가, 비어 있으면 기본 기종으로 채운다 */
	UPROPERTY()
	TArray<FSoftObjectPath> Loadout;

	/** 일부러 비운 편성 칸 번호. 버전 3에서 추가 — Loadout의 빈 경로는 예전처럼 "기본 기종"으로 남겨 옛 저장의 뜻이 바뀌지 않게 따로 둔다 */
	UPROPERTY()
	TSet<int32> EmptyLoadoutSlots;

	/** 깬 스테이지 (UUSStageData::StageId). 버전 4에서 추가, 비어 있으면 첫 스테이지만 열린다 */
	UPROPERTY()
	TSet<FName> ClearedStages;

	/** 끝낸 판 수 */
	UPROPERTY()
	int32 RunCount = 0;

	/** 클리어(목표 시간 생존) 횟수 */
	UPROPERTY()
	int32 ClearCount = 0;

	/** 최고 생존 시간(초) */
	UPROPERTY()
	float BestSurvivedSeconds = 0.0f;

	/** 한 판 최다 처치 */
	UPROPERTY()
	int32 BestKillCount = 0;
};
