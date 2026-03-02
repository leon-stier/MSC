#pragma once
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "MSC_HealthAttributeSet.generated.h"

UCLASS()
class MSC_API UMSC_HealthAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	UMSC_HealthAttributeSet();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UMSC_HealthAttributeSet, Health);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UMSC_HealthAttributeSet, MaxHealth);
	
};
