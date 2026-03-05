#pragma once
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "MSC_HealthAttributeSet.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAttributeChangedEvent, UAttributeSet*, AttributeSet, float, OldValue, float, NewValue);

UCLASS()
class MSC_API UMSC_HealthAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	UMSC_HealthAttributeSet();
	
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UMSC_HealthAttributeSet, Health);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UMSC_HealthAttributeSet, MaxHealth);
	
	UPROPERTY(BlueprintAssignable)
	FAttributeChangedEvent OnHealthChanged;
};
