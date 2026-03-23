#pragma once

#include "MSC_EnemySpawner.generated.h"

struct FGameplayTag;
class AMSC_CharacterEnemy;
class UCapsuleComponent;
class UArrowComponent;

UCLASS(abstract)
class MSC_API AMSC_EnemySpawner : public AActor
{
	
	GENERATED_BODY()
	
	AMSC_EnemySpawner();
	
	/** Initialization */
	virtual void BeginPlay() override;

	/** Cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* SpawnCapsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UArrowComponent* SpawnDirection;
	
protected:
	/** Spawn an enemy and subscribe to its death event */
	void SpawnEnemy();

	void OnDeadTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	
	/** Type of enemy to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner")
	TSubclassOf<AMSC_CharacterEnemy> EnemyClass;

	/** If true, the first enemy will be spawned as soon as the game starts */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner")
	bool bShouldSpawnEnemiesImmediately = true;

	/** Time to wait before spawning the first enemy on game start */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner", meta = (ClampMin = 0, ClampMax = 10))
	float InitialSpawnDelay = 5.0f;

	/** Number of enemies to spawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner", meta = (ClampMin = 0, ClampMax = 100))
	int32 SpawnCount = 1;
	

	/** Time to wait before spawning the next enemy after the current one dies */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Enemy Spawner", meta = (ClampMin = 0, ClampMax = 10))
	float RespawnDelay = 5.0f;

	/** Time to wait after this spawner is depleted before activating the actor list */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Activation", meta = (ClampMin = 0, ClampMax = 10))
	float ActivationDelay = 1.0f;
	
	/** Flag to ensure this is only activated once */
	bool bHasBeenActivated = false;

	/** Timer to spawn enemies after a delay */
	FTimerHandle SpawnTimer;
	
	int32 CurrentlyAlive = 0;
};
