#include "CombatEventSubsystem.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "CombatEventTypes.h"
#include "Kismet/GameplayStatics.h"
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

	// If the player is already blocking, immediately count as a block reaction.
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (APawn* PlayerPawn = PlayerController->GetPawn())
		{
			if (IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(PlayerPawn))
			{
				if (UAbilitySystemComponent* AbilitySystem = AbilityInterface->GetAbilitySystemComponent())
				{
					const FGameplayTag BlockingTag = FGameplayTag::RequestGameplayTag(FName("Combat.Blocking"));
					if (AbilitySystem->HasMatchingGameplayTag(BlockingTag))
					{
						CloseOpportunity(OpportunityType, FGameplayTag::RequestGameplayTag(FName("Ability.Id.Block")));
						return;
					}
				}
			}
		}
	}

	if (WindowDuration <= 0.f) return;
	
	// Auto-close the opportunity after the window expires
	FTimerHandle OpportunityTimer;
	GetWorld()->GetTimerManager().SetTimer(OpportunityTimer,[this, OpportunityType]()
		{
			CloseOpportunity(OpportunityType, FGameplayTag());
		},WindowDuration, false);
}

void UCombatEventSubsystem::CloseOpportunity(ECombatSituation OpportunityType, FGameplayTag ActedAbilityTag)
{
	if (OpenOpportunities.Contains(OpportunityType))
	{
		OpenOpportunities.Remove(OpportunityType);
		ScoreProcessor->ProcessOpportunity(OpportunityType, ActedAbilityTag);
	}
}

void UCombatEventSubsystem::StopInactivityTracking()
{
	if (ScoreProcessor)
	{
		ScoreProcessor->DeactivateInactivitySignal();
	}
}
