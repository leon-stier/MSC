#pragma once
#include "CombatEventTypes.h"
#include "CombatMetrics.h"
#include "SessionManager.h"
#include "ScoreProcessor.generated.h"

struct FCombatMetrics;
enum class ECombatEventType : uint8;
enum class ECombatSituation : uint8;
struct FCombatEvent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnInputForgotten, const FGameplayTag&);


UCLASS()
class UScoreProcessor : public UObject, public FTickableGameObject
{
    GENERATED_BODY()

public:
    void ProcessTelemetryEvent(const FCombatEvent& Event);
    void ProcessOpportunity(ECombatSituation OpportunityType, FGameplayTag ActedAbilityTag);

    void ActivateInactivitySignal();
    void DeactivateInactivitySignal();

    const FCombatMetrics& GetMetrics() const;
    const FCombatMetrics& GetBaselineMetrics() const;
	
	void SetBaselineMetrics(const FCombatMetrics& InBaselineMetrics)
	{
		BaselineMetrics = InBaselineMetrics;
	}
	
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return true; }
    virtual TStatId GetStatId() const override 
    { 
        RETURN_QUICK_DECLARE_CYCLE_STAT(UScoreProcessor, STATGROUP_Tickables); 
    }

	bool bIsFrozen = true;
	
	FOnInputForgotten OnInputForgotten;
	
	UPROPERTY(EditDefaultsOnly, Category = "Hints")
	float FrustrationHintThreshold = 0.5f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	float ForgottenInputDriftThreshold = -0.2f;
	
	void SetTimeProvider(TObjectPtr<USessionTimeProvider> InTimeProvider)
	{
		TimeProvider = InTimeProvider;
	}
	
	void Reset();

private:
    void UpdateFrustrationScore(float DeltaTime, float DiscreteEventWeight);
    void ApplyOpportunityOutcome(FGameplayTag AbilityTag, float Outcome);
    static TArray<FGameplayTag> GetOpportunityActionTags(ECombatSituation OpportunityType);
	
	void CheckHintConditions();
	
	TArray<FGameplayTag> ForgottenInputs;

	UPROPERTY()
	TObjectPtr<USessionTimeProvider> TimeProvider;
	
	float Now() const 
	{ 
		return TimeProvider ? TimeProvider->GetSessionTime() : 0.f; 
	}
	
	float GetInitTime() const
	{
		return TimeProvider ? TimeProvider->GetInitTime() : 0.f;
	}
	
	template<typename T>
	void IncrementAppend(TArray<TPair<float, T>>& Array) const
	{
		T NewCount = Array.IsEmpty() ? 1 : Array.Last().Value + 1;
		Array.Add(TPair<float, T>(Now(), NewCount));
	}

	// Helpers to append timestamped values
	template<typename T>
	void Append(TArray<TPair<float, T>>& Array, T Value) const
	{
		Array.Add(TPair<float, T>(Now(), Value));
	}
	
	
    FCombatMetrics Metrics;
	FCombatMetrics BaselineMetrics;

    // Max expected frustration value used to scale weights into [0,1]
    float FrustrationScoreMax = 10.f;

    // Exponential decay constant
    float DecayConstant = 0.0f;

    // EWMA smoothing factor for opportunity outcomes [0,1]
    // higher = more historical weight
    float OpportunityAlpha = 0.8f;

    // Discrete event weights (w_j)
    TMap<ECombatEventType, float> EventWeights =
    {
        { ECombatEventType::PlayerDied,              3.5f  },
        { ECombatEventType::PlayerTookDamage,        2.f  },
        { ECombatEventType::PlayerAbilityMissed,     0.5f },
        { ECombatEventType::PlayerUnassignedInput,   0.3f },

        // Success events have negative weights
        { ECombatEventType::PlayerSuccessfulCombo,  -2.f  },
        { ECombatEventType::PlayerSuccessfulParry,  -3.f  },
        { ECombatEventType::PlayerSuccessfulDodge,  -2.5f },
        { ECombatEventType::PlayerSuccessfulBlock,  -1.5f },
        { ECombatEventType::PlayerSuccessfulHit,    -1.f  },
    };

    // Duration signal configs and runtime state
    FActiveDurationSignal InactivitySignal =
    {
        FDurationSignalConfig{ 1.f, 1.f, 5.f }, // MaxPenaltyRate, Steepness, InflectionPoint
		0.f, // Duration
		false // bActive
    };
};