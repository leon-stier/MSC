#pragma once

#include "CoreMinimal.h"
#include "CombatMetrics.h"
#include "SessionManager.generated.h"

class UScoreProcessor;

UENUM(BlueprintType)
enum class ESessionState : uint8
{
	Idle,               // No session active
	RecordingBaseline,  // Tester playing, baseline being recorded
	RecordingHints,     // Second session, hints engaged
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSessionStateChanged, const ESessionState&, NewState);

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
	void Initialize(UScoreProcessor* InScoreProcessor);
	
	void StartSession(const FString& PlayerName);

	void EndAndResetSession();
	
	void Reset();
	
	void SaveMetrics(const FCombatMetrics& Metrics) const;

	bool LoadBaseline(const FString& PlayerName, FCombatMetrics& OutMetrics);

	bool IsBaselinePhase() const { return bBaselinePhase; }
	bool IsInitialized() const { return bInitialized; }
	
	UPROPERTY(BlueprintAssignable)
	FOnSessionStateChanged OnSessionStateChanged;
private:
	static FString BuildSessionPath(const FString& PlayerName);
	static bool WriteJsonToFile(const FString& FilePath, const FString& JsonString);

	FString SessionPath;
	bool bBaselinePhase = true;
	bool bInitialized = false;
	
	ESessionState SessionState = ESessionState::Idle;
	FString CurrentTesterName;
	
	UPROPERTY()
	UScoreProcessor* ScoreProcessor;
};