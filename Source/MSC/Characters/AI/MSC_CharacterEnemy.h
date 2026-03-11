#pragma once
#include "../MSC_CharacterBase.h"
#include "MSC_CharacterEnemy.generated.h"

UCLASS(abstract)
class MSC_API AMSC_CharacterEnemy : public AMSC_CharacterBase
{
	GENERATED_BODY()
public:
	AMSC_CharacterEnemy();
	
	virtual void BeginPlay() override;
	
	void DoPunch();
	
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> PunchAbility;
	
	DECLARE_MULTICAST_DELEGATE(FOnAttackCompletedNative);
	FOnAttackCompletedNative OnAttackCompletedNative;
	
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttackCompleted);
	UPROPERTY(BlueprintAssignable)
	FOnAttackCompleted OnAttackCompleted;
	
private:
	UFUNCTION()
	void OnAttackCompletedForward();
};
