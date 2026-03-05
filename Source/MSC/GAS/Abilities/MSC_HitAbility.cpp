#include "MSC_HitAbility.h"

#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

UMSC_HitAbility::UMSC_HitAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UMSC_HitAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, 
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (AttackMontage && ActorInfo->AvatarActor.IsValid())
	{
		ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
		if (Character)
		{
			// Play montage and wait for event
			FGameplayEventData EventData; // optional
			// Using built-in WaitForEvent system in GAS
			UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AttackMontage);
			auto* EventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag(FName("Event.Ability.Trigger")));
			Task->OnCancelled.
			
			AddDynamic(this, &UGameplayAbility::EndAbility);
			Task->OnInterrupted.AddDynamic(this, &UGameplayAbility::EndAbility);
			Task->OnCompleted.AddDynamic(this, &UGameplayAbility::EndAbility);
			Task->ReadyForActivation();
		}
	}
}

void UMSC_HitAbility::OnAttackHitEventReceived(FGameplayEventData Payload)
{
}

void UMSC_HitAbility::DoDamageTrace()
{
}
