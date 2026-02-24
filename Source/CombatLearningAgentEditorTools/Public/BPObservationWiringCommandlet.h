#pragma once

#include "Commandlets/Commandlet.h"

#include "BPObservationWiringCommandlet.generated.h"

UCLASS()
class COMBATLEARNINGAGENTEDITORTOOLS_API UBPObservationWiringCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UBPObservationWiringCommandlet();

	virtual int32 Main(const FString& Params) override;
};
