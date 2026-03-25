#pragma once
#include "Components/StateTreeComponent.h"
#include "StateTreeTaskBase.h"
#include "StateTreeConditionBase.h"

#include "MSC_StateTreeUtility.generated.h"

class AMSC_CharacterPlayer;
class ACharacter;
class AAIController;
class AMSC_CharacterEnemy;




UENUM()
enum class EStateTreeAttackRunPhase : uint8
{
	Rush,
	WaitAttackDone
};

/**
 *  Instance data for En Garde spacing around the player
 */
USTRUCT()
struct FStateTreeEnGardeInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AMSC_CharacterEnemy> Character;

	UPROPERTY(EditAnywhere, Category = Parameter)
	float EnGardeDistance = 340.0f;

	UPROPERTY(EditAnywhere, Category = Parameter)
	float DistanceTolerance = 90.0f;
};

/**
 *  Keeps AI near the player on a loose ring outside attack range
 */
USTRUCT(meta=(DisplayName="En Garde", Category="Combat"))
struct FStateTreeEnGardeTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeEnGardeInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

/**
 *  Instance data for rush + attack execution
 */
USTRUCT()
struct FStateTreeAttackRunInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AMSC_CharacterEnemy> Character;

	UPROPERTY(EditAnywhere, Category = Parameter)
	float AttackDistance = 150.0f;

	UPROPERTY(EditAnywhere, Category = Parameter)
	float AttackDistanceTolerance = 70.0f;

	UPROPERTY(EditAnywhere, Category = Parameter)
	float RushSpeed = 650.0f;

	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bAcquireTokenOnEnter = true;

	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bKeepTokenOnSuccess = false;

	UPROPERTY(Transient)
	float CachedMoveSpeed = 0.0f;

	UPROPERTY(Transient)
	bool bHasAttackToken = false;

	UPROPERTY(Transient)
	bool bAttackDelegateBound = false;

	UPROPERTY(Transient)
	EStateTreeAttackRunPhase Phase = EStateTreeAttackRunPhase::Rush;
};

/**
 *  Acquires attack token, rushes into range, executes attack, then completes
 */
USTRUCT(meta=(DisplayName="Attack Run", Category="Combat"))
struct FStateTreeAttackRunTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeAttackRunInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT()
struct FStateTreeReleaseEngagedTokenInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AMSC_CharacterEnemy> Character;
};

/**
 *  Attach this to Engaged parent state to release token only when Engaged exits.
 */
USTRUCT(meta=(DisplayName="Release Engaged Token", Category="Combat"))
struct FStateTreeReleaseEngagedTokenTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeReleaseEngagedTokenInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

/**
 *  Instance data for retreating back to En Garde after attacking
 */
USTRUCT()
struct FStateTreeRetreatToEnGardeInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AMSC_CharacterEnemy> Character;

	UPROPERTY(EditAnywhere, Category = Parameter)
	float EnGardeDistance = 340.0f;

	UPROPERTY(EditAnywhere, Category = Parameter)
	float DistanceTolerance = 90.0f;

	UPROPERTY(EditAnywhere, Category = Parameter)
	float RetreatSpeed = 280.0f;

	UPROPERTY(Transient)
	float CachedMoveSpeed = 0.0f;
};


/**
 *  Instance data for reactive blocking in response to player attacks
 */
USTRUCT()
struct FStateTreeReactiveBlockInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AMSC_CharacterEnemy> Character;

	UPROPERTY(EditAnywhere, Category = Parameter)
	float BlockChance = 0.85f;

	UPROPERTY(Transient)
	bool bBlockAbilityActive = false;
};

/**
 *  Activates block ability in response to player attacking
 */
USTRUCT(meta=(DisplayName="Reactive Block", Category="Combat"))
struct FStateTreeReactiveBlockTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeReactiveBlockInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

/**
 *  Condition to check if player is currently attacking
 */
USTRUCT()
struct FStateTreeIsPlayerAttackingConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AMSC_CharacterEnemy> Character;
};
STATETREE_POD_INSTANCEDATA(FStateTreeIsPlayerAttackingConditionInstanceData);

USTRUCT(DisplayName="Is Player Attacking?", Category="Combat")
struct FStateTreeIsPlayerAttackingCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeIsPlayerAttackingConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

/**
 *  Condition to check if enemy is NOT currently attacking (opportunity to attack)
 */
USTRUCT()
struct FStateTreeCanEnemyAttackConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AMSC_CharacterEnemy> Character;
};
STATETREE_POD_INSTANCEDATA(FStateTreeCanEnemyAttackConditionInstanceData);

USTRUCT(DisplayName="Can Enemy Attack?", Category="Combat")
struct FStateTreeCanEnemyAttackCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeCanEnemyAttackConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

USTRUCT(meta=(DisplayName="Retreat To En Garde", Category="Combat"))
struct FStateTreeRetreatToEnGardeTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeRetreatToEnGardeInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};


/**
 *  Instance data struct for the FStateTreeIsCharacterDeadCondition condition
 */
USTRUCT()
struct FStateTreeCharacterDeadConditionInstanceData
{
	GENERATED_BODY()
	
	/** Character to check dead status on */
	UPROPERTY(EditAnywhere, Category = "Context")
	AMSC_CharacterEnemy* Character;
};
STATETREE_POD_INSTANCEDATA(FStateTreeCharacterDeadConditionInstanceData);

/**
 *  StateTree condition to check if the character is dead
 */
USTRUCT(DisplayName="Is Character Dead?", Category="Combat")
struct FStateTreeIsCharacterDeadCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FStateTreeCharacterDeadConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	
	FStateTreeIsCharacterDeadCondition() = default;
	
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
	
};

/**
 *  Instance data struct for the Get Player Info task
 */
USTRUCT()
struct FStateTreeGetPlayerInfoInstanceData
{
	GENERATED_BODY()

	/** Character that owns this task */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<ACharacter> Character;

	/** Character that owns this task */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<ACharacter> TargetPlayerCharacter;

	/** Last known location for the target */
	UPROPERTY(VisibleAnywhere)
	FVector TargetPlayerLocation = FVector::ZeroVector;

	/** Distance to the target */
	UPROPERTY(VisibleAnywhere)
	float DistanceToTarget = 0.0f;
};

/**
 *  StateTree task to get information about the player character
 */
USTRUCT(meta=(DisplayName="GetPlayerInfo", Category="Combat"))
struct FStateTreeGetPlayerInfoTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/* Ensure we're using the correct instance data struct */
	using FInstanceDataType = FStateTreeGetPlayerInfoInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Runs while the owning state is active */
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};

/**
 *  Instance data struct for the Face Towards Actor StateTree task
 */
USTRUCT()
struct FStateTreeFaceActorInstanceData
{
	GENERATED_BODY()

	/** AI Controller that will determine the focused actor */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> Controller;

	/** Actor that will be faced towards */
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<AActor> ActorToFaceTowards;
};

/**
 *  StateTree task to face an AI-Controlled Pawn towards an Actor
 */
USTRUCT(meta=(DisplayName="Face Towards Actor", Category="Combat"))
struct FStateTreeFaceActorTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/* Ensure we're using the correct instance data struct */
	using FInstanceDataType = FStateTreeFaceActorInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Runs when the owning state is entered */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** Runs when the owning state is ended */
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};