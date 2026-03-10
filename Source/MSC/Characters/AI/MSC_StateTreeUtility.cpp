#include "MSC_StateTreeUtility.h"

#include "MSC_CharacterEnemy.h"
#include "StateTreeExecutionContext.h"
#include "MSC/GAS/MSC_AbilitySystemComponent.h"

EStateTreeRunStatus FStateTreeComboAttackTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	
	UE_LOG(LogTemp, Warning, TEXT("Logging Task from AI"));
	InstanceData.Character->DoPunch();
	return EStateTreeRunStatus::Running;
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
