#pragma once


#include "CombatEventSubsystem.generated.h"

class UScoreProcessor;
enum class ECombatEventType : uint8;
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
	
	// Single entry point for all events
	void ReportEvent(FCombatEvent Event);

	// Opportunity tracking
	void ReportOpportunity(ECombatEventType OpportunityType, float WindowDuration);
	void CloseOpportunity(ECombatEventType OpportunityType, bool bWasActedOn);

	// Inactivity tracking
	void StartInactivityTracking();
	void ResetInactivityTimer();

	DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatEvent, const FCombatEvent&);
	FOnCombatEvent OnCombatEvent;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
private:

	void OnInactivityTimerFired();

	UPROPERTY()
	UScoreProcessor* ScoreProcessor;

	// Tracks open opportunities and when they were created
	TMap<ECombatEventType, float> OpenOpportunities;

	FTimerHandle InactivityTimer;
	float InactivityThreshold = 3.f;
};