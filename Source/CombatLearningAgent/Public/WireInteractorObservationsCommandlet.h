#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "WireInteractorObservationsCommandlet.generated.h"

UCLASS()
class COMBATLEARNINGAGENT_API UWireInteractorObservationsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UWireInteractorObservationsCommandlet();

	virtual int32 Main(const FString& Params) override;
};
