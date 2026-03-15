#pragma once
#include "../MSC_CharacterBase.h"
#include "MSC_CharacterEnemy.generated.h"

DECLARE_DELEGATE(FOnAttackCompletedNative);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAttackCompleted);


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
	
	FOnAttackCompletedNative OnAttackCompletedNative;
	
	UPROPERTY(BlueprintCallable)
	FOnAttackCompleted OnAttackCompleted;
	
private:
	UFUNCTION()
	void OnAttackCompletedForward();
};
