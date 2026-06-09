#include "MSC_HealthAttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "EngineUtils.h"
#include "GameplayEffectExtension.h"
#include "Kismet/GameplayStatics.h"
#include "MSC/PlayerModel/CombatEventSubsystem.h"
#include "MSC/PlayerModel/CombatEventTypes.h"
#include "MSC/Characters/Player/MSC_CharacterPlayer.h"
#include "MSC/Characters/Player/RespawnManager.h"

UMSC_HealthAttributeSet::UMSC_HealthAttributeSet()
{
	InitHealth(100.0f);
	InitMaxHealth(100.0f);
	OnHealthChanged.Broadcast(this, 0.0f, GetHealth());
}

void UMSC_HealthAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	
}

void UMSC_HealthAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	
	if (Attribute == GetHealthAttribute())
	{
		if (NewValue <= 0.0f)
		{
			FGameplayTagContainer DeathAbilityTagContainer;
			DeathAbilityTagContainer.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Id.Die")));
			GetOwningAbilitySystemComponent()->TryActivateAbilitiesByTag(DeathAbilityTagContainer);

			// Only report telemetry for the player-controlled character
			if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
			{
				AActor* Avatar = ASC->GetAvatarActor();
				if (Avatar && Avatar->IsA<AMSC_CharacterPlayer>())
				{
					if (UCombatEventSubsystem* CombatEvents = UCombatEventSubsystem::Get(this))
					{
						FCombatEvent Event;
						Event.EventType = ECombatEventType::PlayerDied;
						CombatEvents->ReportEvent(Event);
						
						TActorIterator<ARespawnManager> It(GetOwningActor()->GetWorld());
						UE_LOG(LogTemp, Warning, TEXT("Did I get the RespawnManager?"));
						
						if (It)
						{
							It->TriggerRespawn();
						}
					}
				}
			}
		}

		OnHealthChanged.Broadcast(this, OldValue, NewValue);

		// Only report damage telemetry for the player
		if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			AActor* Avatar = ASC->GetAvatarActor();
			if (Avatar && Avatar->IsA<AMSC_CharacterPlayer>())
			{
				if (UCombatEventSubsystem* CombatEvents = UCombatEventSubsystem::Get(this))
				{
					FCombatEvent Event;
					Event.EventType = ECombatEventType::PlayerTookDamage;
					CombatEvents->ReportEvent(Event);
				}
			}
		}
	} else if (Attribute == GetMaxHealthAttribute())
	{
		// When max health changes, broadcast OnHealthChanged so that health bars will update
		const float CurrentHealth = GetHealth();
		OnHealthChanged.Broadcast(this, CurrentHealth, CurrentHealth);
	}
}

void UMSC_HealthAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		if (Data.EffectSpec.Def->GetAssetTags().HasTag(FGameplayTag::RequestGameplayTag(FName("Effects.HitReaction"))))
		{
			AActor* Instigator = Cast<AActor>(
				Data.EffectSpec.GetEffectContext().GetInstigator());
			AActor* Target = Data.Target.GetAvatarActor();
			
			

			FGameplayEventData EventData;
			EventData.Instigator = Instigator;
			EventData.Target = Target;
			EventData.EventMagnitude = -Data.EvaluatedData.Magnitude;

			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
				Target,
				FGameplayTag::RequestGameplayTag(FName("Event.Trigger.HitReaction")),
				EventData
			);
		}
	}
}
