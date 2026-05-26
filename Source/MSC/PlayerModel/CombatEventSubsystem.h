#pragma once
#include "CombatMetrics.h"
#include "CombatEventSubsystem.generated.h"

class UScoreProcessor;
enum class ECombatSituation : uint8;
struct FCombatEvent;

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
	void CloseOpportunity(ECombatSituation OpportunityType, bool bWasActedOn);
	

	// Inactivity tracking
	UFUNCTION(BlueprintCallable, Category="Combat Metrics")
	void StopInactivityTracking();

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatEvent, const FCombatEvent&);
	FOnCombatEvent OnCombatEvent;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
private:

	UPROPERTY()
	UScoreProcessor* ScoreProcessor;

	// Tracks open opportunities and when they were created
	TMap<ECombatSituation, float> OpenOpportunities;

};