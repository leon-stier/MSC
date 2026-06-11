#pragma once
#include "CombatMetrics.h"
#include "CombatEventSubsystem.generated.h"

class USessionManager;
class UScoreProcessor;
enum class ECombatSituation : uint8;
struct FCombatEvent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHintTriggered, const FString&, HintText);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHintDismissed);



UCLASS()
class UCombatEventSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	static UCombatEventSubsystem* Get(const UObject* WorldContextObject)
	{
		if (!WorldContextObject) return nullptr;
        
		UWorld* World = WorldContextObject->GetWorld();
		if (!World) return nullptr;

		UGameInstance* GameInstance = World->GetGameInstance();
		if (!GameInstance) return nullptr;

		return GameInstance->GetSubsystem<UCombatEventSubsystem>();
	}
	
	UFUNCTION(BlueprintCallable, Category="Combat Metrics")
	void FreezeScores();
	
	UFUNCTION(BlueprintCallable, Category="Combat Metrics")
	void UnfreezeScores();
	
	UFUNCTION(BlueprintCallable, Category="Combat Metrics")
	float GetFrustrationScore();
	
	UFUNCTION(BlueprintCallable, Category="Combat Metrics")
	float GetMetricValue(FGameplayTag MetricOrAbilityTag) const;
	
	// Single entry point for all events
	UFUNCTION(BlueprintCallable, Category="Combat Metrics")
	void ReportEvent(FCombatEvent Event);

	// Opportunity tracking
	UFUNCTION(BlueprintCallable, Category="Combat Metrics")
	void ReportOpportunity(ECombatSituation OpportunityType, float WindowDuration);
	UFUNCTION(BlueprintCallable, Category="Combat Metrics")
	void CloseOpportunity(ECombatSituation OpportunityType, FGameplayTag ActedAbilityTag);
	
	UPROPERTY(BlueprintAssignable)
	FOnHintTriggered OnHintTriggered;

	UPROPERTY(BlueprintAssignable)
	FOnHintDismissed OnHintDismissed;

	// Inactivity tracking
	UFUNCTION(BlueprintCallable, Category="Combat Metrics")
	void StopInactivityTracking();

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatEvent, const FCombatEvent&);
	FOnCombatEvent OnCombatEvent;
	
	UFUNCTION(BlueprintCallable)
	void InitializeSession(const FString& PlayerName);

	UFUNCTION(BlueprintCallable)
	void StartHintsPhase();

	// How often to autosave in seconds
	UPROPERTY(EditDefaultsOnly, Category = "Session")
	float AutoSaveInterval = 10.f;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
private:
	UFUNCTION()
	void TriggerHint(const FGameplayTag& ForgottenInputTag);
	
	void DisableHint();

	bool bHintActive = false;
	
	FTimerHandle HintTimer;
	
	UPROPERTY()
	USessionManager* SessionManager;

	// Separate metrics for baseline and hints phases
	FCombatMetrics BaselineMetrics;

	FTimerHandle AutoSaveTimer;

	void OnAutosaveTimer() const;
	
	bool bIsFrozen = false;

	UPROPERTY(EditDefaultsOnly, Category = "Hints")
	float ForgottenInputDriftThreshold = -0.3f;
	
	UPROPERTY()
	UScoreProcessor* ScoreProcessor;

	// Tracks open opportunities and when they were created
	TMap<ECombatSituation, float> OpenOpportunities;

};