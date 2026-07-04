#pragma once
#include "CombatMetrics.h"
#include "HintData.h"
#include "SessionManager.h"
#include "CombatEventSubsystem.generated.h"

class UAbilityHintData;
class USessionManager;
class UScoreProcessor;
enum class ECombatSituation : uint8;
struct FCombatEvent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHintTriggered, FAbilityHintEntry, HintData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHintDismissed);



UCLASS(Config=Game)
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
	float GetMetricValue(FGameplayTag MetricOrAbilityTag) const;
	
	UFUNCTION(BlueprintCallable, Category="Combat Metrics")
	float GetBaselineValue(FGameplayTag MetricOrAbilityTag) const;
	
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
	void StartSession(const FString& PlayerName);
	
	UFUNCTION(BlueprintCallable)
	void StopSession();
	
	UFUNCTION(BlueprintCallable)
	bool IsBaselinePhase() const { return SessionManager->IsBaselinePhase(); }
	
	void StartAutoSave();
	
	void StopAutoSave();
	
	// How often to autosave in seconds
	UPROPERTY(EditDefaultsOnly, Category = "Session")
	float AutoSaveInterval = 10.f;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	UPROPERTY(BlueprintAssignable)
	FOnSessionStateChanged OnSessionStateChanged;
private:
	UFUNCTION()
	void TriggerHint(const FGameplayTag& ForgottenInputTag);
	
	void DisableHint();

	bool bHintActive = false;
	
	FTimerHandle HintTimer;
	
	UPROPERTY(Config, EditDefaultsOnly, Category = "Hints")
	FSoftObjectPath HintDataPath;
	
	UPROPERTY(EditDefaultsOnly, Category = "Hints")
	TObjectPtr<UAbilityHintData> HintData;
	
	UPROPERTY()
	USessionManager* SessionManager;

	FTimerHandle AutoSaveTimer;

	void OnAutosaveTimer() const;
	
	UFUNCTION()
	void OnSessionStateChangedHandler(const ESessionState& NewState);
	
	bool bIsFrozen = false;

	UPROPERTY(EditDefaultsOnly, Category = "Hints")
	float ForgottenInputDriftThreshold = -0.3f;
	
	UPROPERTY()
	UScoreProcessor* ScoreProcessor;

	// Tracks open opportunities and when they were created
	TMap<ECombatSituation, float> OpenOpportunities;

};