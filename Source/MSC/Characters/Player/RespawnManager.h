#pragma once

#include "RespawnManager.generated.h"

enum class ESessionState : uint8;
class AMSC_EnemySpawner;
class AMSC_CharacterPlayer;

UCLASS()
class ARespawnManager : public AActor
{
	GENERATED_BODY()
public:
	
	void TriggerRespawn();
	
protected:
	virtual void BeginPlay() override;
private:
	void ResetAllEnemies();
	void ResetPlayer();
	void OnRespawnTimerComplete();
	
	UFUNCTION()
	void OnSessionStateChanged(const ESessionState& NewState);
	
	UPROPERTY(EditAnywhere, Category="Respawn")
	AMSC_EnemySpawner* EnemySpawner;

	UPROPERTY(EditAnywhere, Category = "Respawn")
	float RespawnDelay = 3.f;

	UPROPERTY(EditAnywhere, Category = "Respawn")
	TSubclassOf<AMSC_CharacterPlayer> PlayerClass;

	FTimerHandle RespawnTimer;
};
