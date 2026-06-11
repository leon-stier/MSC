#pragma once

#include "CoreMinimal.h"
#include "CombatMetrics.h"
#include "Dom/JsonObject.h"

class FMetricsSerializer
{
public:
	static TSharedPtr<FJsonObject> MetricsToJson(const FCombatMetrics& Metrics);
	static FString MetricsToJsonString(const FCombatMetrics& Metrics);
	static bool WriteMetricsToFile(const FString& FilePath, const FCombatMetrics& Metrics);
};