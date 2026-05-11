#include "ScoreProcessor.h"

#include "CombatEventTypes.h"

void UScoreProcessor::Tick(float DeltaTime)
{
	if (InactivitySignal.bActive)
	{
		InactivitySignal.Duration += DeltaTime;
	}

	// Apply decay and duration penalties with no discrete events this tick
	UpdateFrustrationScore(DeltaTime, 0.f);
}

void UScoreProcessor::UpdateFrustrationScore(const float DeltaTime, const float DiscreteEventWeight)
{
	// F(t+dt) = F(t) * e^(-lambda * dt) + sum(w_j) + sum(r_k(t) * dt)
	const float DecayedScore = Metrics.FrustrationScore * FMath::Exp(-DecayConstant * DeltaTime);
	const float DurationPenalty = InactivitySignal.GetPenaltyRate() * DeltaTime;

	Metrics.FrustrationScore = DecayedScore + DiscreteEventWeight + DurationPenalty;

	// Clamp to zero — frustration can't go negative
	Metrics.FrustrationScore = FMath::Max(0.f, Metrics.FrustrationScore);
}

void UScoreProcessor::ProcessEvent(const FCombatEvent& Event)
{
	// Look up discrete event weight (w_j)
	float EventWeight = 0.f;
	if (const float* Weight = EventWeights.Find(Event.EventType))
	{
		EventWeight = *Weight;
	}

	// Apply frustration update with discrete event weight
	// DeltaTime is 0 here since Tick handles the continuous part
	UpdateFrustrationScore(0.f, EventWeight);

	// Update raw metrics and proficiency
	switch (Event.EventType)
	{
	case ECombatEventType::PlayerDied:
		Metrics.Deaths++;
		break;

	case ECombatEventType::PlayerTookDamage:
		Metrics.DamageTakenCount++;
		break;

	case ECombatEventType::PlayerSuccessfulCombo:
		Metrics.SuccessfulCombos++;
		break;

	case ECombatEventType::PlayerSuccessfulParry:
		Metrics.SuccessfulParries++;
		UpdateProficiency(Event.AbilityTag, true);
		break;

	case ECombatEventType::PlayerSuccessfulBlock:
		Metrics.SuccessfulBlocks++;
		UpdateProficiency(Event.AbilityTag, true);
		break;

	case ECombatEventType::PlayerSuccessfulDodge:
		Metrics.SuccessfulDodges++;
		UpdateProficiency(Event.AbilityTag, true);
		break;

	case ECombatEventType::PlayerSuccessfulHit:
		Metrics.SuccessfulHits++;
		UpdateProficiency(Event.AbilityTag, true);
		DeactivateInactivitySignal();
		break;

	case ECombatEventType::PlayerAbilitySuccessful:
		Metrics.AbilityActivations.FindOrAdd(Event.AbilityTag)++;
		UpdateProficiency(Event.AbilityTag, true);
		break;

	case ECombatEventType::PlayerAbilityMissed:
		Metrics.AbilitiesMissed++;
		Metrics.AbilityMisses.FindOrAdd(Event.AbilityTag)++;
		UpdateProficiency(Event.AbilityTag, false);
		break;

	case ECombatEventType::PlayerInactive:
		ActivateInactivitySignal();
		break;

	case ECombatEventType::PlayerUnassignedInput:
		Metrics.UnassignedInputs++;
		break;

	default: break;
	}
}

void UScoreProcessor::UpdateProficiency(const FGameplayTag& AbilityTag, bool bSuccess)
{
	if (!AbilityTag.IsValid()) return;

	float& Proficiency = Metrics.InputProficiency.FindOrAdd(AbilityTag, 0.5f);

	// EWMA: P(t+1) = alpha * outcome + (1 - alpha) * P(t)
	// outcome is 1.0 for success, 0.0 for failure
	float Outcome = bSuccess ? 1.f : 0.f;
	Proficiency = ProficiencyAlpha * Outcome + (1.f - ProficiencyAlpha) * Proficiency;
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

void UScoreProcessor::ProcessOpportunity(ECombatEventType OpportunityType,
                                         bool bWasActedOn)
{
	Metrics.TotalOpportunities++;

	FInputOpportunityRecord& Record = Metrics.OpportunityRecords.FindOrAdd(OpportunityType);
	Record.Total++;

	if (bWasActedOn)
	{
		Metrics.ActedOpportunities++;
		Record.ActedOn++;
	}
	else
	{
		// Missed opportunity is a discrete failure event
		ProcessEvent({ECombatEventType::MissedOpportunity});
	}
}
