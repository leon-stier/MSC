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

USTRUCT()
struct FStateTreeTokenInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AMSC_CharacterEnemy> Character;
};

USTRUCT(meta=(DisplayName="Request Token", Category="Combat"))
struct FStateTreeRequestTokenTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()
	
	using FInstanceDataType = FStateTreeTokenInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

USTRUCT(meta=(DisplayName="Release Token", Category="Combat"))
struct FStateTreeReleaseTokenTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeTokenInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

/**
 *  Instance data for keeping enemy in close engaged range around player
 */
USTRUCT()
struct FStateTreeMaintainEngageDistanceInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AMSC_CharacterEnemy> Character;

	UPROPERTY(EditAnywhere, Category = Parameter)
	float PreferredDistance = 170.0f;

	UPROPERTY(EditAnywhere, Category = Parameter)
	float DistanceTolerance = 55.0f;

	UPROPERTY(EditAnywhere, Category = Parameter)
	float EngageMoveSpeed = 560.0f;

	UPROPERTY(Transient)
	float CachedMoveSpeed = 0.0f;
};

/**
 *  Keeps enemy near the player while engaged so attack/block loop stays in range
 */
USTRUCT(meta=(DisplayName="Maintain Engage Distance", Category="Combat"))
struct FStateTreeMaintainEngageDistanceTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreeMaintainEngageDistanceInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

/**
 *  Instance data for attack execution
 */
USTRUCT()
struct FStateTreeAttackInstanceData
{
	GENERATED_BODY()

	/** Character that will perform the attack */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AMSC_CharacterEnemy> Character;
};

/**
 *  StateTree task to perform a combo attack
 */
USTRUCT(meta=(DisplayName="Combo Attack", Category="Combat"))
struct FStateTreeComboAttackTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/* Ensure we're using the correct instance data struct */
	using FInstanceDataType = FStateTreeAttackInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** Runs when the owning state is entered */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** Runs when the owning state is ended */
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
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

	UPROPERTY(EditAnywhere, Category = Parameter)
	float MaxBlockDuration = 0.7f;

	UPROPERTY(Transient)
	bool bBlockAbilityActive = false;

	UPROPERTY(Transient)
	float ElapsedBlockTime = 0.0f;
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

USTRUCT()
struct FStateTreePlayerDeadConditionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AMSC_CharacterEnemy> Character;
};
STATETREE_POD_INSTANCEDATA(FStateTreePlayerDeadConditionInstanceData);

USTRUCT(DisplayName="Is Player Dead?", Category="Combat")
struct FStateTreeIsPlayerDeadCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FStateTreePlayerDeadConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

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