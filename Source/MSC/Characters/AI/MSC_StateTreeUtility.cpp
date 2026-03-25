#include "MSC_StateTreeUtility.h"

#include "AIController.h"
#include "MSC_CharacterEnemy.h"
#include "StateTreeExecutionContext.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeAsyncExecutionContext.h"
#include "Abilities/GameplayAbility.h"
#include "MSC/Characters/Player/MSC_CharacterPlayer.h"
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
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get();
	if (!Enemy) return EStateTreeRunStatus::Failed;

	AAIController* Controller = Cast<AAIController>(Enemy->GetController());
	AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(Enemy, 0);
	if (!Controller || !PlayerActor) return EStateTreeRunStatus::Failed;
	Controller->SetFocus(PlayerActor);

	const FVector DesiredLocation = GetEnGardeLocation(Enemy, PlayerActor, InstanceData.EnGardeDistance);
	const float DistanceToSlot = FVector::Distance(Enemy->GetActorLocation(), DesiredLocation);

	if (DistanceToSlot > InstanceData.DistanceTolerance)
	{
		Controller->MoveToLocation(DesiredLocation, InstanceData.DistanceTolerance, false, true, false, true, nullptr, true);
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeAttackRunTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get();
	if (!Enemy || !Enemy->GetCharacterMovement()) return EStateTreeRunStatus::Failed;

	InstanceData.bHasAttackToken = false;
	if (InstanceData.bAcquireTokenOnEnter)
	{
		if (Enemy->HasEngagedAttackToken())
		{
			// Token is already owned by this enemy for Engaged loop.
		}
		else
		{
			AMSC_CharacterPlayer* Player = Cast<AMSC_CharacterPlayer>(UGameplayStatics::GetPlayerPawn(Enemy, 0));
			if (!Player || !Player->RequestAttackToken()) return EStateTreeRunStatus::Failed;
			InstanceData.bHasAttackToken = true;
		}
	}

	InstanceData.bAttackDelegateBound = false;
	InstanceData.Phase = EStateTreeAttackRunPhase::Rush;
	InstanceData.CachedMoveSpeed = Enemy->GetCharacterMovement()->MaxWalkSpeed;
	Enemy->GetCharacterMovement()->MaxWalkSpeed = InstanceData.RushSpeed;

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeAttackRunTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get();
	if (!Enemy) return EStateTreeRunStatus::Failed;

	AAIController* Controller = Cast<AAIController>(Enemy->GetController());
	AMSC_CharacterPlayer* Player = Cast<AMSC_CharacterPlayer>(UGameplayStatics::GetPlayerPawn(Enemy, 0));
	if (!Controller || !Player) return EStateTreeRunStatus::Failed;
	Controller->SetFocus(Player);

	if (InstanceData.Phase == EStateTreeAttackRunPhase::Rush)
	{
		const FVector EnemyLocation = Enemy->GetActorLocation();
		const FVector PlayerLocation = Player->GetActorLocation();
		FVector Direction = (EnemyLocation - PlayerLocation).GetSafeNormal2D();
		if (Direction.IsNearlyZero())
		{
			Direction = Enemy->GetActorForwardVector().GetSafeNormal2D();
		}

		const FVector AttackPosition = PlayerLocation + Direction * InstanceData.AttackDistance;
		Controller->MoveToLocation(AttackPosition, InstanceData.AttackDistanceTolerance, false, true, false, true, nullptr, true);

		const float DistanceToPlayer = FVector::Distance(EnemyLocation, PlayerLocation);
		if (DistanceToPlayer > InstanceData.AttackDistance + InstanceData.AttackDistanceTolerance)
		{
			return EStateTreeRunStatus::Running;
		}

		Controller->StopMovement();

		if (!Enemy->PunchAbility || !Enemy->MSC_AbilitySystemComponent)
		{
			return EStateTreeRunStatus::Failed;
		}

		const bool bActivated = Enemy->MSC_AbilitySystemComponent->TryActivateAbilityByClass(Enemy->PunchAbility);
		if (!bActivated)
		{
			return EStateTreeRunStatus::Failed;
		}

		Enemy->OnAttackCompletedNative.BindLambda(
			[WeakContext = Context.MakeWeakExecutionContext(), WeakEnemy = TWeakObjectPtr<AMSC_CharacterEnemy>(Enemy), bKeepTokenOnSuccess = InstanceData.bKeepTokenOnSuccess]()
			{
				if (bKeepTokenOnSuccess && WeakEnemy.IsValid())
				{
					WeakEnemy->SetHasEngagedAttackToken(true);
				}
				static_cast<void>(WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded));
			}
		);
		InstanceData.bAttackDelegateBound = true;
		InstanceData.Phase = EStateTreeAttackRunPhase::WaitAttackDone;
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeAttackRunTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get();

	if (Enemy && InstanceData.bAttackDelegateBound)
	{
		Enemy->OnAttackCompletedNative.Unbind();
		InstanceData.bAttackDelegateBound = false;
	}

	if (InstanceData.bHasAttackToken)
	{
		const bool bTokenTransferredToEngaged = Enemy && Enemy->HasEngagedAttackToken();
		if (!bTokenTransferredToEngaged)
		{
			if (AMSC_CharacterPlayer* Player = Cast<AMSC_CharacterPlayer>(UGameplayStatics::GetPlayerPawn(Enemy, 0)))
			{
				Player->ReturnAttackToken();
			}
		}
		InstanceData.bHasAttackToken = false;
	}

	RestoreMovementSpeed(Enemy, InstanceData.CachedMoveSpeed);
	InstanceData.CachedMoveSpeed = 0.0f;
}

EStateTreeRunStatus FStateTreeReleaseEngagedTokenTask::EnterState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	return EStateTreeRunStatus::Running;
}

void FStateTreeReleaseEngagedTokenTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get())
	{
		Enemy->ReleaseEngagedAttackToken();
	}
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

EStateTreeRunStatus FStateTreeRetreatToEnGardeTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get();
	if (!Enemy) return EStateTreeRunStatus::Failed;

	AAIController* Controller = Cast<AAIController>(Enemy->GetController());
	AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(Enemy, 0);
	if (!Controller || !PlayerActor) return EStateTreeRunStatus::Failed;
	Controller->SetFocus(PlayerActor);

	const FVector RetreatLocation = GetEnGardeLocation(Enemy, PlayerActor, InstanceData.EnGardeDistance);
	Controller->MoveToLocation(RetreatLocation, InstanceData.DistanceTolerance, false, true, false, true, nullptr, true);

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
	// have we transitioned from another state?
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// get the instance data
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// set the AI Controller's focus
		InstanceData.Controller->SetFocus(InstanceData.ActorToFaceTowards);
	}

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
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FStateTreeReactiveBlockTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get();
	if (!Enemy) return EStateTreeRunStatus::Failed;

	// Keep blocking while player is attacking
	if (Enemy->IsPlayerAttacking())
	{
		return EStateTreeRunStatus::Running;
	}

	// Player stopped attacking, exit block
	return EStateTreeRunStatus::Succeeded;
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
