#include "SessionManager.h"
#include "MetricsSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

void USessionManager::InitializeSession(const FString& PlayerName)
{
    SessionPath = BuildSessionPath(PlayerName);

    // Create the session directory
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    if (!PlatformFile.DirectoryExists(*SessionPath))
    {
        PlatformFile.CreateDirectoryTree(*SessionPath);
    }

    bBaselinePhase = true;
    bInitialized = true;

    UE_LOG(LogTemp, Log, TEXT("Session initialized at: %s"), *SessionPath);
}

void USessionManager::StartHintsPhase()
{
    if (!bInitialized) return;

    bBaselinePhase = false;
    OnHintsPhaseStarted.Broadcast();

    UE_LOG(LogTemp, Log, TEXT("Hints phase started"));
}

void USessionManager::SaveMetrics(const FCombatMetrics& Metrics) const
{
    if (!bInitialized) return;

	if (bBaselinePhase)
	{
		FMetricsSerializer::WriteMetricsToFile(SessionPath / TEXT("BaselineMetrics.json"), Metrics);
	} 
	else
	{
		FMetricsSerializer::WriteMetricsToFile(SessionPath / TEXT("Metrics.json"), Metrics);
	}
	
    UE_LOG(LogTemp, Log, TEXT("Saved metrics to %s"), *SessionPath);
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

bool USessionManager::WriteJsonToFile(
    const FString& FilePath, 
    const FString& JsonString)
{
    if (!FFileHelper::SaveStringToFile(JsonString, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to write metrics to %s"), *FilePath);
        return false;
    }
    return true;
}