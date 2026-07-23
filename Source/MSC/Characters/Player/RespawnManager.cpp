#include "RespawnManager.h"

#include "EngineUtils.h"
#include "MSC_CharacterPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "MSC/Characters/AI/MSC_CharacterEnemy.h"
#include "MSC/Characters/AI/MSC_EnemySpawner.h"
#include "MSC/PlayerModel/CombatEventSubsystem.h"

void ARespawnManager::BeginPlay()
{
	Super::BeginPlay();
	
	if (UCombatEventSubsystem* CombatEvents = UCombatEventSubsystem::Get(this))
	{
		CombatEvents->OnSessionStateChanged.AddDynamic(this, &ARespawnManager::OnSessionStateChanged);
	}
}

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
	UE_LOG(LogTemp, Warning, TEXT("Respawn timer complete. Resetting player and enemies."));
	ResetAllEnemies();
	ResetPlayer();
	EnemySpawner->Start();

	if (UCombatEventSubsystem* CombatEvents = UCombatEventSubsystem::Get(this))
	{
		CombatEvents->UnfreezeScores();
	}
}

void ARespawnManager::OnSessionStateChanged(const ESessionState& NewState)
{
	if (NewState == ESessionState::Idle || NewState == ESessionState::Baseline)
	{
		ResetPlayer();
	}
}

void ARespawnManager::ResetAllEnemies()
{
	EnemySpawner->ResetAll();
}

void ARespawnManager::ResetPlayer()
{

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;
	PC->GetPawn()->Destroy();
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	AMSC_CharacterPlayer* NewPlayer = GetWorld()->SpawnActor<AMSC_CharacterPlayer>(PlayerClass, SpawnParams);

	if (IsValid(NewPlayer))
	{
		PC->Possess(NewPlayer);
	}
	
}