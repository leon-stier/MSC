#include "MSC_CharacterEnemy.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "MSC/GAS/MSC_AbilitySystemComponent.h"

AMSC_CharacterEnemy::AMSC_CharacterEnemy()
{
}

void AMSC_CharacterEnemy::DoPunch()
{
	if (PunchAbility && MSC_AbilitySystemComponent)
	{
		MSC_AbilitySystemComponent->TryActivateAbilityByClass(PunchAbility);
	}
}
