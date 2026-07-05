#include "SessionManager.h"

#include "CombatEventSubsystem.h"
#include "MetricsSerializer.h"
#include "ScoreProcessor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

void USessionManager::Initialize(UScoreProcessor* InScoreProcessor)
{
	ScoreProcessor = InScoreProcessor;
}

void USessionManager::StartSession(const FString& PlayerName)
{
	if (SessionState != ESessionState::Idle)
	{
		UE_LOG(LogTemp, Warning, 
			TEXT("StartSession called while session already active - call EndAndResetSession first"));
		return;
	}
	
	CurrentTesterName = PlayerName;
	
	// Check if baseline exists. If it does, go to Hints phase
	
	FCombatMetrics BaselineMetrics;
	if (LoadBaseline(PlayerName, BaselineMetrics))
	{
		ScoreProcessor->SetBaselineMetrics(BaselineMetrics);
		SessionState = ESessionState::Hints;
		OnSessionStateChanged.Broadcast(SessionState);
		UE_LOG(LogTemp, Log, TEXT("Loaded previous baseline for %s - Starting Hints Phase"), *PlayerName);
	} else
	{	
		SessionPath = BuildSessionPath(PlayerName);
		
		// Create the session directory
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (!PlatformFile.DirectoryExists(*SessionPath))
		{
			PlatformFile.CreateDirectoryTree(*SessionPath);
		}
		SessionState = ESessionState::Baseline;
		OnSessionStateChanged.Broadcast(SessionState);
		UE_LOG(LogTemp, Log, TEXT("No baseline exists for %s - Starting Baseline Phase"), *PlayerName);
	}
	ScoreProcessor->bIsFrozen = false;
	
	UCombatEventSubsystem::Get(GetWorld())->StartAutoSave();
	
    UE_LOG(LogTemp, Log, TEXT("Session initialized at: %s"), *SessionPath);
}

void USessionManager::EndAndResetSession()
{
	if (SessionState == ESessionState::Idle)
	{
		UE_LOG(LogTemp, Warning, TEXT("EndAndResetSession called while no session active"));
		return;
	}
	UCombatEventSubsystem::Get(GetWorld())->StopAutoSave();
	if (SessionState == ESessionState::Baseline)
	{
		SaveMetrics(ScoreProcessor->GetMetrics());
		UE_LOG(LogTemp, Log, TEXT("Baseline saved for %s"), *CurrentTesterName);
	} else
	{
		SaveMetrics(ScoreProcessor->GetMetrics());
		UE_LOG(LogTemp, Log, TEXT("Hint session saved for %s"), *CurrentTesterName);
	}
	Reset();
}

void USessionManager::Reset()
{
	ScoreProcessor->Reset();
	CurrentTesterName = FString();
	SessionState = ESessionState::Idle;
	OnSessionStateChanged.Broadcast(SessionState);
	
	UE_LOG(LogTemp, Log, TEXT("Ready for next tester"));
}

void USessionManager::SaveMetrics(const FCombatMetrics& Metrics) const
{
	if (SessionState == ESessionState::Baseline)
	{
		FMetricsSerializer::WriteMetricsToFile(SessionPath / TEXT("BaselineMetrics.json"), Metrics);
	} 
	else
	{
		FMetricsSerializer::WriteMetricsToFile(SessionPath / TEXT("Metrics.json"), Metrics);
	}
	
    UE_LOG(LogTemp, Log, TEXT("Saved metrics to %s"), *SessionPath);
}

bool USessionManager::LoadBaseline(const FString& PlayerName, FCombatMetrics& OutMetrics)
{
	// Find the most recent session folder for this player
	FString SafeName = PlayerName.Replace(TEXT(" "), TEXT("_"));
	FString SessionsDir = FPaths::ProjectSavedDir() / TEXT("Sessions");

	TArray<FString> SessionFolders;
	IFileManager::Get().FindFiles(SessionFolders, *(SessionsDir / SafeName + TEXT("_*")), false, true);

	if (SessionFolders.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("No previous sessions found for %s"), *PlayerName);
		return false;
	}

	SessionFolders.Sort();
	FString MostRecentSession = SessionsDir / SessionFolders.Last();
	FString BaselinePath = MostRecentSession / TEXT("BaselineMetrics.json");
	SessionPath = MostRecentSession;

	return FMetricsSerializer::ReadLastMetricsSnapshot(BaselinePath, OutMetrics);
}

FString USessionManager::BuildSessionPath(const FString& PlayerName)
{
    // Sanitize player name for use as folder name
    FString SafeName = PlayerName.Replace(TEXT(" "), TEXT("_"));
    SafeName = SafeName.Replace(TEXT("/"), TEXT(""));
    SafeName = SafeName.Replace(TEXT("\\"), TEXT(""));

    // Timestamp for uniqueness if same player runs multiple sessions
    FString Timestamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
    FString FolderName = FString::Printf(TEXT("%s_%s"), *SafeName, *Timestamp);

    return FPaths::ProjectSavedDir() / TEXT("Sessions") / FolderName;
}