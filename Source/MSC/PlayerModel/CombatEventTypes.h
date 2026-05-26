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
	
	PlayerSuccessfulCombo,
	PlayerSuccessfulHit,
	PlayerSuccessfulParry,
	PlayerSuccessfulBlock,
	PlayerSuccessfulDodge,
	PlayerAbilitySuccessful,
	
	PlayerInactive,             // time spent doing nothing
};

UENUM(BlueprintType)
enum class ECombatSituation : uint8
{
	NormalAttack,        // default block opportunity
	UndodgeableAttack,   // must block or parry
	UnblockableAttack,   // must dodge
	ParryWindow,         // tight parry timing
	DodgeWindow,         // tight dodge timing
	HitWindow,           // opening to hit
	ComboWindow          // chain input window
};

USTRUCT(BlueprintType)
struct FCombatEvent
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	ECombatEventType EventType;

	UPROPERTY()
	float Timestamp = 0.f;

	// Which ability was involved (if any)
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag AbilityTag;

	// Extra context (damage amount, combo count etc.)
	UPROPERTY(BlueprintReadWrite)
	float Magnitude = 0.f;

	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<AActor> Instigator;

	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<AActor> Target;
};
