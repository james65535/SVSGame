// Copyright Epic Games, Inc. All Rights Reserved.

#include "Players/SpyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Players/IsoCameraActor.h"
#include "Players/PlayerInteractionComponent.h"
#include "Rooms/SVSRoom.h"
#include "Players/IsoCameraComponent.h"
#include "Players/SVSPlayerController.h"
#include "net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

//////////////////////////////////////////////////////////////////////////
// ASpyCharacter

ASpyCharacter::ASpyCharacter()
{
	bReplicates = true;
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);

	PlayerInteractionComponent = CreateDefaultSubobject<UPlayerInteractionComponent>("Player Interaction Component");
	PlayerInteractionComponent->SetupAttachment(RootComponent);
	PlayerInteractionComponent->SetRelativeLocation(FVector(25.0f, 0.0f, 0.0f));
	PlayerInteractionComponent->SetIsReplicated(true);
	
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	
	FollowCamera = CreateDefaultSubobject<UIsoCameraComponent>(TEXT("FollowCamera"));
	
	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void ASpyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	SharedParams.RepNotifyCondition = REPNOTIFY_Always;
	SharedParams.Condition = COND_SimulatedOnly;

	DOREPLIFETIME_WITH_PARAMS_FAST(ASpyCharacter, bIsHiddenInGame, SharedParams);
}

void ASpyCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("Char: %s Role: %d"), *GetName(), GetLocalRole());
	/** Hide opponents character on local client */
	//!(GetLocalRole() == ROLE_SimulatedProxy) ? bIsHiddenInGame = true : bIsHiddenInGame = false; 
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		UE_LOG(LogTemp, Warning, TEXT("Authority ran"));
		bIsHiddenInGame = false;
	} else
	{
		UE_LOG(LogTemp, Warning, TEXT("No Authority ran"));
		bIsHiddenInGame = true;
	}
	SetActorHiddenInGame(bIsHiddenInGame);
	MARK_PROPERTY_DIRTY_FROM_NAME(ASpyCharacter, bIsHiddenInGame, this);
	
	//Add Input Mapping Context
	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ASpyCharacter::UpdateCameraLocation(const ASVSRoom* InRoom) const
{
	if (!IsValid(FollowCamera) && !IsValid(InRoom)) { return; }
	
    const ASVSPlayerController* PlayerController = Cast<ASVSPlayerController>(Controller);
	if (AIsoCameraActor* IsoCameraActor = Cast<AIsoCameraActor>(PlayerController->GetViewTarget()))
	{
		IsoCameraActor->SetRoomTarget(InRoom);
	}
}

void ASpyCharacter::SetSpyHidden(const bool bIsSpyHidden)
{
	bIsHiddenInGame = bIsSpyHidden;
	MARK_PROPERTY_DIRTY_FROM_NAME(ASpyCharacter, bIsHiddenInGame, this);
}

//////////////////////////////////////////////////////////////////////////
// Input

void ASpyCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASpyCharacter::Move);

		// Looking
		//EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASpyCharacter::Look);

		// Interacting
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Completed, this, &ASpyCharacter::Interact);
	}

}

void ASpyCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

// void ASpyCharacter::Look(const FInputActionValue& Value)
// {
// 	// input is a Vector2D
// 	const FVector2D LookAxisVector = Value.Get<FVector2D>();
//
// 	if (Controller != nullptr)
// 	{
// 		// add yaw and pitch input to controller
// 		AddControllerYawInput(LookAxisVector.X);
// 		AddControllerPitchInput(LookAxisVector.Y);
// 	}
// }

void ASpyCharacter::Interact(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("Character Triggered Interact"));
	if (IsValid(PlayerInteractionComponent) && GetPlayerInteractionComponent()->bCanInteractWithActor)
	{
		GetPlayerInteractionComponent()->RequestInteractWithObject();
	}
}




