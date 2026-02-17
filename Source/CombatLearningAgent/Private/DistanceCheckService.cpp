// Fill out your copyright notice in the Description page of Project Settings.


#include "DistanceCheckService.h"
#include "AIEnemyController.h"
#include "BaseRole.h"
#include "AIEnemyForTraining.h"
#include "BehaviorTree/BlackboardComponent.h"

void UDistanceCheckService::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIEnemyController* AIController = Cast<AAIEnemyController>(OwnerComp.GetAIOwner());
	if (AIController)
	{
		ABaseRole* Target = Cast<ABaseRole>(AIController->GetBlackboardComponent()->GetValueAsObject(TEXT("Target")));
		AAIEnemyForTraining* Self = Cast<AAIEnemyForTraining>(AIController->GetPawn());
		if (Target)
		{
			float Distance = Self->GetDistanceTo(Target);
			if (Distance <= 250)
			{
				AIController->GetBlackboardComponent()->SetValueAsBool("Attack", true);
				AIController->GetBlackboardComponent()->SetValueAsBool("Run", false);
			}
			else
			{
				AIController->GetBlackboardComponent()->SetValueAsBool("Attack", false);
				if (Distance >= 1300)
				{
					AIController->GetBlackboardComponent()->SetValueAsBool("Run", true);
				}
			}
		}
	}

}
