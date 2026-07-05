#include "ScoreProcessor.h"

#include "CombatEventTypes.h"

void UScoreProcessor::ProcessTelemetryEvent(const FCombatEvent& Event)
{
	if (bIsFrozen) return;
	// Look up discrete event weight (w_j)
	float EventWeight = 0.f;
	if (const float* Weight = EventWeights.Find(Event.EventType))
	{
		EventWeight = *Weight;
	}

	// Apply frustration update with discrete event weight
	// DeltaTime is 0 here since Tick handles the continuous part
	UpdateFrustrationScore(0.f, EventWeight);

	// Update raw metrics
	switch (Event.EventType)
	{
	case ECombatEventType::PlayerDied:
		IncrementAppend(Metrics.Deaths);
		break;

	case ECombatEventType::PlayerTookDamage:
		IncrementAppend(Metrics.DamageTakenCount);
		break;

	case ECombatEventType::PlayerSuccessfulCombo:
		IncrementAppend(Metrics.SuccessfulCombos);
		break;

	case ECombatEventType::PlayerSuccessfulParry:
		IncrementAppend(Metrics.SuccessfulParries);
		break;

	case ECombatEventType::PlayerSuccessfulBlock:
		IncrementAppend(Metrics.SuccessfulBlocks);
		break;

	case ECombatEventType::PlayerSuccessfulDodge:
		IncrementAppend(Metrics.SuccessfulDodges);
		break;

	case ECombatEventType::PlayerSuccessfulHit:
		IncrementAppend(Metrics.SuccessfulHits);
		DeactivateInactivitySignal();
		break;

		// Unused
	case ECombatEventType::PlayerAbilitySuccessful:
		IncrementAppend(Metrics.AbilityActivations.FindOrAdd(Event.AbilityTag));
		break;

	case ECombatEventType::PlayerAbilityMissed:
		IncrementAppend(Metrics.AbilitiesMissed);
		IncrementAppend(Metrics.AbilityMisses.FindOrAdd(Event.AbilityTag));
		break;

	case ECombatEventType::PlayerInactive:
		ActivateInactivitySignal();
		break;

	case ECombatEventType::PlayerUnassignedInput:
		IncrementAppend(Metrics.UnassignedInputs);
		break;


	default: break;
	}
}

void UScoreProcessor::ProcessOpportunity(ECombatSituation OpportunityType, FGameplayTag ActedAbilityTag)
{
	if (bIsFrozen) return;
	IncrementAppend(Metrics.TotalOpportunities);
	const TArray<FGameplayTag> ActionTags = GetOpportunityActionTags(OpportunityType);
	if (ActionTags.IsEmpty())
	{
		return;
	}
	bool bActed = false;
	for (const FGameplayTag& ActionTag : ActionTags)
	{
		const float Outcome = (ActedAbilityTag.IsValid() && ActionTag == ActedAbilityTag) ? 1.f : 0.f;
		if (Outcome > 0.f)
		{
			bActed = true;
		}
		ApplyOpportunityOutcome(ActionTag, Outcome);
	}
	if (bActed) IncrementAppend(Metrics.ActedOpportunities);
}

void UScoreProcessor::ApplyOpportunityOutcome(FGameplayTag AbilityTag, float Outcome)
{
	if (!AbilityTag.IsValid())
	{
		return;
	}

	TArray<TPair<float, float>>& ProficiencyArray = Metrics.InputProficiency.FindOrAdd(AbilityTag, {{GetInitTime(), 0.5f}});
	float CurrentProficiency = ProficiencyArray.Last().Value;
	CurrentProficiency = OpportunityAlpha * CurrentProficiency + (1.f - OpportunityAlpha) * Outcome;
	
	Append(ProficiencyArray, CurrentProficiency);
	
	auto Baseline = BaselineMetrics.InputProficiency.Find(AbilityTag);
	float BaselineValue = (Baseline == nullptr || Baseline->IsEmpty()) ? 0.f : Baseline->Last().Value;

	if (CurrentProficiency - BaselineValue < ForgottenInputDriftThreshold)
	{
		UE_LOG(LogTemp, Warning, TEXT("Forgotten input: %s"), *AbilityTag.ToString());
		ForgottenInputs.Add(AbilityTag);
		CheckHintConditions();
	}
}

TArray<FGameplayTag> UScoreProcessor::GetOpportunityActionTags(ECombatSituation OpportunityType)
{
	switch (OpportunityType)
	{
	case ECombatSituation::NormalAttack:
		return {
			FGameplayTag::RequestGameplayTag(FName("Ability.Id.Block")),
			FGameplayTag::RequestGameplayTag(FName("Ability.Id.Parry")),
			FGameplayTag::RequestGameplayTag(FName("Ability.Id.Dodge"))
		};

	case ECombatSituation::UndodgeableAttack:
		return {
			FGameplayTag::RequestGameplayTag(FName("Ability.Id.Block")),
			FGameplayTag::RequestGameplayTag(FName("Ability.Id.Parry"))
		};

	case ECombatSituation::UnblockableAttack:
		return {
			FGameplayTag::RequestGameplayTag(FName("Ability.Id.Dodge"))
		};

	case ECombatSituation::HitWindow:
		return {
			FGameplayTag::RequestGameplayTag(FName("Ability.Id.Hit"))
		};

	case ECombatSituation::ComboWindow:
		return {
			FGameplayTag::RequestGameplayTag(FName("Ability.Id.Combo"))
		};

	default:
		return {};
	}
}

void UScoreProcessor::CheckHintConditions()
{
	if (Metrics.FrustrationScore.Last().Value < FrustrationHintThreshold || ForgottenInputs.IsEmpty()) return;
	OnInputForgotten.Broadcast(ForgottenInputs.Pop());
}

void UScoreProcessor::ActivateInactivitySignal()
{
	if (!InactivitySignal.bActive)
	{
		InactivitySignal.bActive = true;
		InactivitySignal.Duration = 0.f;
	}
}

void UScoreProcessor::DeactivateInactivitySignal()
{
	InactivitySignal.bActive = false;
	InactivitySignal.Duration = 0.f;
}

const FCombatMetrics& UScoreProcessor::GetMetrics() const
{
	return Metrics;
}

const FCombatMetrics& UScoreProcessor::GetBaselineMetrics() const
{
	return BaselineMetrics;
}

void UScoreProcessor::Tick(float DeltaTime)
{
	if (InactivitySignal.bActive)
	{
		if (bIsFrozen)
		{
			InactivitySignal.bActive = false;
			InactivitySignal.Duration = 0.f;
			return;
		}
		InactivitySignal.Duration += DeltaTime;
	}

	// Apply decay and duration penalties with no discrete events this tick
	UpdateFrustrationScore(DeltaTime, 0.f);
}

void UScoreProcessor::Reset()
{
	Metrics.Clear();
	BaselineMetrics.Clear();
	Metrics.Reset();
	BaselineMetrics.Reset();
}

void UScoreProcessor::UpdateFrustrationScore(const float DeltaTime, const float DiscreteEventWeight)
{
	if (bIsFrozen) return;
	
	float CurrentFrustrationScore = Metrics.FrustrationScore.Last().Value;
	// F(t+dt) = F(t) * e^(-lambda * dt) + sum(w_j) + sum(r_k(t) * dt)
	const float DecayedScore = CurrentFrustrationScore * FMath::Exp(-DecayConstant * DeltaTime);
	const float DurationPenalty = InactivitySignal.GetPenaltyRate() * DeltaTime;

	const float ScaledEventWeight = DiscreteEventWeight / FrustrationScoreMax;
	const float ScaledDurationPenalty = DurationPenalty / FrustrationScoreMax;

	CurrentFrustrationScore = DecayedScore + ScaledEventWeight + ScaledDurationPenalty;
	CurrentFrustrationScore = FMath::Clamp(CurrentFrustrationScore, 0.f, 1.f);
	Append(Metrics.FrustrationScore, CurrentFrustrationScore);
	CheckHintConditions();
}
