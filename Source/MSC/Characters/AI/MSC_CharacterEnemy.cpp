#include "MSC_CharacterEnemy.h"

#include "MSC/GAS/MSC_AbilitySystemComponent.h"

AMSC_CharacterEnemy::AMSC_CharacterEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMSC_CharacterEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	OnAttackCompleted.AddDynamic(this, &AMSC_CharacterEnemy::OnAttackCompletedForward);
}


void AMSC_CharacterEnemy::DoPunch()
{
	if (PunchAbility && MSC_AbilitySystemComponent)
	{
		MSC_AbilitySystemComponent->TryActivateAbilityByClass(PunchAbility);
	}
}

void AMSC_CharacterEnemy::OnAttackCompletedForward()
{
	OnAttackCompletedNative.ExecuteIfBound();
}
