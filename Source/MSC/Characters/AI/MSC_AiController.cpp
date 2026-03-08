#include "MSC_AIController.h"
#include "Components/StateTreeAIComponent.h"

AMSC_AIController::AMSC_AIController()
{
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	check(StateTreeAI)
	
	bStartAILogicOnPossess = true;
	
	bAttachToPawn = true;
}
