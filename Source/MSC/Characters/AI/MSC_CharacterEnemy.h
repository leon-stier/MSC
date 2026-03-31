#pragma once
#include "../MSC_CharacterBase.h"
#include "Containers/List.h"
#include "MSC_CharacterEnemy.generated.h"

class AMSC_CharacterPlayer;
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
	void SendContinueComboInputEvent();
	bool IsPlayerAttacking() const;
	
	bool HasEngagedAttackToken() const;
	void SetHasEngagedAttackToken(bool bInHasToken);
	void ReleaseEngagedAttackToken();

	UFUNCTION(BlueprintCallable, Category = "LockOn")
	void SetIsLockTarget(bool bInIsLockTarget);

	UFUNCTION(BlueprintPure, Category = "LockOn")
	bool IsLockTarget() const { return bIsLockTarget; }

	UFUNCTION(BlueprintImplementableEvent, Category = "LockOn")
	void BP_OnLockTargetStateChanged(bool bNowLocked);
	
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> PunchAbility;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> BlockAbility;
	
	FOnAttackCompletedNative OnAttackCompletedNative;
	
	UPROPERTY(BlueprintCallable)
	FOnAttackCompleted OnAttackCompleted;

	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = 0.1f))
	float PlayerAttackTimeoutSeconds = 5.0f;
	
private:
	UFUNCTION()
	void OnAttackCompletedForward();
	
	UFUNCTION()
	void OnStunnedTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	UFUNCTION()
	void OnPlayerAttackStarted(FGameplayTag CallbackTag, int32 NewCount);

	UPROPERTY(Transient)
	float LastPlayerAttackEventTime = 0.0f;

	UPROPERTY(Transient)
	bool bHasEngagedAttackToken = false;

	UPROPERTY(Transient)
	bool bIsLockTarget = false;

};
