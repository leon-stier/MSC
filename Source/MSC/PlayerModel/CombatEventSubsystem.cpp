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

void UCombatEventSubsystem::ReportEvent(FCombatEvent Event)
{
	Event.Timestamp = GetWorld()->GetTimeSeconds();
    
	// OnCombatEvent.Broadcast(Event);

	ScoreProcessor->ProcessTelemetryEvent(Event);
}

void UCombatEventSubsystem::ReportOpportunity(ECombatEventType OpportunityType, float WindowDuration)
{
	OpenOpportunities.Add(OpportunityType, GetWorld()->GetTimeSeconds());

	// Auto-close the opportunity after the window expires
	FTimerHandle OpportunityTimer;
	GetWorld()->GetTimerManager().SetTimer(OpportunityTimer,[this, OpportunityType]()
		{
			CloseOpportunity(OpportunityType, false);
		},WindowDuration, false);
}

void UCombatEventSubsystem::CloseOpportunity(ECombatEventType OpportunityType, bool bWasActedOn)
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
