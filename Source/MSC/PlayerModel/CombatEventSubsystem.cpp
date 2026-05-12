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

void UCombatEventSubsystem::ResetInactivityTimer()
{
	GetWorld()->GetTimerManager().SetTimer(InactivityTimer, this, &UCombatEventSubsystem::OnInactivityTimerFired, InactivityThreshold, false);
}

void UCombatEventSubsystem::OnInactivityTimerFired()
{
	FCombatEvent Event;
	Event.EventType = ECombatEventType::PlayerInactive;
	Event.Magnitude = InactivityThreshold;
	ReportEvent(Event);
}