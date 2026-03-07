#include "MSC_MovementAttributeSet.h"


UMSC_MovementAttributeSet::UMSC_MovementAttributeSet()
{
	InitMoveSpeed(400.0f);
}

void UMSC_MovementAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	
	if (Attribute == GetMoveSpeedAttribute())
	{
		OnMoveSpeedChanged.Broadcast(this, OldValue, NewValue);
	}
}
