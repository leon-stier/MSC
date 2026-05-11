#pragma once


enum class ECombatEventType : uint8;
struct FGameplayTag;

USTRUCT()
struct FDurationSignalConfig
{
	GENERATED_BODY()

	// R_max: maximum penalty rate
	UPROPERTY(EditDefaultsOnly)
	float MaxPenaltyRate = 1.f;

	// a_k: sigmoid steepness
	UPROPERTY(EditDefaultsOnly)
	float Steepness = 1.f;

	// b_k: sigmoid inflection point (seconds)
	UPROPERTY(EditDefaultsOnly)
	float InflectionPoint = 5.f;
};

USTRUCT()
struct FActiveDurationSignal
{
	GENERATED_BODY()

	FDurationSignalConfig Config;

	// d_k(t): how long this signal has been active
	float Duration = 0.f;

	bool bActive = false;

	// r_k(t): sigmoid penalty rate
	float GetPenaltyRate() const
	{
		return Config.MaxPenaltyRate / (1.f + FMath::Exp(-Config.Steepness * (Duration - Config.InflectionPoint)));
	}
};

USTRUCT()
struct FInputOpportunityRecord
{
	GENERATED_BODY()

	int32 TotalOpportunities = 0;

	int32 TimesUsed = 0;

	// Current usage rate [0,1]
	float GetUsageRate() const
	{
		return TotalOpportunities > 0 
			? static_cast<float>(TimesUsed) / TotalOpportunities 
			: 0.f;
	}
};

USTRUCT()
struct FInputBaseline
{
	GENERATED_BODY()

	float BaselineUsageRate = 0.f;

	bool bIsEstablished = false;
};

USTRUCT()
struct FInputDeviationRecord
{
	GENERATED_BODY()

	FInputBaseline Baseline;
	FInputOpportunityRecord Current;

	// How much usage has changed relative to baseline
	// Negative means less usage than baseline
	float GetUsageDeviation() const
	{
		if (!Baseline.bIsEstablished) return 0.f;
		return Current.GetUsageRate() - Baseline.BaselineUsageRate;
	}

	// Normalized Deviation [-1, 1] relative to baseline
	// -1 means never used anymore, 0 means same as baseline, +1 means always used
	float GetNormalizedDeviation() const
	{
		if (!Baseline.bIsEstablished || Baseline.BaselineUsageRate <= 0.f) 
			return 0.f;
		return GetUsageDeviation() / Baseline.BaselineUsageRate;
	}

	bool IsForgotten(float DeviationThreshold = -0.5f) const
	{
		return Baseline.bIsEstablished && GetNormalizedDeviation() < DeviationThreshold;
	}
};

USTRUCT()
struct FCombatMetrics
{
	GENERATED_BODY()

    // Global frustration score F(t)
    float FrustrationScore = 0.f;

    // Per-input EWMA proficiency [0,1]
    TMap<FGameplayTag, float> InputProficiency;

    // Raw counters
    int32 Deaths = 0;
    int32 DamageTakenCount = 0;
    int32 SuccessfulCombos = 0;
    int32 SuccessfulHits = 0;
    int32 SuccessfulParries = 0;
    int32 SuccessfulBlocks = 0;
    int32 SuccessfulDodges = 0;
    int32 AbilitiesMissed = 0;
    int32 UnassignedInputs = 0;

    int32 TotalOpportunities = 0;
    int32 ActedOpportunities = 0;

    TMap<FGameplayTag, int32> AbilityActivations;
    TMap<FGameplayTag, int32> AbilityMisses;
    TMap<ECombatEventType, FInputOpportunityRecord> OpportunityRecords;
	
	TMap<FGameplayTag, FInputDeviationRecord> InputDeviationRecords;
	bool bRecordingBaseline = false;
	bool bBaselineEstablished = false;
};