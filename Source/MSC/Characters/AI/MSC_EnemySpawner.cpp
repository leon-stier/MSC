#include "MSC_EnemySpawner.h"

#include "AbilitySystemComponent.h"
#include "EngineUtils.h"
#include "GameplayTagContainer.h"
#include "MSC_CharacterEnemy.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "MSC/PlayerModel/CombatEventSubsystem.h"

AMSC_EnemySpawner::AMSC_EnemySpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// create the root
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// create the reference spawn capsule
	SpawnCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Spawn Capsule"));
	SpawnCapsule->SetupAttachment(RootComponent);

	SpawnCapsule->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	SpawnCapsule->SetCapsuleSize(35.0f, 90.0f);
	SpawnCapsule->SetCollisionProfileName(FName("NoCollision"));

	SpawnDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("Spawn Direction"));
	SpawnDirection->SetupAttachment(RootComponent);
}


void AMSC_EnemySpawner::BeginPlay()
{
	Super::BeginPlay();
	
	UCombatEventSubsystem* CombatEventSubsystem = UCombatEventSubsystem::Get(this);
	if (!CombatEventSubsystem) return;
	
	CombatEventSubsystem->OnSessionStateChanged.AddDynamic(this, &AMSC_EnemySpawner::OnSessionChanged);
}

void AMSC_EnemySpawner::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
}


void AMSC_EnemySpawner::Start()
{
	GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &AMSC_EnemySpawner::SpawnEnemy, InitialSpawnDelay);
}

void AMSC_EnemySpawner::SpawnEnemy()
{
    if (!IsValid(EnemyClass)) return;

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = 
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    for (int i = 0; i < SpawnCount; i++)
    {
        AMSC_CharacterEnemy* SpawnedEnemy = GetWorld()->SpawnActor<AMSC_CharacterEnemy>(
            EnemyClass, SpawnCapsule->GetComponentTransform(), SpawnParams);

        if (!IsValid(SpawnedEnemy) || !SpawnedEnemy->GetAbilitySystemComponent()) continue;

        SpawnedEnemies.Add(SpawnedEnemy);
        CurrentlyAlive++;

        FDelegateHandle Handle = SpawnedEnemy->GetAbilitySystemComponent()
            ->RegisterGameplayTagEvent(
                FGameplayTag::RequestGameplayTag(FName("Combat.Dead")),
                EGameplayTagEventType::NewOrRemoved)
            .AddLambda([this, SpawnedEnemy](const FGameplayTag Tag, int32 NewCount)
            {
                if (NewCount > 0)
                {
                    OnEnemyDied(SpawnedEnemy);
                }
            });

        DeadTagEventHandles.Add(SpawnedEnemy, Handle);
    }
}

void AMSC_EnemySpawner::OnEnemyDied(AMSC_CharacterEnemy* Enemy)
{
    // Clean up delegate
    if (FDelegateHandle* Handle = DeadTagEventHandles.Find(Enemy))
    {
        Enemy->GetAbilitySystemComponent()
            ->RegisterGameplayTagEvent(
                FGameplayTag::RequestGameplayTag(FName("Combat.Dead")),
                EGameplayTagEventType::NewOrRemoved)
            .Remove(*Handle);

        DeadTagEventHandles.Remove(Enemy);
    }

    SpawnedEnemies.Remove(Enemy);
    CurrentlyAlive--;

    // Schedule despawn
    FTimerHandle DespawnTimer;
    GetWorldTimerManager().SetTimer(DespawnTimer, [Enemy]()
    {
        if (IsValid(Enemy))
        {
            Enemy->Destroy();
        }
    }, 3.f, false);

    // Spawn next wave if all dead
    if (CurrentlyAlive <= 0)
    {
        GetWorldTimerManager().SetTimer(SpawnTimer, this,
            &AMSC_EnemySpawner::SpawnEnemy, RespawnDelay);
    }
}

void AMSC_EnemySpawner::ResetAll()
{
	GetWorldTimerManager().ClearTimer(SpawnTimer);

	// Copy the array since destroying enemies may modify it via callbacks
	TArray<AMSC_CharacterEnemy*> EnemiesToDestroy = SpawnedEnemies;


	for (AMSC_CharacterEnemy* Enemy : EnemiesToDestroy)
	{
		if (!IsValid(Enemy)) continue;
		
		UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
		if (!IsValid(ASC)) 
		{
			Enemy->Destroy();
			continue;
		}

		// Remove delegate BEFORE destroying to prevent OnEnemyDied firing
		if (FDelegateHandle* Handle = DeadTagEventHandles.Find(Enemy))
		{
			Enemy->GetAbilitySystemComponent()
				->RegisterGameplayTagEvent(
					FGameplayTag::RequestGameplayTag(FName("Combat.Dead")),
					EGameplayTagEventType::NewOrRemoved)
				.Remove(*Handle);
		
			DeadTagEventHandles.Remove(Enemy);
		}

		Enemy->Destroy();
	}

		
	SpawnedEnemies.Empty();
	DeadTagEventHandles.Empty();
	CurrentlyAlive = 0;

}

void AMSC_EnemySpawner::OnSessionChanged(const ESessionState& NewState)
{
	if (NewState == ESessionState::Learning || NewState == ESessionState::Hints)
	{
		Start();
	}
	if (NewState == ESessionState::Baseline)
	{
		ResetAll();
		Start();
	}
	if (NewState == ESessionState::Idle)
	{
		ResetAll();
	}
}
