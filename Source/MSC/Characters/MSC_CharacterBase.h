#pragma once

#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "MSC_CharacterBase.generated.h"

UCLASS(Abstract)
class MSC_API AMSC_CharacterBase : public ACharacter, public IAbilitySystemInterface
{
	
	GENERATED_BODY()
public:
	AMSC_CharacterBase();
	
	virtual void BeginPlay() override;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly);
	TObjectPtr<class UMSC_AbilitySystemComponent> MSC_AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<class UMSC_HealthAttributeSet> HealthSet;
	
};
