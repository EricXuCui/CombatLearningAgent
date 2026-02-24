// Fill out your copyright notice in the Description page of Project Settings.


#include "StrafeTask.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "KIsmet/KismetMathLibrary.h"

EBTNodeResult::Type UStrafeTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AIController = Cast<AAIEnemyController>(OwnerComp.GetAIOwner());
	if (AIController && AIController->GetBlackboardComponent())
	{
		Self = Cast<AAIEnemyForTraining>(AIController->GetPawn());
		if (Self)
		{
			if (!AIController->GetBlackboardComponent()->GetValueAsBool("StrafeDoOnce"))
			{
				AIController->GetBlackboardComponent()->SetValueAsBool("StrafeDoOnce", true);
				Self->SetActorTickEnabled(true);
				GetWorld()->GetTimerManager().SetTimer(_DelayCancelStrafeMovement, this, &UStrafeTask::CancelStrafe, UKismetMathLibrary::RandomIntegerInRange(3, 5), false
					, UKismetMathLibrary::RandomIntegerInRange(3, 5));
				return EBTNodeResult::Succeeded;
			}
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
	return EBTNodeResult::Succeeded;
}

void UStrafeTask::CancelStrafe()
{
	if (AIController)
	{
		if (Self && AIController->GetBlackboardComponent())
		{
			AIController->GetBlackboardComponent()->SetValueAsBool("Strafe", false);
			Self->SetActorTickEnabled(false);
			AIController->GetBlackboardComponent()->SetValueAsBool("StrafeDoOnce", false);
		}
	}
}
