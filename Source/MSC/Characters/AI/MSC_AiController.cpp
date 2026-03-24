#include "MSC_AIController.h"
#include "Components/StateTreeAIComponent.h"

AMSC_AIController::AMSC_AIController()
{
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));
	check(StateTreeAI)
	
	// We need to grant the abilities first
	bStartAILogicOnPossess = false;
	bAttachToPawn = true;
}

void AMSC_AIController::BeginPlay()
{
	Super::BeginPlay();
}