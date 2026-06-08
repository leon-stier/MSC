#include "RespawnManager.h"

#include "EngineUtils.h"
#include "MSC_CharacterPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "MSC/Characters/AI/MSC_CharacterEnemy.h"
#include "MSC/Characters/AI/MSC_EnemySpawner.h"
#include "MSC/PlayerModel/CombatEventSubsystem.h"

void ARespawnManager::TriggerRespawn()
{
	// Freeze scores immediately
	if (UCombatEventSubsystem* CombatEvents = UCombatEventSubsystem::Get(this))
	{
		CombatEvents->FreezeScores();
	}

	// Start respawn sequence after delay
	GetWorldTimerManager().SetTimer(RespawnTimer, this,
		&ARespawnManager::OnRespawnTimerComplete,
		RespawnDelay, false);
}

void ARespawnManager::OnRespawnTimerComplete()
{
	ResetAllEnemies();
	ResetPlayer();

	if (UCombatEventSubsystem* CombatEvents = UCombatEventSubsystem::Get(this))
	{
		CombatEvents->UnfreezeScores();
	}
}

void ARespawnManager::ResetAllEnemies()
{
	EnemySpawner->ResetAll();
}

void ARespawnManager::ResetPlayer()
{
	AMSC_CharacterPlayer* Player = Cast<AMSC_CharacterPlayer>(
		UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

	Player->Destroy();
	GetWorld()->SpawnActor(PlayerClass);
}