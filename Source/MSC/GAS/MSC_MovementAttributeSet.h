#pragma once
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "MSC_HealthAttributeSet.h"
#include "MSC_MovementAttributeSet.generated.h"

UCLASS()
class MSC_API UMSC_MovementAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	UMSC_MovementAttributeSet();
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS_BASIC(UMSC_MovementAttributeSet, MoveSpeed);
};
