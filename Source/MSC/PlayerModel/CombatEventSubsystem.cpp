#include "CombatEventSubsystem.h"

#include "CombatEventTypes.h"


void UCombatEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ScoreProcessor = NewObject<UScoreProcessor>(this);
}

void UCombatEventSubsystem::ReportEvent(FCombatEvent Event)
{
	Event.Timestamp = GetWorld()->GetTimeSeconds();
    
	// Maybe. Taking damage shouldn't reset inactivity
	ResetInactivityTimer();

	OnCombatEvent.Broadcast(Event);

	ScoreProcessor->ProcessEvent(Event);
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