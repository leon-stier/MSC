#pragma once
#include "CombatEventTypes.h"
#include "CombatMetrics.h"

struct FCombatMetrics;
enum class ECombatEventType : uint8;
struct FCombatEvent;

// ScoreProcessor.h
UCLASS()
class UScoreProcessor : public UObject, public FTickableGameObject
{
    GENERATED_BODY()

public:
    void ProcessEvent(const FCombatEvent& Event);
    void ProcessOpportunity(ECombatEventType OpportunityType, bool bWasActedOn);

    void ActivateInactivitySignal();
    void DeactivateInactivitySignal();

    const FCombatMetrics& GetMetrics() const { return Metrics; }

    // FTickableGameObject
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return true; }
    virtual TStatId GetStatId() const override 
    { 
        RETURN_QUICK_DECLARE_CYCLE_STAT(UScoreProcessor, STATGROUP_Tickables); 
    }

private:
    void UpdateFrustrationScore(float DeltaTime, float DiscreteEventWeight);
    void UpdateProficiency(const FGameplayTag& AbilityTag, bool bSuccess);

    FCombatMetrics Metrics;

    // lambda: exponential decay constant
    float DecayConstant = 0.1f;

    // EWMA smoothing factor for proficiency [0,1]
    // closer to 1 = faster adaptation
    float ProficiencyAlpha = 0.2f;

    // Discrete event weights (w_j)
    TMap<ECombatEventType, float> EventWeights =
    {
        { ECombatEventType::PlayerDied,              5.f  },
        { ECombatEventType::PlayerTookDamage,        1.f  },
        { ECombatEventType::PlayerAbilityMissed,     0.5f },
        { ECombatEventType::PlayerUnassignedInput,   0.3f },
        { ECombatEventType::MissedOpportunity,       0.8f },

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