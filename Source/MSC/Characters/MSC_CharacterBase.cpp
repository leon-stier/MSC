#include "MSC_CharacterBase.h"
#include "../GAS/MSC_AbilitySystemComponent.h"
#include "MSC/GAS/MSC_HealthAttributeSet.h"


AMSC_CharacterBase::AMSC_CharacterBase()
{
	MSC_AbilitySystemComponent = CreateDefaultSubobject<UMSC_AbilitySystemComponent>(TEXT("ASC"));
	HealthSet = CreateDefaultSubobject<UMSC_HealthAttributeSet>(TEXT("HealthSet"));
	
	UE_LOG(LogTemp, Warning, TEXT("Base Constructor finished %hhd"), MSC_AbilitySystemComponent == nullptr);
}

void AMSC_CharacterBase::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("Trying to access AbilitySystemComponent. %hhd"), MSC_AbilitySystemComponent == nullptr);

	MSC_AbilitySystemComponent->InitAbilityActorInfo(this, this);
}

UAbilitySystemComponent* AMSC_CharacterBase::GetAbilitySystemComponent() const
{
	return MSC_AbilitySystemComponent;
}
	