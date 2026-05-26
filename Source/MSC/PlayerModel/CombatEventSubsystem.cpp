#include "CombatEventSubsystem.h"

#include "CombatEventTypes.h"
#include "ScoreProcessor.h"


void UCombatEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ScoreProcessor = NewObject<UScoreProcessor>(this);
}

void UCombatEventSubsystem::Deinitialize()
{
	Super::Deinitialize();
}


float UCombatEventSubsystem::GetFrustrationScore()
{
	return ScoreProcessor->GetMetrics().FrustrationScore;
}

float UCombatEventSubsystem::GetMetricValue(FGameplayTag MetricOrAbilityTag) const
{
	if (!ScoreProcessor)
	{
		return 0.f;
	}

	const FCombatMetrics& Metrics = ScoreProcessor->GetMetrics();
	const FGameplayTag FrustrationTag = FGameplayTag::RequestGameplayTag(FName("Metric.FrustrationScore"));

	if (MetricOrAbilityTag == FrustrationTag)
	{
		return Metrics.FrustrationScore;
	}

	if (const float* Value = Metrics.InputProficiency.Find(MetricOrAbilityTag))
	{
		return *Value;
	}

	return 0.f;
}

void UCombatEventSubsystem::ReportEvent(FCombatEvent Event)
{
	Event.Timestamp = GetWorld()->GetTimeSeconds();
    
	// OnCombatEvent.Broadcast(Event);

	ScoreProcessor->ProcessTelemetryEvent(Event);
}

void UCombatEventSubsystem::ReportOpportunity(ECombatSituation OpportunityType, float WindowDuration)
{
	OpenOpportunities.Add(OpportunityType, GetWorld()->GetTimeSeconds());

	// Auto-close the opportunity after the window expires
	FTimerHandle OpportunityTimer;
	GetWorld()->GetTimerManager().SetTimer(OpportunityTimer,[this, OpportunityType]()
		{
			CloseOpportunity(OpportunityType, false);
		},WindowDuration, false);
}

void UCombatEventSubsystem::CloseOpportunity(ECombatSituation OpportunityType, bool bWasActedOn)
{
	if (OpenOpportunities.Contains(OpportunityType))
	{
		OpenOpportunities.Remove(OpportunityType);
		ScoreProcessor->ProcessOpportunity(OpportunityType, bWasActedOn);
	}
}

void UCombatEventSubsystem::StopInactivityTracking()
{
	if (ScoreProcessor)
	{
		ScoreProcessor->DeactivateInactivitySignal();
	}
}
