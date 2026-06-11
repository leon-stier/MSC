#pragma once

#include "CoreMinimal.h"
#include "CombatMetrics.h"
#include "SessionManager.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnBaselinePhaseComplete);
DECLARE_MULTICAST_DELEGATE(FOnHintsPhaseStarted);

UCLASS()
class USessionTimeProvider : public UObject
{
	GENERATED_BODY()

public:
	void StartSession() { StartTime = FPlatformTime::Seconds(); }
	void ResetSession() { StartTime = FPlatformTime::Seconds(); }

	double GetSessionTime() const 
	{ 
		return FPlatformTime::Seconds() - StartTime; 
	}
	
	double GetInitTime() const { return StartTime; }

private:
	double StartTime = 0.0;
};

UCLASS()
class USessionManager : public UObject
{
	GENERATED_BODY()

public:
	void InitializeSession(const FString& PlayerName);

	void StartHintsPhase();

	void SaveMetrics(const FCombatMetrics& Metrics) const;

	bool IsBaselinePhase() const { return bBaselinePhase; }
	bool IsInitialized() const { return bInitialized; }

	FOnBaselinePhaseComplete OnBaselinePhaseComplete;
	FOnHintsPhaseStarted OnHintsPhaseStarted;

private:
	static FString BuildSessionPath(const FString& PlayerName);
	static bool WriteJsonToFile(const FString& FilePath, const FString& JsonString);

	FString SessionPath;
	bool bBaselinePhase = true;
	bool bInitialized = false;
};