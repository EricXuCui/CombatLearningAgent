// Fill out your copyright notice in the Description page of Project Settings.


#include "FocusTask.h"
#include "AIEnemyController.h"
#include "BaseRole.h"
#include "BehaviorTree/BlackboardComponent.h"

EBTNodeResult::Type UFocusTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIEnemyController* AIController = Cast<AAIEnemyController>(OwnerComp.GetAIOwner());
	if (AIController)
	{
		ABaseRole* Target = Cast<ABaseRole>(AIController->GetBlackboardComponent()->GetValueAsObject(TEXT("Target")));
		if (Target)
		{
			AIController->SetFocus(Target);
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
