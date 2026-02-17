// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "AIEnemyController.h"
#include "AIEnemyForTraining.h"
#include "StrafeTask.generated.h"

/**
 * 
 */
UCLASS()
class COMBATLEARNINGAGENT_API UStrafeTask : public UBTTaskNode
{
	GENERATED_BODY()
public:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	void CancelStrafe();
	AAIEnemyController* AIController;
	AAIEnemyForTraining* Self;
	FTimerHandle _DelayCancelStrafeMovement;
};
