// Fill out your copyright notice in the Description page of Project Settings.


#include "RunningTask.h"
#include "AIEnemyController.h"
#include "AIEnemyForTraining.h"

EBTNodeResult::Type URunningTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIEnemyController* AIController = Cast<AAIEnemyController>(OwnerComp.GetAIOwner());
	if (AIController)
	{
		AAIEnemyForTraining* Target = Cast<AAIEnemyForTraining>(AIController->GetPawn());
		if (Target)
		{
			Target->RunningMovement(true);
			return EBTNodeResult::Succeeded;
		}
	}
	return  EBTNodeResult::Failed;
}
