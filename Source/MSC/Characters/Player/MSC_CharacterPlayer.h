#pragma once
#include "../MSC_CharacterBase.h"
#include "MSC_CharacterPlayer.generated.h"

class UInputMappingContext;
class AMSC_CharacterEnemy;
class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;


UCLASS()
class MSC_API AMSC_CharacterPlayer : public AMSC_CharacterBase 
{
	GENERATED_BODY()

public:
	AMSC_CharacterPlayer();
	
	virtual void Tick(float DeltaSeconds) override;
	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();
	
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoPunch();
	
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoBlockStart();
	
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoBlockEnd();
	
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoParry();
	
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoDodge();
	
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLockTarget();

	UFUNCTION()
	void HandleLockSwitchInput(const FInputActionValue& Value);

	UFUNCTION()
	void OnLockSwitchReleased(const FInputActionValue& Value);
	
	UFUNCTION()
	void UpdateLockOnRotation(float DetlaTime);
	
	UFUNCTION(BlueprintCallable)
	bool RequestAttackToken();
	
	UFUNCTION(BlueprintCallable)
	void ReturnAttackToken();
	
	UFUNCTION(BlueprintCallable)
	AMSC_CharacterEnemy* GetHitTarget() const;
	
	UPROPERTY()
	int MaxAllowedAttackers = 1;
	
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> PunchAbility;
	
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> BlockAbility;
	
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> ParryAbility;
	
	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> DodgeAbility;
	
	UPROPERTY()
	float TargetLockDistance = 500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "LockOn", meta = (ClampMin = "0.0"))
	float TargetLockSimilarDistanceThreshold = 75.0f;

	UPROPERTY(EditDefaultsOnly, Category = "LockOn", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LockSwitchDeadzone = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category = "LockOn", meta = (ClampMin = "0.0"))
	float LockSwitchCooldownSeconds = 0.2f;
	
protected:
	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;
	
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* PunchAction;
	
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* BlockAction;
	
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ParryAction;
	
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* DodgeAction;
	
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LockTargetAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LockSwitchAction;
	
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
	
private:
	void OnTargetDied(const FGameplayTag Tag, int32 NewCount);
	
	void UnlockTarget();
	void SetLockedTarget(AMSC_CharacterEnemy* NewTarget);
	AMSC_CharacterEnemy* FindBestSideLockTarget(int32 SideSign) const;
	
	FDelegateHandle DeadTagEventHandle;
	
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	UPROPERTY()
	AMSC_CharacterEnemy* HitTarget;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* LookMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	int32 LookMappingContextPriority = 0;
	
	int CurrentAttackerTokens = MaxAllowedAttackers;
	float LastLockSwitchTime = -1000.0f;
	bool bLockSwitchAxisLatched = false;
	
};
