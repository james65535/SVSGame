// Copyright Epic Games, Inc. All Rights Reserved.

#include "Players/SpyCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Players/IsoCameraActor.h"
#include "Players/IsoCameraComponent.h"
#include "Players/SpyPlayerController.h"
#include "Players/SpyInteractionComponent.h"
#include "Players/SpyCombatantInterface.h"
#include "Players/SpyPlayerState.h"
#include "Items/InventoryComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "SVSLogger.h"
#include "AbilitySystem/SpyGameplayAbility.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "AbilitySystem/SpyAttributeSet.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Items/InteractInterface.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Rooms/SVSRoom.h"
#include "net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Rooms/SpyFurniture.h"
#include "SpyVsSpy/SpyVsSpy.h"

ASpyCharacter::ASpyCharacter()
{
	bReplicates = true;

	/** ASC Binding occurs in setupinput and onrep_playerstate due possibility of race condition
	 * bool prevents duplicate binding as it is called in both methods */
	bAbilitySystemComponentBound = false;
	SpyDeadTag = FGameplayTag::RequestGameplayTag("State.Dead");
	
	/** Set size for collision capsule */
	GetCapsuleComponent()->InitCapsuleSize(35.0f, 90.0f);

	SpyInteractionComponent = CreateDefaultSubobject<USpyInteractionComponent>("Interaction Component");
	SpyInteractionComponent->SetupAttachment(RootComponent);
	SpyInteractionComponent->SetRelativeLocation(FVector(25.0f, 0.0f, 0.0f));
	SpyInteractionComponent->SetIsReplicated(true);

	PlayerInventoryComponent = CreateDefaultSubobject<UInventoryComponent>("Inventory Component");
	PlayerInventoryComponent->SetIsReplicated(true);
	
	/** Don't rotate when the controller rotates. Let that just affect the camera. */
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	/** Configure character movement */
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	/** Note: For faster iteration times these variables, and many more, can be tweaked in the
	 * Character Blueprint instead of recompiling to adjust them */
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	
	FollowCamera = CreateDefaultSubobject<UIsoCameraComponent>(TEXT("FollowCamera"));

	// TODO replace
	AttackZone = CreateDefaultSubobject<USphereComponent>("AttackZoneSphere");
	AttackZone->SetupAttachment(GetCapsuleComponent());
	AttackZone->SetSphereRadius(25.0f);
	AttackZone->SetHiddenInGame(false);
	AttackZone->SetVisibility(true);
	AttackZone->SetRelativeLocation(FVector(30.0f, 0.0f, 40.0f));
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

void ASpyCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	BindAbilitySystemComponentInput();
}

void ASpyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!IsValid(SpyPlayerState))
	{ SpyPlayerState = GetPlayerStateChecked<ASpyPlayerState>(); }
	
	AbilitySystemComponentInit();
	AddStartupGameplayAbilities();
}

ASpyPlayerState* ASpyCharacter::GetSpyPlayerState() const
{
	return CastChecked<ASpyPlayerState>(GetPlayerState(), ECastCheckedType::NullAllowed);
}

void ASpyCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (!IsValid(SpyPlayerState))
	{ SpyPlayerState = GetPlayerState<ASpyPlayerState>(); }
	
	AbilitySystemComponentInit();
	BindAbilitySystemComponentInput();
}

void ASpyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	/** Hide opponents character on local client */
	GetLocalRole() == ROLE_SimulatedProxy ? bIsHiddenInGame = true : bIsHiddenInGame = false;
	SetSpyHidden(bIsHiddenInGame);

	/** Add delegates for Character Overlaps */
	OnActorBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnOverlapBegin);
	OnActorEndOverlap.AddUniqueDynamic(this, &ThisClass::OnOverlapEnd);
}

void ASpyCharacter::OnOverlapBegin(AActor* OverlappedActor, AActor* OtherActor)
{
	/* If OtherActor is a Room then capture the room which character is trying to enter */
	if (ASVSRoom* SVSRoomOverlapped = Cast<ASVSRoom>(OtherActor))
	{
		/** Prep for room transfer if already in a room,
		 * this multi part process helps reduce change of bugs due to character clipping
		 * as they cannot be in a new room if they have not left the old room */
		RoomEntering = SVSRoomOverlapped;

		/** Handle logic for start of the game where current room will be null and
		 * existing room traversal checks require the previous current room to end overlap so
		 * we provide need a way to avoid this check at start of game */
		if (!IsValid(CurrentRoom))
		{ ProcessRoomChange(RoomEntering); }
	}
}

void ASpyCharacter::OnOverlapEnd(AActor* OverlappedActor, AActor* OtherActor)
{
	/** If OtherActor is a Room then finalise Room change if
	 * room from overlap end is indeed the previous current room */
	if (const ASVSRoom* OverlapEndRoom = Cast<ASVSRoom>(OtherActor))
	{
		if (OverlapEndRoom == CurrentRoom && IsValid(RoomEntering))
		{ ProcessRoomChange(RoomEntering); }
		else
		{ UE_LOG(SVSLogDebug, Log, TEXT("%s Character: %s ended overlap with room: %s but could not processroomchange as RoomEntering is not valid"), IsLocallyControlled() ? *FString("Local") : *FString("Remote"), *GetName(), *OtherActor->GetName()); }
	}
}

void ASpyCharacter::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!IsValid(SpyPlayerState) ||
		!SpyPlayerState->IsWinner()
		|| Montage != CelebrateMontage)
	{ return; }

	PlayAnimMontage(CelebrateMontage, 1.0f, TEXT("Winner"));
}

void ASpyCharacter::ProcessRoomChange(ASVSRoom* NewRoom)
{
	checkfSlow(NewRoom, "SpyCharacter attempted to change current room but room pointer is null");
	
	/** If character is local then hide any other characters in the room we are leaving */
	if (GetLocalRole() == ROLE_AutonomousProxy && IsValid(CurrentRoom))
	{ CurrentRoom->ChangeOpposingOccupantsVisibility(this, true); }
	
	/** Handle Remote character changing room */
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		// TODO check room is occupied flag
		
		/* Is new room occupied by local player */
		if (NewRoom->bRoomLocallyHiddenInGame == false)
		{ SetSpyHidden(false); }
		/* Is old room occupied by local player */
		else if (IsValid(CurrentRoom) && !CurrentRoom->bRoomLocallyHiddenInGame)
		{ SetSpyHidden(true); }
	}
	
	/** The room we entered is now officially the current room */
	CurrentRoom = NewRoom;
	
	/** Update camera for local character */
	if (GetLocalRole() == ROLE_AutonomousProxy && IsValid(CurrentRoom))
	{ UpdateCameraLocation(CurrentRoom); }
	
	/** If character is local then Unhide characters in the room we entered */
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{ RoomEntering->ChangeOpposingOccupantsVisibility(this, false); }

	/** Clean up temporary pointer used for traversal process */
	RoomEntering = nullptr;
}

void ASpyCharacter::SetSpyHidden(const bool bIsSpyHidden)
{
	bIsHiddenInGame = bIsSpyHidden;
	SetActorHiddenInGame(bIsSpyHidden);
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, bIsHiddenInGame, this); // TODO this probably doesn't need to be replicated since hiding a local only thing
}

bool ASpyCharacter::PlayCelebrateMontage()
{
	const bool bPlayedSuccessfully = PlayAnimMontage(CelebrateMontage, 1.0f) > 0.f;

	if (bPlayedSuccessfully)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

		if (!MontageEndedDelegate.IsBound())
		{ MontageEndedDelegate.BindUObject(this, &ThisClass::OnMontageEnded); }

		AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, CelebrateMontage);
	}

	return bPlayedSuccessfully;
}

void ASpyCharacter::NM_FinishedMatch_Implementation()
{
	if (ASpyPlayerController* SpyPlayerController = Cast<ASpyPlayerController>(GetController()))
	{ SpyPlayerController->FinishedMatch(); }
	GetCharacterMovement()->DisableMovement();
	PlayCelebrateMontage();

	// TODO may need a timer or animation notify for destroy
	Destroy();
}

void ASpyCharacter::UpdateCameraLocation(const ASVSRoom* InRoom) const
{
	/** Just process this logic on the client if all pointers are valid */
	if (!IsValid(FollowCamera) ||
		!IsValid(InRoom) ||
		!IsLocallyControlled() ||
		GetLocalRole() != ROLE_AutonomousProxy)
	{ return; }
	
	const ASpyPlayerController* PlayerController = Cast<ASpyPlayerController>(Controller);
	if (AIsoCameraActor* IsoCameraActor = Cast<AIsoCameraActor>(PlayerController->GetViewTarget()))
	{ IsoCameraActor->SetRoomTarget(InRoom); }
}

void ASpyCharacter::BindAbilitySystemComponentInput()
{
	if (bAbilitySystemComponentBound ||
		!IsValid(SpyAbilitySystemComponent)
		|| !IsValid(InputComponent)) { return; }
	
	const FTopLevelAssetPath AbilityEnumAssetPath = FTopLevelAssetPath(
		FName(PROJECT_PATH),
		FName(ABILITY_INPUT_ID));

	const FGameplayAbilityInputBinds Binds(
		"Confirm",
		"Cancel",
		AbilityEnumAssetPath,
		static_cast<int32>(ESVSAbilityInputID::Confirm),
		static_cast<int32>(ESVSAbilityInputID::Cancel));
	SpyAbilitySystemComponent->BindAbilityActivationToInputComponent(InputComponent, Binds);

	bAbilitySystemComponentBound = true;
}

UAbilitySystemComponent* ASpyCharacter::GetAbilitySystemComponent() const
{
	return SpyAbilitySystemComponent;
}

void ASpyCharacter::AbilitySystemComponentInit()
{
	if (IsValid(SpyPlayerState))
	{
		SpyAbilitySystemComponent = SpyPlayerState->GetSpyAbilitySystemComponent();
		SpyAbilitySystemComponent->InitAbilityActorInfo(SpyPlayerState, this);
	}
}

void ASpyCharacter::AddStartupGameplayAbilities()
{
	/** Grant abilities only on the server */
	if (GetLocalRole() != ROLE_Authority || !IsValid(SpyPlayerState)) { return; }
	
	if (IsValid(SpyAbilitySystemComponent) && !SpyAbilitySystemComponent->bCharacterAbilitiesGiven)
	{
		/** Apply abilities */
		for (TSubclassOf<USpyGameplayAbility>& StartupAbility : GameplayAbilities)
		{
			UE_LOG(SVSLogDebug, Log, TEXT("Adding ability: %s"), *StartupAbility->GetName());
			SpyAbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(
				StartupAbility,
				1,
				static_cast<int32>(StartupAbility.GetDefaultObject()->AbilityInputID),
				this));
		}

		/** Apply Passives */
		for (const TSubclassOf<UGameplayEffect>& GameplayEffect : PassiveGameplayEffects)
		{
			FGameplayEffectContextHandle EffectContext = SpyAbilitySystemComponent->MakeEffectContext();
			EffectContext.AddSourceObject(this);

			FGameplayEffectSpecHandle EffectSpecHandle = SpyAbilitySystemComponent->MakeOutgoingSpec(
				GameplayEffect,
				1,
				EffectContext);

			if (EffectSpecHandle.IsValid())
			{
				FActiveGameplayEffectHandle ActiveGameplayEffectHandle =
					SpyAbilitySystemComponent->ApplyGameplayEffectSpecToTarget(
						*EffectSpecHandle.Data.Get(),
						SpyAbilitySystemComponent);
			}
		}
		SpyAbilitySystemComponent->bCharacterAbilitiesGiven = true;
	}
}

void ASpyCharacter::RemoveCharacterAbilities()
{
}

void ASpyCharacter::RequestDeath()
{
	if (!IsValid(SpyAbilitySystemComponent))
	{ return; }

	UE_LOG(SVSLog, Warning, TEXT("%s Character: %s has requested death, may they be at peace"), IsLocallyControlled() ? *FString("Local") : *FString("Remote"), *GetName());
	
	/** Only runs on Server */
	RemoveCharacterAbilities();

	GetCapsuleComponent()->DestroyComponent();
	GetMesh()->SetCollisionProfileName("Ragdoll", true);
	GetMesh()->SetAllBodiesSimulatePhysics(true);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->WakeAllRigidBodies();
	GetCharacterMovement()->SetMovementMode(MOVE_None);
	//GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// GetCharacterMovement()->GravityScale = 0;
	// GetCharacterMovement()->Velocity = FVector(0);
	
	SpyPlayerState->ReduceRemainingMatchTime();
	OnCharacterDied.Broadcast(this);
	
	SpyAbilitySystemComponent->CancelAllAbilities();

	FGameplayTagContainer EffectTagsToRemove;
	EffectTagsToRemove.AddTag(EffectRemoveOnDeathTag);
	int32 NumEffectsRemoved = SpyAbilitySystemComponent->RemoveActiveEffectsWithTags(EffectTagsToRemove);

	SpyAbilitySystemComponent->AddLooseGameplayTag(SpyDeadTag);

	// TODO replace with a locally executed GameplayCue
	// if (DeathSound)
	// {
	// 	UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	// }

	// if (DeathMontage)
	// {
	// 	PlayAnimMontage(DeathMontage);
	// }
	// else
	// {
	// 	FinishDeath();
	// }

	// TODO could get rid of timer and respond to a notify
	GetWorld()->GetTimerManager().SetTimer(
			FinishDeathTimerHandle,
			this,
			&ThisClass::FinishDeath,
			FinishDeathRateSeconds,
			false);
}

void ASpyCharacter::FinishDeath()
{
	// TODO Verify HasAuthority check is sufficient for multiplayer server/client architecture
	if (!HasAuthority())
	{ return; }
	UE_LOG(SVSLog, Warning, TEXT("%s Character: %s is finishing their death process - RIP"), IsLocallyControlled() ? *FString("Local") : *FString("Remote"), *GetName());
	// TODO this should all be clean up stuff
	//Destroy();
}

void ASpyCharacter::HandlePrimaryAttack()
{
	if (GetLocalRole() != ROLE_Authority)
	{ return; }
	
	UE_LOG(SVSLogDebug, Log, TEXT("Handling Attack"));

	// TODO Update to accomodate the movement of the attacking weapon and time delays which may occur
	/** Check if weapon attack capsule collides with a valid target */
	TArray<AActor*> ActorsArray;
	AttackZone->GetOverlappingActors(ActorsArray);
	int Count = 0;
	if (ActorsArray.Num() > 0)
	{
		
		for (AActor* Actor : ActorsArray)
		{
			UE_LOG(SVSLogDebug, Log, TEXT("Handling Attack check - ActorHasTag: %s"), Actor->ActorHasTag(CombatantTag) ? *FString("True") : *FString("False"));

			if (Actor &&
				Actor != this &&
				Actor->ActorHasTag(CombatantTag) &&
				UKismetSystemLibrary::DoesImplementInterface(Actor, USpyCombatantInterface::StaticClass()) &&
				UKismetSystemLibrary::DoesImplementInterface(Actor, UAbilitySystemInterface::StaticClass()))
			{
				UE_LOG(SVSLogDebug, Log, TEXT("Found Enemy: %s"), *Actor->GetName());
				/** Don't process attack if enemy is dead */
				if (Cast<IAbilitySystemInterface>(Actor)->GetAbilitySystemComponent()->HasMatchingGameplayTag(
						FGameplayTag::RequestGameplayTag("State.Dead")))
				{
					UE_LOG(SVSLogDebug, Log, TEXT("Found IsDead")) // TODO Revisit this to remove or buildout
					continue;
				}

				UE_LOG(SVSLogDebug, Log, TEXT("Applying Attack Force"))
				// TODO sort out attack force and consider multiplayer aspect/multicast
				constexpr float AttackForce = 750.0f;
				Cast<ISpyCombatantInterface>(Actor)->ApplyAttackImpactForce(GetActorLocation(), AttackForce);

				const FGameplayTag Tag = FGameplayTag::RequestGameplayTag("Attack.Hit");
				FGameplayEventData Payload = FGameplayEventData();
				Payload.Instigator = GetInstigator();
				Payload.Target = Actor;
				Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(Actor);
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetInstigator(), Tag, Payload);

				++Count;
			}
		}
	}
	
	/** if our Count returns 0, it means we did not hit an enemy and we should end our ability */
	if (Count == 0)
	{
		UE_LOG(SVSLogDebug, Log, TEXT("Spy Attack hit zero combatants"))
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag("Attack.NoHit");
		FGameplayEventData Payload = FGameplayEventData();
		Payload.Instigator = GetInstigator();
		Payload.TargetData = FGameplayAbilityTargetDataHandle();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetInstigator(), Tag, Payload);
	}
}

void ASpyCharacter::NM_PlayAttackAnimation_Implementation(const float TimerValue)
{
	if (IsRunningDedicatedServer())
	{ return; }
	
	UE_LOG(SVSLogDebug, Log, TEXT("%s Character: %s Triggered Play Attack Anim Montage"), IsLocallyControlled() ? *FString("Local") : *FString("Remote"), *GetName());
	PlayAnimMontage(AttackMontage);
}

void ASpyCharacter::HandleTrapTrigger()
{
	if (GetLocalRole() != ROLE_Authority)
	{ return; }

	ASpyFurniture* FurnitureActor = nullptr;
	if (GetInteractionComponent()->CanInteractWithKnownInteractionInterface())
	{
		FurnitureActor = Cast<ASpyFurniture>(
			GetInteractionComponent()->
			GetLatestInteractableComponent()->
			Execute_GetInteractableOwner(
				GetInteractionComponent()->GetLatestInteractableComponent().GetObjectRef()));
	}
	
	UE_LOG(SVSLogDebug, Log, TEXT("%s Victim: %s Handling Trap Trigger"), IsLocallyControlled() ? *FString("Local") : *FString("Remote"), *GetName());
	UE_LOG(SVSLogDebug, Log, TEXT("%s Instigator: %s Handling Trap Trigger"), GetInstigator()->IsLocallyControlled() ? *FString("Local") : *FString("Remote"), *GetInstigator()->GetName());
	// TODO sort out attack force and consider multiplayer aspect/multicast
	constexpr float AttackForce = 1500.0f;
	FVector AttackOrigin = FVector::ZeroVector;

	if (GetInteractionComponent()->CanInteractWithKnownInteractionInterface())
	{
		AttackOrigin = GetInteractionComponent()->
		GetLatestInteractableComponent()->
		Execute_GetInteractableOwner(
			GetInteractionComponent()->GetLatestInteractableComponent().GetObjectRef())->GetActorLocation();
	}

	Cast<ISpyCombatantInterface>(this)->ApplyAttackImpactForce(AttackOrigin, AttackForce);

	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag("TrapTrigger.Hit");
	FGameplayEventData Payload = FGameplayEventData();

	if (IsValid(FurnitureActor))
	{
		Payload.Instigator = FurnitureActor;
		UE_LOG(SVSLogDebug, Log, TEXT("Using furniture as instigator"));
	}
	else
	{ Payload.Instigator = this; }

	Payload.Target = this;
	Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(this);
	
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, Tag, Payload);
	
	/** if our Count returns 0, it means we did not hit an enemy and we should end our ability */
	// TODO determine what would cause this trap tripper to no hit
	// if (Count == 0)
	// {
	// 	UE_LOG(SVSLogDebug, Log, TEXT("Spy Attack hit zero combatants"))
	// 	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag("Attack.NoHit");
	// 	FGameplayEventData Payload = FGameplayEventData();
	// 	Payload.Instigator = GetInstigator();
	// 	Payload.TargetData = FGameplayAbilityTargetDataHandle();
	// 	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetInstigator(), Tag, Payload);
	// }
}

void ASpyCharacter::NM_PlayTrapTriggerAnimation_Implementation(const float TimerValue)
{
	if (IsRunningDedicatedServer())
	{ return; }
	
	UE_LOG(SVSLogDebug, Log, TEXT("%s Character: %s Triggered Play Trap Trigger Anim Montage"), IsLocallyControlled() ? *FString("Local") : *FString("Remote"), *GetName());
	PlayAnimMontage(AttackMontage); // TODO update
}

void ASpyCharacter::ApplyAttackImpactForce(const FVector FromLocation, const float InAttackForce) const
{
	if (GetLocalRole() != ROLE_Authority)
	{ return; }
	
	NM_ApplyAttackImpactForce(FromLocation, InAttackForce);
}

void ASpyCharacter::NM_ApplyAttackImpactForce_Implementation(const FVector FromLocation,
	const float InAttackForce) const
{
	const FVector TargetLocation = GetActorLocation();
	const FVector Direction = UKismetMathLibrary::GetDirectionUnitVector(FromLocation, TargetLocation);

	UE_LOG(SVSLogDebug, Log, TEXT("Launching character after attack"));
	GetCharacterMovement()->Launch(FVector(
		Direction.X * InAttackForce,
		Direction.Y * InAttackForce,
		Direction.Z)); // TODO was: abs(Direction.Z + 1) * InAttackForce)
}

float ASpyCharacter::GetHealth() const
{
	if (IsValid(SpyPlayerState))
	{ return SpyPlayerState->GetHealth(); }
	return -1.0f; /** Return non-sensical value to indicate there is an issue */
}

float ASpyCharacter::GetMaxHealth() const
{
	if (IsValid(SpyPlayerState))
	{ return SpyPlayerState->GetMaxHealth(); }
	return -1.0f; /** Return non-sensical value to indicate there is an issue */
}

bool ASpyCharacter::IsAlive()
{
	return GetHealth() >= 1.0f;
}

void ASpyCharacter::Move(const FInputActionValue& Value)
{
	/** input is a Vector2D */
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		/** find out which way is forward */
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		/** get forward vector */
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		/** get right vector */
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		/** add movement */
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ASpyCharacter::RequestSprint(const FInputActionValue& Value)
{
}

void ASpyCharacter::RequestPrimaryAttack(const FInputActionValue& Value)
{
	if (!IsValid(SpyAbilitySystemComponent)) { return; }

	if (Value.Get<bool>())
	{
		SpyAbilitySystemComponent->AbilityLocalInputPressed(
			static_cast<int32>(ESVSAbilityInputID::PrimaryAttackAction));
	}
	else
	{
		SpyAbilitySystemComponent->AbilityLocalInputReleased(
			static_cast<int32>(ESVSAbilityInputID::PrimaryAttackAction));
	}
}

void ASpyCharacter::RequestNextTrap(const FInputActionValue& Value)
{
	// if (Inventory.Weapons.Num() < 2)
	// {
	// 	return;
	// }
	//
	// int32 CurrentWeaponIndex = Inventory.Weapons.Find(CurrentWeapon);
	// UnEquipCurrentWeapon();
	//
	// if (CurrentWeaponIndex == INDEX_NONE)
	// {
	// 	EquipWeapon(Inventory.Weapons[0]);
	// }
	// else
	// {
	// 	EquipWeapon(Inventory.Weapons[(CurrentWeaponIndex + 1) % Inventory.Weapons.Num()]);
	// }
}

void ASpyCharacter::RequestPreviousTrap(const FInputActionValue& Value)
{
	// if (Inventory.Weapons.Num() < 2)
	// {
	// 	return;
	// }
	//
	// int32 CurrentWeaponIndex = Inventory.Weapons.Find(CurrentWeapon);
	//
	// UnEquipCurrentWeapon();
	//
	// if (CurrentWeaponIndex == INDEX_NONE)
	// {
	// 	EquipWeapon(Inventory.Weapons[0]);
	// }
	// else
	// {
	// 	int32 IndexOfPrevWeapon = FMath::Abs(CurrentWeaponIndex - 1 + Inventory.Weapons.Num()) % Inventory.Weapons.Num();
	// 	EquipWeapon(Inventory.Weapons[IndexOfPrevWeapon]);
	// }
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

void ASpyCharacter::RequestInteract()
{
	if (!IsValid(GetInteractionComponent()) || !GetInteractionComponent()->CanInteractWithKnownInteractionInterface())
	{ return; }
	
	GetInteractionComponent()->RequestInteractWithObject();
}

void ASpyCharacter::RequestTrapTrigger()
{
	SpyAbilitySystemComponent->AbilityLocalInputPressed(
			static_cast<int32>(ESVSAbilityInputID::TrapTriggerAction));
}
