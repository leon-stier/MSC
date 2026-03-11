#include "MSC_StateTreeUtility.h"

#include "MSC_CharacterEnemy.h"
#include "StateTreeExecutionContext.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeAsyncExecutionContext.h"

EStateTreeRunStatus FStateTreeComboAttackTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	AMSC_CharacterEnemy* Enemy = InstanceData.Character.Get();
	if (!Enemy) return EStateTreeRunStatus::Failed;
	
	InstanceData.Character->OnAttackCompletedNative.AddLambda(
			[WeakContext = Context.MakeWeakExecutionContext()]()
			{
				WeakContext.FinishTask(EStateTreeFinishTaskType::Succeeded);
			}
		);
	
	return EStateTreeRunStatus::Succeeded;
}

void FStateTreeComboAttackTask::ExitState(FStateTreeExecutionContext& Context,
	const FStateTreeTransitionResult& Transition) const
{
	FStateTreeTaskCommonBase::ExitState(Context, Transition);
}

FText FStateTreeComboAttackTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView,
	const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FStateTreeTaskCommonBase::GetDescription(ID, InstanceDataView, BindingLookup, Formatting);
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
	InstanceData.DistanceToTarget = FVector::Distance(InstanceData.TargetPlayerLocation, InstanceData.Character->GetActorLocation());

	return EStateTreeRunStatus::Running;
}

FText FStateTreeGetPlayerInfoTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView,
	const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	return FText::FromString("<b>Get Player Info</b>");
}
