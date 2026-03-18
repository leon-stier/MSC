#include "MSC_HealthAttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectExtension.h"

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
		OnHealthChanged.Broadcast(this, OldValue, NewValue);
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

			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
				Target,
				FGameplayTag::RequestGameplayTag(FName("Event.Trigger.HitReaction")),
				EventData
			);
		}
		if (Health.GetCurrentValue() <= 0.f)
		{
			// Apply dead tag to the ASC
			Data.Target.AddLooseGameplayTag(
				FGameplayTag::RequestGameplayTag(FName("State.Dead"))
			);
		}
	}
}
