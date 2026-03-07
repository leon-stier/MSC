#include "MSC_CharacterBase.h"
#include "../GAS/MSC_AbilitySystemComponent.h"
#include "MSC/GAS/MSC_HealthAttributeSet.h"


AMSC_CharacterBase::AMSC_CharacterBase()
{
	MSC_AbilitySystemComponent = CreateDefaultSubobject<UMSC_AbilitySystemComponent>(TEXT("ASC"));
	HealthSet = CreateDefaultSubobject<UMSC_HealthAttributeSet>(TEXT("HealthSet"));
}

void AMSC_CharacterBase::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("Processing %d abilities in %s"), DefaultAbilities.Num(), *GetNameSafe(this));

	MSC_AbilitySystemComponent->InitAbilityActorInfo(this, this);
	
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (!AbilityClass)
		{
			continue;
		}
		MSC_AbilitySystemComponent->GiveAbility(AbilityClass);
	}
}

UAbilitySystemComponent* AMSC_CharacterBase::GetAbilitySystemComponent() const
{
	return MSC_AbilitySystemComponent;
}
	