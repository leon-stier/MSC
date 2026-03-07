#include "MSC_CharacterPlayer.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/OverlapResult.h"
#include "AbilitySystemComponent.h"
#include "MSC/GAS/MSC_AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"


AMSC_CharacterPlayer::AMSC_CharacterPlayer()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false;
	// GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 400.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	
}

void AMSC_CharacterPlayer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	UpdateLockOnRotation(DeltaSeconds);
}

void AMSC_CharacterPlayer::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AMSC_CharacterPlayer::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AMSC_CharacterPlayer::DoJumpStart()
{
	Jump();
}

void AMSC_CharacterPlayer::DoJumpEnd()
{
	StopJumping();
}

void AMSC_CharacterPlayer::DoPunch()
{
	if (PunchAbility && MSC_AbilitySystemComponent)
	{
		MSC_AbilitySystemComponent->TryActivateAbilityByClass(PunchAbility);
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, FGameplayTag::RequestGameplayTag(FName("Event.ContinueCombo.Input")), FGameplayEventData());
	}
}

void AMSC_CharacterPlayer::DoBlockStart()
{
	UE_LOG(LogTemp, Log, TEXT("Blocking!"));
	if (MSC_AbilitySystemComponent && BlockAbility)
	{
		MSC_AbilitySystemComponent->TryActivateAbilityByClass(BlockAbility);
	}
}

void AMSC_CharacterPlayer::DoBlockEnd()
{
	if (MSC_AbilitySystemComponent && BlockAbility)
	{
		FGameplayTagContainer AbilityTags;
		AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Block")));
		MSC_AbilitySystemComponent->CancelAbilities(&AbilityTags);
	}
	
}

void AMSC_CharacterPlayer::DoLockTarget()
{
	if (HitTarget)
	{
		HitTarget = nullptr;
		return;
	}

	const FVector Start = GetActorLocation();
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	
	// DrawDebugSphere(GetWorld(), Start, 700.0f, 16, FColor::Green, false, 2.f);

	const bool bHit = GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		Start,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(700.0f),
		QueryParams
	);
	
	if (!bHit) return;
	
	AActor* BestTarget = nullptr;
	float BestScore = -1.f;

	const FVector Forward = GetActorForwardVector();

	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* Candidate = Result.GetActor();
		if (!Candidate) continue;

		FVector Direction = (Candidate->GetActorLocation() - Start).GetSafeNormal();
		const float Dot = FVector::DotProduct(Forward, Direction);

		// Higher dot = more in front of player
		if (Dot > BestScore)
		{
			BestScore = Dot;
			BestTarget = Candidate;
		}
	}

	HitTarget = BestTarget;
}

void AMSC_CharacterPlayer::UpdateLockOnRotation(float DeltaTime)
{
	if (!HitTarget || !Controller) return;
	
	const FVector Start = GetActorLocation();
	const FVector TargetLocation = HitTarget->GetActorLocation();

	FVector Direction = (TargetLocation - Start);
	Direction.Z = 0.f; // Keep rotation horizontal

	const FRotator TargetRotation = Direction.Rotation();

	// Optional: Smooth rotation
	const FRotator NewRotation = FMath::RInterpTo(
		Controller->GetControlRotation(),
		TargetRotation,
		DeltaTime,
		10.f // Rotation speed
	);

	Controller->SetControlRotation(TargetRotation);
}


void AMSC_CharacterPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMSC_CharacterPlayer::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AMSC_CharacterPlayer::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMSC_CharacterPlayer::Look);
		EnhancedInputComponent->BindAction(LockTargetAction, ETriggerEvent::Triggered, this, &AMSC_CharacterPlayer::DoLockTarget);
		
		// Punch
		EnhancedInputComponent->BindAction(PunchAction, ETriggerEvent::Triggered, this, &AMSC_CharacterPlayer::DoPunch);
		
		// Block
		EnhancedInputComponent->BindAction(BlockAction, ETriggerEvent::Started, this, &AMSC_CharacterPlayer::DoBlockStart);
		EnhancedInputComponent->BindAction(BlockAction, ETriggerEvent::Completed, this, &AMSC_CharacterPlayer::DoBlockEnd);
		
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AMSC_CharacterPlayer::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AMSC_CharacterPlayer::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}


