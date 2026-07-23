#include "CombatEventSubsystem.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "CombatEventTypes.h"
#include "HintData.h"
#include "Kismet/GameplayStatics.h"
#include "ScoreProcessor.h"


void UCombatEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ScoreProcessor = NewObject<UScoreProcessor>(this);
	ScoreProcessor->SetTimeProvider(NewObject<USessionTimeProvider>());
	SessionManager = NewObject<USessionManager>(this);
	SessionManager->Initialize(ScoreProcessor);
	// InitializeSession(TEXT("Player1")); // Placeholder player name, should be set dynamically

	ScoreProcessor->OnInputForgotten.AddUObject(this, &UCombatEventSubsystem::TriggerHint);
	
	UAbilityHintData* LoadedHintData = Cast<UAbilityHintData>(HintDataPath.TryLoad());
	if (IsValid(LoadedHintData))
	{
		HintData = LoadedHintData;
	}
	
	SessionManager->OnSessionStateChanged.AddDynamic(this, &UCombatEventSubsystem::OnSessionStateChangedHandler);
}

void UCombatEventSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UCombatEventSubsystem::TriggerHint(const FGameplayTag& ForgottenInputTag)
{
	if (bHintActive || bHintBackingOff) return;
	FAbilityHintEntry HintDataEntry = HintData->GetHintEntry(ForgottenInputTag);
	OnHintTriggered.Broadcast(HintDataEntry);
	GetWorld()->GetTimerManager().SetTimer(HintTimer, this, &UCombatEventSubsystem::DisableHint, 5.f, false);
	bHintActive = true;
}

void UCombatEventSubsystem::DisableHint()
{
	bHintActive = false;
	bHintBackingOff = true;
	GetWorld()->GetTimerManager().ClearTimer(HintTimer);
	GetWorld()->GetTimerManager().SetTimer(HintBackoffTimer, this, &UCombatEventSubsystem::DisableBackoff, 5.f, false);
	OnHintDismissed.Broadcast(); 
}

void UCombatEventSubsystem::DisableBackoff()
{
	ScoreProcessor->RecheckProficiencyThresholds();
	bHintBackingOff = false;
	GetWorld()->GetTimerManager().ClearTimer(HintBackoffTimer);
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

float UCombatEventSubsystem::GetBaselineValue(FGameplayTag MetricOrAbilityTag) const
{
	if (!ScoreProcessor)
	{
		return 0.f;
	}

	const FCombatMetrics& Metrics = ScoreProcessor->GetBaselineMetrics();
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

void UCombatEventSubsystem::StartSession(const FString& PlayerName)
{
	SessionManager->StartSession(PlayerName);
}

void UCombatEventSubsystem::StopSession()
{
	SessionManager->EndAndResetSession();
}

void UCombatEventSubsystem::GoToBaseline()
{
	SessionManager->GoToBaseline();
}

void UCombatEventSubsystem::SetController(FString Type)
{
	ControllerType = Type;
}

FString UCombatEventSubsystem::GetControllerType()
{
	return ControllerType;
}

void UCombatEventSubsystem::StartAutoSave()
{
	// Start autosave timer
	GetWorld()->GetTimerManager().SetTimer(AutoSaveTimer,
		this, &UCombatEventSubsystem::OnAutosaveTimer,
		AutoSaveInterval, true); // true = looping
}

void UCombatEventSubsystem::StopAutoSave()
{
	GetWorld()->GetTimerManager().ClearTimer(AutoSaveTimer);
}

void UCombatEventSubsystem::OnAutosaveTimer() const
{
	SessionManager->SaveMetrics(ScoreProcessor->GetMetrics());
}

void UCombatEventSubsystem::OnSessionStateChangedHandler(const ESessionState& NewState)
{
	OnSessionStateChanged.Broadcast(NewState);
}
