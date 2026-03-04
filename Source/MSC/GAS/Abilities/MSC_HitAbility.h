#pragma once
#include "Abilities/GameplayAbility.h"

UCLASS()
class MSC_API UMSC_HitAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UMSC_HitAbility();
	
	// Montage to play
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	UAnimMontage* AttackMontage;

	// GameplayEffect to apply on hit
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	TSubclassOf<UGameplayEffect> DamageGE;

	// Sphere trace radius
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float HitRadius = 100.f;

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// Called when AnimNotify/GamePlayEvent fires
	UFUNCTION()
	void OnAttackHitEventReceived(FGameplayEventData Payload);

	// Sphere trace logic
	void DoDamageTrace();
};
