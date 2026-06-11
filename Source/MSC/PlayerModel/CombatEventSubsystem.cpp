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
	SessionManager = NewObject<USessionManager>(this);
	InitializeSession(TEXT("Player1")); // Placeholder player name, should be set dynamically

	ScoreProcessor->OnInputForgotten.AddUObject(this, &UCombatEventSubsystem::TriggerHint);
}

void UCombatEventSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UCombatEventSubsystem::TriggerHint(const FGameplayTag& ForgottenInputTag)
{
	OnHintTriggered.Broadcast("Press " + ForgottenInputTag.GetTagName().ToString() + " to react!");
	GetWorld()->GetTimerManager().SetTimer(HintTimer, this, &UCombatEventSubsystem::DisableHint, 3.f, false);
	bHintActive = true;
}

void UCombatEventSubsystem::DisableHint()
{
	bHintActive = false;
	GetWorld()->GetTimerManager().ClearTimer(HintTimer);
	OnHintDismissed.Broadcast();
}


void UCombatEventSubsystem::FreezeScores()
{
	bIsFrozen = true;
	ScoreProcessor->bIsFrozen = true;
}

void UCombatEventSubsystem::UnfreezeScores()
{
	bIsFrozen = false;
	ScoreProcessor->bIsFrozen = false;
}

float UCombatEventSubsystem::GetFrustrationScore()
{
	return ScoreProcessor->GetMetrics().FrustrationScore.Last().Value;
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
		return Metrics.FrustrationScore.Last().Value;
	}

	if (const auto Value = Metrics.InputProficiency.Find(MetricOrAbilityTag))
	{
		return Value->Last().Value;
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
	GetWorld()->GetTimerManager().SetTimer(OpportunityTimer, [this, OpportunityType]()
	{
		CloseOpportunity(OpportunityType, FGameplayTag());
	}, WindowDuration, false);
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

void UCombatEventSubsystem::InitializeSession(const FString& PlayerName)
{
	SessionManager->InitializeSession(PlayerName);
	ScoreProcessor->SetTimeProvider(NewObject<USessionTimeProvider>());

	// Start autosave timer
	GetWorld()->GetTimerManager().SetTimer(AutoSaveTimer,
		this, &UCombatEventSubsystem::OnAutosaveTimer,
		AutoSaveInterval, true); // true = looping
}

void UCombatEventSubsystem::StartHintsPhase()
{
	// Save baseline metrics before switching
	BaselineMetrics = ScoreProcessor->GetMetrics();
	SessionManager->SaveMetrics(BaselineMetrics);

	SessionManager->StartHintsPhase();

	// Reset score processor for the hints phase
	ScoreProcessor->SwitchToLiveMetrics();

	SessionManager->OnHintsPhaseStarted.Broadcast();
}

void UCombatEventSubsystem::OnAutosaveTimer() const
{
	SessionManager->SaveMetrics(ScoreProcessor->GetMetrics());
}
