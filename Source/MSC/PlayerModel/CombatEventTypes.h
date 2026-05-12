#pragma once
#include "GameplayTagContainer.h"
#include "CombatEventTypes.generated.h"

UENUM(BlueprintType)
enum class ECombatEventType : uint8
{
	// Player actions
	PlayerDied,
	PlayerTookDamage,
	PlayerAbilityMissed,        // missed timing window or enemy
	PlayerUnassignedInput,
	MissedOpportunity,
	
	PlayerSuccessfulCombo,
	PlayerSuccessfulHit,
	PlayerSuccessfulParry,
	PlayerSuccessfulBlock,
	PlayerSuccessfulDodge,
	PlayerAbilitySuccessful,
	
	PlayerInactive,             // time spent doing nothing
};

USTRUCT(BlueprintType)
struct FCombatEvent
{
	GENERATED_BODY()
	
	UPROPERTY()
	ECombatEventType EventType;

	UPROPERTY()
	float Timestamp = 0.f;

	// Which ability was involved (if any)
	UPROPERTY()
	FGameplayTag AbilityTag;

	// Extra context (damage amount, combo count etc.)
	UPROPERTY()
	float Magnitude = 0.f;

	UPROPERTY()
	TWeakObjectPtr<AActor> Instigator;

	UPROPERTY()
	TWeakObjectPtr<AActor> Target;
};
