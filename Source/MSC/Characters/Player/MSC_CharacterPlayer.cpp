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
#include "EnhancedInputSubsystems.h"
#include "MSC/Characters/AI/MSC_CharacterEnemy.h"


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
	GetCharacterMovement()->MinAnalogWalkSpeed = 0.f;
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
		AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Id.Block")));
		MSC_AbilitySystemComponent->CancelAbilities(&AbilityTags);
	}
	
}

void AMSC_CharacterPlayer::DoParry()
{
	if (MSC_AbilitySystemComponent && ParryAbility)
	{
		MSC_AbilitySystemComponent->TryActivateAbilityByClass(ParryAbility);
	}
}

void AMSC_CharacterPlayer::DoDodge()
{
	if (MSC_AbilitySystemComponent && DodgeAbility)
	{
		MSC_AbilitySystemComponent->TryActivateAbilityByClass(DodgeAbility);
	}
}

void AMSC_CharacterPlayer::DoLockTarget()
{
	if (HitTarget)
	{
		UnlockTarget();
		return;
	}

	const FVector Start = GetActorLocation();
	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	
	const bool bHit = GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		Start,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(TargetLockDistance),
		QueryParams
	);
	
	if (!bHit) return;
	
	AMSC_CharacterEnemy* BestTarget = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();
	float BestDot = -1.0f;
	const float SimilarDistanceThresholdSq = FMath::Square(TargetLockSimilarDistanceThreshold);

	const FVector Forward = GetActorForwardVector();

	for (const FOverlapResult& Result : OverlapResults)
	{
		AMSC_CharacterEnemy* Candidate = Cast<AMSC_CharacterEnemy>(Result.GetActor());
		if (!Candidate) continue;
		
		static FGameplayTagContainer DeadTags;
		DeadTags.AddTag(FGameplayTag::RequestGameplayTag("Combat.Dead"));
		DeadTags.AddTag(FGameplayTag::RequestGameplayTag("Combat.Dying"));
		if (Candidate->MSC_AbilitySystemComponent->HasAnyMatchingGameplayTags(DeadTags)) continue;

		const FVector ToCandidate = Candidate->GetActorLocation() - Start;
		const float DistanceSq = ToCandidate.SizeSquared();
		const FVector Direction = ToCandidate.GetSafeNormal();
		const float Dot = FVector::DotProduct(Forward, Direction);

		const bool bIsClearlyCloser = DistanceSq + SimilarDistanceThresholdSq < BestDistanceSq;
		const bool bIsSimilarDistance = FMath::Abs(DistanceSq - BestDistanceSq) <= SimilarDistanceThresholdSq;
		const bool bIsBetterFrontPreference = bIsSimilarDistance && Dot > BestDot;

		if (BestTarget == nullptr || bIsClearlyCloser || bIsBetterFrontPreference)
		{
			BestTarget = Candidate;
			BestDistanceSq = DistanceSq;
			BestDot = Dot;
		}
	}
	if (BestTarget == nullptr) return;

	SetLockedTarget(BestTarget);
	
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = 
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
			GetLocalViewingPlayerController()->GetLocalPlayer()))
	{
		Subsystem->RemoveMappingContext(LookMappingContext);
	}
}

void AMSC_CharacterPlayer::HandleLockSwitchInput(const FInputActionValue& Value)
{
	if (!HitTarget || !GetWorld())
	{
		return;
	}

	const FVector2D SwitchAxis = Value.Get<FVector2D>();
	const float Horizontal = SwitchAxis.X;

	if (FMath::Abs(Horizontal) < LockSwitchDeadzone)
	{
		bLockSwitchAxisLatched = false;
		return;
	}

	if (bLockSwitchAxisLatched)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastLockSwitchTime < LockSwitchCooldownSeconds)
	{
		return;
	}

	const int32 SideSign = Horizontal > 0.0f ? 1 : -1;
	if (AMSC_CharacterEnemy* SideTarget = FindBestSideLockTarget(SideSign))
	{
		SetLockedTarget(SideTarget);
		LastLockSwitchTime = Now;
	}

	// Latch until the stick is released to avoid repeated switching per deflection.
	bLockSwitchAxisLatched = true;
}

void AMSC_CharacterPlayer::OnLockSwitchReleased(const FInputActionValue& Value)
{
	bLockSwitchAxisLatched = false;
}

AMSC_CharacterEnemy* AMSC_CharacterPlayer::FindBestSideLockTarget(const int32 SideSign) const
{
	if (!HitTarget)
	{
		return nullptr;
	}

	const FVector Start = GetActorLocation();
	FVector CurrentDirection = HitTarget->GetActorLocation() - Start;
	CurrentDirection.Z = 0.0f;
	if (CurrentDirection.IsNearlyZero())
	{
		return nullptr;
	}
	CurrentDirection.Normalize();

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bHit = GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		Start,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(TargetLockDistance),
		QueryParams
	);
	if (!bHit)
	{
		return nullptr;
	}

	AMSC_CharacterEnemy* BestTarget = nullptr;
	float BestAngle = TNumericLimits<float>::Max();
	float BestDistanceSq = TNumericLimits<float>::Max();

	for (const FOverlapResult& Result : OverlapResults)
	{
		AMSC_CharacterEnemy* Candidate = Cast<AMSC_CharacterEnemy>(Result.GetActor());
		if (!Candidate || Candidate == HitTarget)
		{
			continue;
		}

		static FGameplayTagContainer DeadTags;
		DeadTags.AddTag(FGameplayTag::RequestGameplayTag("Combat.Dead"));
		DeadTags.AddTag(FGameplayTag::RequestGameplayTag("Combat.Dying"));
		if (Candidate->MSC_AbilitySystemComponent->HasAnyMatchingGameplayTags(DeadTags))
		{
			continue;
		}

		FVector CandidateDirection = Candidate->GetActorLocation() - Start;
		CandidateDirection.Z = 0.0f;
		if (CandidateDirection.IsNearlyZero())
		{
			continue;
		}
		const float DistanceSq = CandidateDirection.SizeSquared();
		CandidateDirection.Normalize();

		const float Dot = FVector::DotProduct(CurrentDirection, CandidateDirection);
		const float ClampedDot = FMath::Clamp(Dot, -1.0f, 1.0f);
		const float Angle = FMath::Acos(ClampedDot);
		const float CrossZ = FVector::CrossProduct(CurrentDirection, CandidateDirection).Z;
		const bool bOnRequestedSide = (SideSign > 0) ? (CrossZ > 0.0f) : (CrossZ < 0.0f);
		if (!bOnRequestedSide)
		{
			continue;
		}

		const bool bIsBetterAngle = Angle + KINDA_SMALL_NUMBER < BestAngle;
		const bool bSimilarAngle = FMath::Abs(Angle - BestAngle) <= KINDA_SMALL_NUMBER;
		const bool bIsCloserTieBreak = bSimilarAngle && DistanceSq < BestDistanceSq;
		if (BestTarget == nullptr || bIsBetterAngle || bIsCloserTieBreak)
		{
			BestTarget = Candidate;
			BestAngle = Angle;
			BestDistanceSq = DistanceSq;
		}
	}

	return BestTarget;
}

void AMSC_CharacterPlayer::UpdateLockOnRotation(float DeltaTime)
{
	if (!HitTarget || !Controller) return;

	const FVector Start = GetActorLocation();
	FVector Direction = HitTarget->GetActorLocation() - Start;
	Direction.Z = 0.f;

	if (Direction.IsNearlyZero()) return;

	const FRotator TargetRotation = Direction.Rotation();

	const float LockOnInterpSpeed = 12.0f;
	const FRotator NewRotation = FMath::RInterpTo(
		Controller->GetControlRotation(),
		TargetRotation,
		DeltaTime,
		LockOnInterpSpeed
	);

	Controller->SetControlRotation(NewRotation);
}

bool AMSC_CharacterPlayer::RequestAttackToken()
{	
	if (CurrentAttackerTokens > 0)
	{
		--CurrentAttackerTokens;
		return true;
	}
	return false;
}

void AMSC_CharacterPlayer::ReturnAttackToken()
{
	CurrentAttackerTokens++;
	if (CurrentAttackerTokens > MaxAllowedAttackers)
	{
		CurrentAttackerTokens = MaxAllowedAttackers;
	}
}

AMSC_CharacterEnemy* AMSC_CharacterPlayer::GetHitTarget() const
{
	return HitTarget;
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
		if (LockSwitchAction)
		{
			EnhancedInputComponent->BindAction(LockSwitchAction, ETriggerEvent::Triggered, this, &AMSC_CharacterPlayer::HandleLockSwitchInput);
			EnhancedInputComponent->BindAction(LockSwitchAction, ETriggerEvent::Completed, this, &AMSC_CharacterPlayer::OnLockSwitchReleased);
			EnhancedInputComponent->BindAction(LockSwitchAction, ETriggerEvent::Canceled, this, &AMSC_CharacterPlayer::OnLockSwitchReleased);
		}
		
		// Punch
		EnhancedInputComponent->BindAction(PunchAction, ETriggerEvent::Triggered, this, &AMSC_CharacterPlayer::DoPunch);
		
		// Block
		EnhancedInputComponent->BindAction(BlockAction, ETriggerEvent::Started, this, &AMSC_CharacterPlayer::DoBlockStart);
		EnhancedInputComponent->BindAction(BlockAction, ETriggerEvent::Completed, this, &AMSC_CharacterPlayer::DoBlockEnd);
		
		// Parry
		EnhancedInputComponent->BindAction(ParryAction, ETriggerEvent::Triggered, this, &AMSC_CharacterPlayer::DoParry);
		
		// Dodge
		EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Triggered, this, &AMSC_CharacterPlayer::DoDodge);
		
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

void AMSC_CharacterPlayer::OnTargetDied(const FGameplayTag Tag, int32 NewCount)
{
	UnlockTarget();
	DoLockTarget();
}

void AMSC_CharacterPlayer::UnlockTarget()
{
	if (HitTarget)
	{
		SetLockedTarget(nullptr);
		
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = 
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
			GetLocalViewingPlayerController()->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(LookMappingContext, LookMappingContextPriority);
		}
	}
}

void AMSC_CharacterPlayer::SetLockedTarget(AMSC_CharacterEnemy* NewTarget)
{
	if (HitTarget == NewTarget)
	{
		return;
	}

	if (HitTarget)
	{
		HitTarget->SetIsLockTarget(false);
		HitTarget->GetAbilitySystemComponent()->RegisterGameplayTagEvent(
			FGameplayTag::RequestGameplayTag(FName("Combat.Dying")),
			EGameplayTagEventType::NewOrRemoved)
			.Remove(DeadTagEventHandle);
	}

	HitTarget = NewTarget;

	if (HitTarget)
	{
		HitTarget->SetIsLockTarget(true);
		DeadTagEventHandle = HitTarget->GetAbilitySystemComponent()->RegisterGameplayTagEvent(
			FGameplayTag::RequestGameplayTag(FName("Combat.Dying")),
			EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &AMSC_CharacterPlayer::OnTargetDied);
	}
}
