#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "HintData.generated.h"

USTRUCT(BlueprintType)
struct FAbilityHintEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString HintText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString IconName;
};

UCLASS()
class UAbilityHintData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category = "Hints")
	TArray<FAbilityHintEntry> Entries;

	const FAbilityHintEntry* FindEntry(const FGameplayTag& Tag) const
	{
		return Entries.FindByPredicate([&Tag](const FAbilityHintEntry& Entry)
		{
			return Entry.AbilityTag == Tag;
		});
	}

	FString GetHintString(const FGameplayTag& Tag) const
	{
		const FAbilityHintEntry* Entry = FindEntry(Tag);
		return Entry ? Entry->HintText : Tag.ToString();
	}
	
	FAbilityHintEntry GetHintEntry(const FGameplayTag& Tag) const
	{
		const FAbilityHintEntry* Entry = FindEntry(Tag);
		return Entry ? *Entry : FAbilityHintEntry();
	}
};