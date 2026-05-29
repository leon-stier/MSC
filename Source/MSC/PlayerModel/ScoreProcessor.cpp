#include "ScoreProcessor.h"

#include "CombatEventTypes.h"

void UScoreProcessor::ProcessTelemetryEvent(const FCombatEvent& Event)
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

	// Update raw metrics
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
		break;

	case ECombatEventType::PlayerSuccessfulBlock:
		Metrics.SuccessfulBlocks++;
		break;

	case ECombatEventType::PlayerSuccessfulDodge:
		Metrics.SuccessfulDodges++;
		break;

	case ECombatEventType::PlayerSuccessfulHit:
		Metrics.SuccessfulHits++;
		DeactivateInactivitySignal();
		break;

	case ECombatEventType::PlayerAbilitySuccessful:
		Metrics.AbilityActivations.FindOrAdd(Event.AbilityTag)++;
		break;

	case ECombatEventType::PlayerAbilityMissed:
		Metrics.AbilitiesMissed++;
		Metrics.AbilityMisses.FindOrAdd(Event.AbilityTag)++;
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

void UScoreProcessor::ProcessOpportunity(ECombatSituation OpportunityType, bool bWasActedOn)
{
	const FGameplayTag ActionTag = GetOpportunityActionTag(OpportunityType);
	if (!ActionTag.IsValid())
	{
		return;
	}

	float& Proficiency = Metrics.InputProficiency.FindOrAdd(ActionTag, 0.5f);
	const float Outcome = bWasActedOn ? 1.f : 0.f;

	// EWMA: P_i = alpha * P_i + (1 - alpha) * x_i
	Proficiency = OpportunityAlpha * Proficiency + (1.f - OpportunityAlpha) * Outcome;
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
{ return Metrics; }

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

	const float ScaledEventWeight = DiscreteEventWeight / FrustrationScoreMax;
	const float ScaledDurationPenalty = DurationPenalty / FrustrationScoreMax;

	Metrics.FrustrationScore = DecayedScore + ScaledEventWeight + ScaledDurationPenalty;
	Metrics.FrustrationScore = FMath::Clamp(Metrics.FrustrationScore, 0.f, 1.f);
}

FGameplayTag UScoreProcessor::GetOpportunityActionTag(ECombatSituation OpportunityType) const
{
	switch (OpportunityType)
	{
	case ECombatSituation::NormalAttack:
	case ECombatSituation::UndodgeableAttack:
		return FGameplayTag::RequestGameplayTag(FName("Ability.Id.Block"));

	case ECombatSituation::UnblockableAttack:
	case ECombatSituation::DodgeWindow:
		return FGameplayTag::RequestGameplayTag(FName("Ability.Id.Dodge"));

	case ECombatSituation::ParryWindow:
		return FGameplayTag::RequestGameplayTag(FName("Ability.Id.Parry"));

	case ECombatSituation::HitWindow:
		return FGameplayTag::RequestGameplayTag(FName("Ability.Id.Hit"));

	case ECombatSituation::ComboWindow:
		return FGameplayTag::RequestGameplayTag(FName("Ability.Id.Combo"));

	default:
		return FGameplayTag();
	}
}
