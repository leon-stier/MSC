#pragma once

#include "AIController.h"
#include "MSC_AIController.generated.h"

class UStateTreeAIComponent;

UCLASS(abstract)
class MSC_API AMSC_AIController : public AAIController 
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStateTreeAIComponent* StateTreeAI;
public:
	AMSC_AIController();
};
