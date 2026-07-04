#include "MetricsSerializer.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

// MetricsSerializer.cpp
TSharedPtr<FJsonObject> FMetricsSerializer::MetricsToJson(const FCombatMetrics& Metrics)
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

	// Helper lambda to serialize a timestamped float array
	auto SerializeFloatHistory = [](const TArray<TPair<float, float>>& History)
		-> TArray<TSharedPtr<FJsonValue>>
	{
		TArray<TSharedPtr<FJsonValue>> Array;
		for (const auto& [Time, Value] : History)
		{
			TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
			Entry->SetNumberField("t", Time);
			Entry->SetNumberField("v", Value);
			Array.Add(MakeShared<FJsonValueObject>(Entry));
		}
		return Array;
	};

	// Helper lambda to serialize a timestamped int array
	auto SerializeIntHistory = [](const TArray<TPair<float, int32>>& History)
		-> TArray<TSharedPtr<FJsonValue>>
	{
		TArray<TSharedPtr<FJsonValue>> Array;
		for (const auto& [Time, Value] : History)
		{
			TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
			Entry->SetNumberField("t", Time);
			Entry->SetNumberField("v", Value);
			Array.Add(MakeShared<FJsonValueObject>(Entry));
		}
		return Array;
	};

	// Scalar histories
	Root->SetArrayField("FrustrationScore", SerializeFloatHistory(Metrics.FrustrationScore));
	Root->SetArrayField("Deaths", SerializeIntHistory(Metrics.Deaths));
	Root->SetArrayField("DamageTakenCount", SerializeIntHistory(Metrics.DamageTakenCount));
	Root->SetArrayField("SuccessfulCombos", SerializeIntHistory(Metrics.SuccessfulCombos));
	Root->SetArrayField("SuccessfulHits", SerializeIntHistory(Metrics.SuccessfulHits));
	Root->SetArrayField("SuccessfulParries", SerializeIntHistory(Metrics.SuccessfulParries));
	Root->SetArrayField("SuccessfulBlocks", SerializeIntHistory(Metrics.SuccessfulBlocks));
	Root->SetArrayField("SuccessfulDodges", SerializeIntHistory(Metrics.SuccessfulDodges));
	Root->SetArrayField("AbilitiesMissed", SerializeIntHistory(Metrics.AbilitiesMissed));
	Root->SetArrayField("UnassignedInputs", SerializeIntHistory(Metrics.UnassignedInputs));
	Root->SetArrayField("TotalOpportunities", SerializeIntHistory(Metrics.TotalOpportunities));
	Root->SetArrayField("ActedOpportunities", SerializeIntHistory(Metrics.ActedOpportunities));

	// Per-ability maps
	auto SerializeMapOfIntHistories = [&SerializeIntHistory](
		const TMap<FGameplayTag, TArray<TPair<float, int32>>>& Map)
		-> TSharedPtr<FJsonObject>
	{
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		for (const auto& [Key, History] : Map)
		{
			Obj->SetArrayField(Key.ToString(), SerializeIntHistory(History));
		}
		return Obj;
	};

	auto SerializeMapOfFloatHistories = [&SerializeFloatHistory](
		const TMap<FGameplayTag, TArray<TPair<float, float>>>& Map)
		-> TSharedPtr<FJsonObject>
	{
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		for (const auto& [Key, History] : Map)
		{
			Obj->SetArrayField(Key.ToString(), SerializeFloatHistory(History));
		}
		return Obj;
	};

	Root->SetObjectField("AbilityActivations", SerializeMapOfIntHistories(Metrics.AbilityActivations));
	Root->SetObjectField("AbilityMisses", SerializeMapOfIntHistories(Metrics.AbilityMisses));
	Root->SetObjectField("InputProficiency", SerializeMapOfFloatHistories(Metrics.InputProficiency));

	return Root;
}


FString FMetricsSerializer::MetricsToJsonString(const FCombatMetrics& Metrics)
{
	TSharedPtr<FJsonObject> JsonObj = MetricsToJson(Metrics);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObj.ToSharedRef(), Writer);
	return OutputString;
}

bool FMetricsSerializer::WriteMetricsToFile(const FString& FilePath, const FCombatMetrics& Metrics)
{
	FString JsonString = MetricsToJsonString(Metrics);
	if (!FFileHelper::SaveStringToFile(JsonString, *FilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to write metrics to %s"), *FilePath);
		return false;
	}
	return true;
}

bool FMetricsSerializer::ReadLastMetricsSnapshot(const FString& FilePath, FCombatMetrics& OutMetrics)
{
    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Could not load metrics from %s"), *FilePath);
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse metrics at %s"), *FilePath);
        return false;
    }

    // Read per-ability maps
    auto ReadLastMapOfFloats = [&Root](
        const FString& FieldName,
        TMap<FGameplayTag, TArray<TPair<float, float>>>& OutMap)
    {
        const TSharedPtr<FJsonObject>* MapObj;
        if (!Root->TryGetObjectField(FieldName, MapObj)) return;

        for (const auto& [Key, ArrayValue] : (*MapObj)->Values)
        {
            FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Key));
            if (!Tag.IsValid()) continue;

            const TArray<TSharedPtr<FJsonValue>>* Array;
            if (!ArrayValue->TryGetArray(Array) || Array->IsEmpty()) continue;

            const TSharedPtr<FJsonObject>* LastEntry;
            if (Array->Last()->TryGetObject(LastEntry))
            {
                float LastValue = static_cast<float>((*LastEntry)->GetNumberField("v"));
                OutMap.FindOrAdd(Tag).Add(TPair<float, float>(0.f, LastValue));
            }
        }
    };

    ReadLastMapOfFloats("InputProficiency", OutMetrics.InputProficiency);

    return true;
}
