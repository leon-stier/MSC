#pragma once
#include "GameplayTagContainer.h"

#include "CombatMetrics.generated.h"

enum class ECombatEventType : uint8;
enum class ECombatSituation : uint8;
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
struct FCombatMetrics
{
	FCombatMetrics()
	{
		SeedFloat(FrustrationScore, 0.f);
		SeedInt(Deaths, 0);
		SeedInt(DamageTakenCount, 0);
		SeedInt(SuccessfulCombos, 0);
		SeedInt(SuccessfulHits, 0);
		SeedInt(SuccessfulParries, 0);
		SeedInt(SuccessfulBlocks, 0);
		SeedInt(SuccessfulDodges, 0);
		SeedInt(AbilitiesMissed, 0);
		SeedInt(UnassignedInputs, 0);
		SeedInt(TotalOpportunities, 0);
		SeedInt(ActedOpportunities, 0);
	}
	
	void Reset()
	{
		SeedFloat(FrustrationScore, 0.f);
		SeedInt(Deaths, 0);
		SeedInt(DamageTakenCount, 0);
		SeedInt(SuccessfulCombos, 0);
		SeedInt(SuccessfulHits, 0);
		SeedInt(SuccessfulParries, 0);
		SeedInt(SuccessfulBlocks, 0);
		SeedInt(SuccessfulDodges, 0);
		SeedInt(AbilitiesMissed, 0);
		SeedInt(UnassignedInputs, 0);
		SeedInt(TotalOpportunities, 0);
		SeedInt(ActedOpportunities, 0);
	}
	
	void Clear()
	{
		FrustrationScore.Empty();
		Deaths.Empty();
		DamageTakenCount.Empty();
		SuccessfulCombos.Empty();
		SuccessfulHits.Empty();
		SuccessfulParries.Empty();
		SuccessfulBlocks.Empty();
		SuccessfulDodges.Empty();
		AbilitiesMissed.Empty();
		UnassignedInputs.Empty();
		TotalOpportunities.Empty();
		ActedOpportunities.Empty();
		InputProficiency.Empty();
		AbilityActivations.Empty();
		AbilityMisses.Empty();
	}
	
	GENERATED_BODY()

    // Global frustration score [0,1], higher means more frustrated
    TArray<TPair<float, float>> FrustrationScore;

    // Per-input EWMA proficiency [0,1]
    TMap<FGameplayTag, TArray<TPair<float, float>>> InputProficiency;
	

    // Raw counters
    TArray<TPair<float, int32>> Deaths;
    TArray<TPair<float, int32>> DamageTakenCount;
    TArray<TPair<float, int32>> SuccessfulCombos;
    TArray<TPair<float, int32>> SuccessfulHits;
    TArray<TPair<float, int32>> SuccessfulParries;
    TArray<TPair<float, int32>> SuccessfulBlocks;
    TArray<TPair<float, int32>> SuccessfulDodges;
    TArray<TPair<float, int32>> AbilitiesMissed;
    TArray<TPair<float, int32>> UnassignedInputs;

    TArray<TPair<float, int32>> TotalOpportunities;
    TArray<TPair<float, int32>> ActedOpportunities;

    TMap<FGameplayTag, TArray<TPair<float, int32>>> AbilityActivations;
    TMap<FGameplayTag, TArray<TPair<float, int32>>> AbilityMisses;
	
	bool bRecordingBaseline = false;
	bool bBaselineEstablished = false;
	
private:
	static void SeedFloat(TArray<TPair<float, float>>& Array, float InitialValue)
	{
		Array.Add(TPair<float, float>(0.f, InitialValue));
	}

	static void SeedInt(TArray<TPair<float, int32>>& Array, int32 InitialValue)
	{
		Array.Add(TPair<float, int32>(0.f, InitialValue));
	}
};