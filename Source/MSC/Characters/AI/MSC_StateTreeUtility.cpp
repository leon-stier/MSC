#include "MSC_StateTreeUtility.h"

#include "AIController.h"
#include "MSC_CharacterEnemy.h"
#include "StateTreeExecutionContext.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeAsyncExecutionContext.h"
#include "Abilities/GameplayAbility.h"
#include "MSC/Characters/Player/MSC_CharacterPlayer.h"
#include "MSC/Characters/MSC_CharacterBase.h"
#include "MSC/GAS/MSC_AbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
	FVector GetEnGardeLocation(const AMSC_CharacterEnemy* Enemy, const AActor* PlayerActor, const float EnGardeDistance)
	{
		const FVector PlayerLocation = PlayerActor->GetActorLocation();
		const float SlotAngleRadians = FMath::DegreesToRadians(static_cast<float>(Enemy->GetUniqueID() % 360));
		const FVector SlotDirection(FMath::Cos(SlotAngleRadians), FMath::Sin(SlotAngleRadians), 0.0f);
		return PlayerLocation + SlotDirection * EnGardeDistance;
	}

	void RestoreMovementSpeed(AMSC_CharacterEnemy* Enemy, const float CachedSpeed)
	{
		if (Enemy && Enemy->GetCharacterMovement() && CachedSpeed > 0.0f)
		{
			Enemy->GetCharacterMovement()->MaxWalkSpeed = CachedSpeed;
		}
	}
}

EStateTreeRunStatus FStateTreeEnGardeTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	(void)DeltaTime;

	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get();
	if (!Enemy) return EStateTreeRunStatus::Failed;

	AAIController* Controller = Cast<AAIController>(Enemy->GetController());
	AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(Enemy, 0);
	if (!Controller || !PlayerActor) return EStateTreeRunStatus::Failed;

	Controller->SetFocus(PlayerActor);

	const float DistanceToPlayer = FVector::Dist2D(
		Enemy->GetActorLocation(),
		PlayerActor->GetActorLocation());

	const float MaxAllowedDistance = InstanceData.EnGardeDistance + InstanceData.DistanceTolerance;

	if (DistanceToPlayer > MaxAllowedDistance)
	{
		// Move in until we're within en garde distance.
		Controller->MoveToActor(
			PlayerActor,
			InstanceData.EnGardeDistance, // acceptance radius around player
			true, true, false, nullptr, true);

		return EStateTreeRunStatus::Running;
	}

	// Inside/under en garde range: hold position (do not back away).
	Controller->StopMovement();
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeRequestTokenTask::EnterState(FStateTreeExecutionContext& Context,
                                                           const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get();
	if (!Enemy)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (Enemy->HasEngagedAttackToken())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	AMSC_CharacterPlayer* Player = Cast<AMSC_CharacterPlayer>(UGameplayStatics::GetPlayerPawn(Enemy, 0));
	if (!Player)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (!Player->RequestAttackToken())
	{
		return EStateTreeRunStatus::Failed;
	}

	Enemy->SetHasEngagedAttackToken(true);
	return EStateTreeRunStatus::Succeeded;
}

void FStateTreeRequestTokenTask::ExitState(FStateTreeExecutionContext& Context,
                                           const FStateTreeTransitionResult& Transition) const
{
	// Token release is handled by explicit release/death states.
}

EStateTreeRunStatus FStateTreeReleaseTokenTask::EnterState(FStateTreeExecutionContext& Context,
                                                           const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get())
	{
		Enemy->ReleaseEngagedAttackToken();
	}

	return EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FStateTreeMaintainEngageDistanceTask::EnterState(FStateTreeExecutionContext& Context,
                                                                     const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get();
	if (!Enemy || !Enemy->GetCharacterMovement())
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.CachedMoveSpeed = Enemy->GetCharacterMovement()->MaxWalkSpeed;
	Enemy->GetCharacterMovement()->MaxWalkSpeed = InstanceData.EngageMoveSpeed;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeMaintainEngageDistanceTask::Tick(FStateTreeExecutionContext& Context,
                                                               const float DeltaTime) const
{
	(void)DeltaTime;

	const FInstanceDataType InstanceData = Context.GetInstanceData(*this);
	AMSC_CharacterEnemy* const Enemy = InstanceData.Character.Get();
	if (!Enemy)
	{
		return EStateTreeRunStatus::Failed;
	}

	AAIController* Controller = Cast<AAIController>(Enemy->GetController());
	AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(Enemy, 0);
	if (!Controller || !PlayerActor)
	{
		return EStateTreeRunStatus::Failed;
	}

	Controller->SetFocus(PlayerActor);

	const float DistanceToPlayer = FVector::Dist2D(Enemy->GetActorLocation(), PlayerActor->GetActorLocation());
	const float MinDistance = FMath::Max(0.0f, InstanceData.PreferredDistance - InstanceData.DistanceTolerance);
	const float MaxDistance = InstanceData.PreferredDistance + InstanceData.DistanceTolerance;

	if (DistanceToPlayer > MaxDistance)
	{
		Controller->MoveToActor(PlayerActor, InstanceData.PreferredDistance, true, true, false, nullptr, true);
		return EStateTreeRunStatus::Running;
	}

	if (DistanceToPlayer < MinDistance)
	{
		FVector MoveAwayDir = Enemy->GetActorLocation() - PlayerActor->GetActorLocation();
		MoveAwayDir.Z = 0.0f;
		MoveAwayDir = MoveAwayDir.GetSafeNormal();
		if (MoveAwayDir.IsNearlyZero())
		{
			MoveAwayDir = Enemy->GetActorForwardVector().GetSafeNormal2D();
		}

		const FVector RetreatLocation = Enemy->GetActorLocation() + MoveAwayDir * (MinDistance - DistanceToPlayer +
			InstanceData.DistanceTolerance);
		Controller->MoveToLocation(RetreatLocation, InstanceData.DistanceTolerance, false, true, false, true, nullptr,
		                           true);
		return EStateTreeRunStatus::Running;
	}

	Controller->StopMovement();
	return EStateTreeRunStatus::Running;
}

void FStateTreeMaintainEngageDistanceTask::ExitState(FStateTreeExecutionContext& Context,
                                                     const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get();
	if (AAIController* Controller = Enemy ? Cast<AAIController>(Enemy->GetController()) : nullptr)
	{
		Controller->StopMovement();
	}
	RestoreMovementSpeed(Enemy, InstanceData.CachedMoveSpeed);
	InstanceData.CachedMoveSpeed = 0.0f;
}

EStateTreeRunStatus FStateTreeComboAttackTask::EnterState(FStateTreeExecutionContext& Context,
                                                          const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get();
	if (!Enemy) return EStateTreeRunStatus::Failed;


	bool Succeeded = false;

	if (Enemy->PunchAbility && Enemy->MSC_AbilitySystemComponent)
	{
		Succeeded = Enemy->MSC_AbilitySystemComponent->TryActivateAbilityByClass(Enemy->PunchAbility);
	}
	if (!Succeeded)
	{
		return EStateTreeRunStatus::Failed;
	}

	Enemy->OnAttackCompletedNative.BindLambda(
		[WeakContext = Context.MakeWeakExecutionContext()]()
		{
			WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
		}
	);

	return EStateTreeRunStatus::Running;
}

void FStateTreeComboAttackTask::ExitState(FStateTreeExecutionContext& Context,
                                          const FStateTreeTransitionResult& Transition) const
{
	FStateTreeTaskCommonBase::ExitState(Context, Transition);
}

FText FStateTreeComboAttackTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView,
                                                const IStateTreeBindingLookup& BindingLookup,
                                                EStateTreeNodeFormatting Formatting) const
{
	return FStateTreeTaskCommonBase::GetDescription(ID, InstanceDataView, BindingLookup, Formatting);
}

EStateTreeRunStatus FStateTreeRetreatToEnGardeTask::EnterState(FStateTreeExecutionContext& Context,
                                                               const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get();
	if (!Enemy || !Enemy->GetCharacterMovement()) return EStateTreeRunStatus::Failed;

	InstanceData.CachedMoveSpeed = Enemy->GetCharacterMovement()->MaxWalkSpeed;
	Enemy->GetCharacterMovement()->MaxWalkSpeed = InstanceData.RetreatSpeed;

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeRetreatToEnGardeTask::Tick(FStateTreeExecutionContext& Context,
                                                         const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get();
	if (!Enemy) return EStateTreeRunStatus::Failed;

	AAIController* Controller = Cast<AAIController>(Enemy->GetController());
	AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(Enemy, 0);
	if (!Controller || !PlayerActor) return EStateTreeRunStatus::Failed;
	Controller->SetFocus(PlayerActor);

	const FVector RetreatLocation = GetEnGardeLocation(Enemy, PlayerActor, InstanceData.EnGardeDistance);
	Controller->MoveToLocation(RetreatLocation, InstanceData.DistanceTolerance, false, true, false, true, nullptr,
	                           true);

	const float DistanceToSlot = FVector::Distance(Enemy->GetActorLocation(), RetreatLocation);
	if (DistanceToSlot <= InstanceData.DistanceTolerance)
	{
		Controller->StopMovement();
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeRetreatToEnGardeTask::ExitState(FStateTreeExecutionContext& Context,
                                               const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	RestoreMovementSpeed(InstanceData.Character.Get(), InstanceData.CachedMoveSpeed);
	InstanceData.CachedMoveSpeed = 0.0f;
}

bool FStateTreeIsCharacterDeadCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.Character)
	{
		return false;
	}

	return InstanceData.Character->IsDead();
}

bool FStateTreeIsPlayerDeadCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.Character)
	{
		return false;
	}

	const AMSC_CharacterBase* PlayerCharacter = Cast<AMSC_CharacterBase>(
		UGameplayStatics::GetPlayerPawn(InstanceData.Character.Get(), 0));
	return PlayerCharacter ? PlayerCharacter->IsDead() : false;
}

EStateTreeRunStatus FStateTreeGetPlayerInfoTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	// get the instance data
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// get the character possessed by the first local player
	InstanceData.TargetPlayerCharacter = Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(InstanceData.Character, 0));

	// do we have a valid target?
	if (InstanceData.TargetPlayerCharacter)
	{
		// update the last known location
		InstanceData.TargetPlayerLocation = InstanceData.TargetPlayerCharacter->GetActorLocation();
	}

	// update the distance
	InstanceData.DistanceToTarget = FVector::Distance(InstanceData.TargetPlayerLocation,
	                                                  InstanceData.Character->GetActorLocation());
	return EStateTreeRunStatus::Running;
}

FText FStateTreeGetPlayerInfoTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView,
                                                  const IStateTreeBindingLookup& BindingLookup,
                                                  EStateTreeNodeFormatting Formatting) const
{
	return FText::FromString("<b>Get Player Info</b>");
}

EStateTreeRunStatus FStateTreeFaceActorTask::EnterState(FStateTreeExecutionContext& Context,
                                                        const FStateTreeTransitionResult& Transition) const
{
	// get the instance data
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// set the AI Controller's focus
	InstanceData.Controller->SetFocus(InstanceData.ActorToFaceTowards);

	return EStateTreeRunStatus::Running;
}

void FStateTreeFaceActorTask::ExitState(FStateTreeExecutionContext& Context,
                                        const FStateTreeTransitionResult& Transition) const
{
	// have we transitioned to another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// clear the AI Controller's focus
		InstanceData.Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

#if WITH_EDITOR
FText FStateTreeFaceActorTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView,
                                              const IStateTreeBindingLookup& BindingLookup,
                                              EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/)
const
{
	return FText::FromString("<b>Face Towards Actor</b>");
}
#endif // WITH_EDITOR

bool FStateTreeIsPlayerAttackingCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.Character) return false;
	return InstanceData.Character->IsPlayerAttacking();
}

bool FStateTreeCanEnemyAttackCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.Character) return false;
	return !InstanceData.Character->IsPlayerAttacking();
}

EStateTreeRunStatus FStateTreeReactiveBlockTask::EnterState(FStateTreeExecutionContext& Context,
                                                            const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get();
	if (!Enemy) return EStateTreeRunStatus::Failed;

	// Roll for block chance
	if (FMath::FRand() > InstanceData.BlockChance)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (!Enemy->BlockAbility || !Enemy->MSC_AbilitySystemComponent)
	{
		return EStateTreeRunStatus::Failed;
	}

	const bool bActivated = Enemy->MSC_AbilitySystemComponent->TryActivateAbilityByClass(Enemy->BlockAbility);
	if (!bActivated)
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.bBlockAbilityActive = true;
	InstanceData.ElapsedBlockTime = 0.0f;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeReactiveBlockTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get();
	if (!Enemy) return EStateTreeRunStatus::Failed;

	InstanceData.ElapsedBlockTime += DeltaTime;
	if (InstanceData.ElapsedBlockTime >= InstanceData.MaxBlockDuration)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	return Enemy->IsPlayerAttacking() ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

void FStateTreeReactiveBlockTask::ExitState(FStateTreeExecutionContext& Context,
                                            const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get();

	if (Enemy && InstanceData.bBlockAbilityActive)
	{
		if (Enemy->MSC_AbilitySystemComponent)
		{
			FGameplayTagContainer BlockTags;
			BlockTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Id.Block")));
			Enemy->MSC_AbilitySystemComponent->CancelAbilities(&BlockTags);
		}
		InstanceData.bBlockAbilityActive = false;
	}
}
