// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "USEnemyAIController.generated.h"

/**
 * 적 빙의용 AI 컨트롤러.
 * 현재 이동/추적은 AUSEnemyCharacter가 직접(직선 추적) 처리하므로 빙의만 담당한다.
 * 추후 BehaviorTree / NavMesh 기반 경로탐색이 필요하면 이 클래스에서 확장한다.
 */
UCLASS()
class US_API AUSEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AUSEnemyAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
};
