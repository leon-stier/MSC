#include "MSC_EnemySpawner.h"

#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "MSC_CharacterEnemy.h"
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"

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

	GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &AMSC_EnemySpawner::SpawnEnemy, InitialSpawnDelay);
}

void AMSC_EnemySpawner::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
}

void AMSC_EnemySpawner::SpawnEnemy()
{
	// ensure the enemy class is valid
	if (IsValid(EnemyClass))
	{
		// spawn the enemy at the reference capsule's transform
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		for (int i = 0; i < SpawnCount; i++)
		{
			AMSC_CharacterEnemy* SpawnedEnemy = GetWorld()->SpawnActor<AMSC_CharacterEnemy>(
				EnemyClass, SpawnCapsule->GetComponentTransform(), SpawnParams);

			// was the enemy successfully created?
			if (SpawnedEnemy && SpawnedEnemy->GetAbilitySystemComponent())
			{
				// Listen for the dead tag on each enemy
				auto EventHandle = SpawnedEnemy->GetAbilitySystemComponent()->RegisterGameplayTagEvent(
					                               FGameplayTag::RequestGameplayTag(FName("Combat.Dead")),
					                               EGameplayTagEventType::NewOrRemoved)
				                               .AddLambda([this, SpawnedEnemy](
					                               const FGameplayTag Tag, int32 NewCount)
					                               {
						                               if (NewCount > 0)
						                               {
							                               FTimerHandle DespawnTimer;
							                               GetWorldTimerManager().SetTimer(
								                               DespawnTimer, [SpawnedEnemy]()
								                               {
									                               if (IsValid(SpawnedEnemy))
									                               {
										                               SpawnedEnemy->Destroy();
									                               }
								                               }, 3.0f, false);

							                               CurrentlyAlive--;

							                               if (CurrentlyAlive <= 0)
							                               {
								                               GetWorld()->GetTimerManager().SetTimer(
									                               SpawnTimer, this, &AMSC_EnemySpawner::SpawnEnemy,
									                               RespawnDelay);
							                               }
						                               }
					                               }
				                               );
				CurrentlyAlive++;
			}
		}
	}
}