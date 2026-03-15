#include "MSC_CharacterBase.h"
#include "../GAS/MSC_AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MSC/GAS/MSC_HealthAttributeSet.h"
#include "MSC/GAS/MSC_MovementAttributeSet.h"


AMSC_CharacterBase::AMSC_CharacterBase()
{
	MSC_AbilitySystemComponent = CreateDefaultSubobject<UMSC_AbilitySystemComponent>(TEXT("ASC"));
	HealthSet = CreateDefaultSubobject<UMSC_HealthAttributeSet>(TEXT("HealthSet"));
	MovementSet = CreateDefaultSubobject<UMSC_MovementAttributeSet>(TEXT("MovementSet"));
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
	MSC_AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UMSC_MovementAttributeSet::GetMoveSpeedAttribute()).AddUObject(this, &AMSC_CharacterBase::OnMoveSpeedChanged);
	GetCharacterMovement()->MaxWalkSpeed = MovementSet->GetMoveSpeed();
}

UAbilitySystemComponent* AMSC_CharacterBase::GetAbilitySystemComponent() const
{
	return MSC_AbilitySystemComponent;
}

void AMSC_CharacterBase::OnMoveSpeedChanged(const FOnAttributeChangeData& Data) const
{
	GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
}

bool AMSC_CharacterBase::IsDead() const
{
	return MSC_AbilitySystemComponent->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Dead")));
}
	