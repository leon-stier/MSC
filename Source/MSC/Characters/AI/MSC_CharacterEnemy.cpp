#include "MSC_CharacterEnemy.h"

#include "MSC/GAS/MSC_AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MSC/Characters/Player/MSC_CharacterPlayer.h"

AMSC_CharacterEnemy::AMSC_CharacterEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->bAllowPhysicsRotationDuringAnimRootMotion = true;
}

void AMSC_CharacterEnemy::BeginPlay()
{
	Super::BeginPlay();
	
	OnAttackCompleted.AddDynamic(this, &AMSC_CharacterEnemy::OnAttackCompletedForward);
	
	if (AMSC_CharacterPlayer* Player = Cast<AMSC_CharacterPlayer>(GetWorld()->GetFirstPlayerController()->GetPawn()))
	{
		if (UAbilitySystemComponent* PlayerASC = Player->GetAbilitySystemComponent())
		{
			PlayerASC->RegisterGameplayTagEvent(
				FGameplayTag::RequestGameplayTag(FName("Event.ContinueCombo.Input")),
				EGameplayTagEventType::NewOrRemoved
			).AddUObject(this, &AMSC_CharacterEnemy::OnPlayerAttackStarted);
		}
	}
}

void AMSC_CharacterEnemy::OnPlayerAttackStarted(FGameplayTag CallbackTag, int32 NewCount)
{
	if (NewCount > 0)
	{
		LastPlayerAttackEventTime = GetWorld()->GetTimeSeconds();
	}
}

bool AMSC_CharacterEnemy::IsPlayerAttacking() const
{
	const float ElapsedSinceLastAttack = GetWorld()->GetTimeSeconds() - LastPlayerAttackEventTime;
	return ElapsedSinceLastAttack < PlayerAttackTimeoutSeconds;
}

bool AMSC_CharacterEnemy::HasEngagedAttackToken() const
{
	return bHasEngagedAttackToken;
}

void AMSC_CharacterEnemy::SetHasEngagedAttackToken(const bool bInHasToken)
{
	bHasEngagedAttackToken = bInHasToken;
}

void AMSC_CharacterEnemy::ReleaseEngagedAttackToken()
{
	if (!bHasEngagedAttackToken)
	{
		return;
	}

	if (AMSC_CharacterPlayer* Player = Cast<AMSC_CharacterPlayer>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Player->ReturnAttackToken();
	}

	bHasEngagedAttackToken = false;
}

void AMSC_CharacterEnemy::SendContinueComboInputEvent()
{
	if (MSC_AbilitySystemComponent)
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			this,
			FGameplayTag::RequestGameplayTag(FName("Event.ContinueCombo.Input")),
			FGameplayEventData()
		);
	}
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
