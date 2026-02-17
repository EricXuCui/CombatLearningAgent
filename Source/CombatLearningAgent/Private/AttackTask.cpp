// Fill out your copyright notice in the Description page of Project Settings.


#include "AttackTask.h"
#include "AIEnemyController.h"
#include "BaseRole.h"
#include "AIEnemyForTraining.h"
#include "BehaviorTree/BlackboardComponent.h"

EBTNodeResult::Type UAttackTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIEnemyController* AIController = Cast<AAIEnemyController>(OwnerComp.GetAIOwner());
	if (AIController)
	{
		AAIEnemyForTraining* Self = Cast<AAIEnemyForTraining>(AIController->GetPawn());
		if (Self)
		{
			Self->Attack();
			Self->RunningMovement(false);
			return EBTNodeResult::Succeeded;
		}
		else
		{
			return EBTNodeResult::Failed;
		}
	}
	else
	{
		return EBTNodeResult::Failed;
	}
}
