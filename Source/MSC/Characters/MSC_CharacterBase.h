#pragma once

#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "MSC_CharacterBase.generated.h"

struct FOnAttributeChangeData;
class UGameplayAbility;

UCLASS(Abstract)
class MSC_API AMSC_CharacterBase : public ACharacter, public IAbilitySystemInterface
{
	
	GENERATED_BODY()
public:
	AMSC_CharacterBase();
	
	virtual void BeginPlay() override;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	void OnMoveSpeedChanged(const FOnAttributeChangeData& Data) const;
	
	/* Default Abilities */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Player|GAS", meta = (DisplayName = "Default Abilities", Description = "Default abilities granted to the character at spawn."))
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly);
	TObjectPtr<class UMSC_AbilitySystemComponent> MSC_AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<class UMSC_HealthAttributeSet> HealthSet;
	
	UPROPERTY()
	TObjectPtr<class UMSC_MovementAttributeSet> MovementSet;
	
};
