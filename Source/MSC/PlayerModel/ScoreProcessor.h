#pragma once
#include "CombatEventTypes.h"
#include "CombatMetrics.h"
#include "ScoreProcessor.generated.h"

struct FCombatMetrics;
enum class ECombatEventType : uint8;
enum class ECombatSituation : uint8;
struct FCombatEvent;

// ScoreProcessor.h
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

    // FTickableGameObject
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return true; }
    virtual TStatId GetStatId() const override 
    { 
        RETURN_QUICK_DECLARE_CYCLE_STAT(UScoreProcessor, STATGROUP_Tickables); 
    }

private:
    void UpdateFrustrationScore(float DeltaTime, float DiscreteEventWeight);
    void ApplyOpportunityOutcome(FGameplayTag AbilityTag, float Outcome);
    TArray<FGameplayTag> GetOpportunityActionTags(ECombatSituation OpportunityType) const;

    FCombatMetrics Metrics;

    // Max expected frustration value used to scale weights into [0,1].
    float FrustrationScoreMax = 10.f;

    // lambda: exponential decay constant
    float DecayConstant = 0.1f;

    // EWMA smoothing factor for opportunity outcomes [0,1]
    // higher = keeps more historical weight
    float OpportunityAlpha = 0.8f;

    // Discrete event weights (w_j)
    TMap<ECombatEventType, float> EventWeights =
    {
        { ECombatEventType::PlayerDied,              5.f  },
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