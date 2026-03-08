#pragma once

#include "AIController.h"
#include "MSC_AiController.generated.h"

class UStateTreeAIComponent;

UCLASS(abstract)
class MSC_API AMSC_AiController : public AAIController 
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UStateTreeAIComponent* StateTreeAI;
public:
	
};
