// Copyright Epic Games, Inc. All Rights Reserved.

#include "Players/SpyCharacter.h"

#include "Players/IsoCameraActor.h"
#include "Players/IsoCameraComponent.h"
#include "Players/SpyPlayerController.h"
#include "Players/SpyInteractionComponent.h"
#include "Players/SpyCombatantInterface.h"
#include "Players/SpyPlayerState.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "SVSLogger.h"
#include "AbilitySystem/SpyGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "AbilitySystem/SpyAttributeSet.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameModes/SpyVsSpyGameMode.h"
#include "Items/InventoryComponent.h"
#include "Items/InventoryWeaponAsset.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Rooms/RoomManager.h"
#include "Rooms/SVSRoom.h"
#include "SpyVsSpy/SpyVsSpy.h"

ASpyCharacter::ASpyCharacter()
{
	bReplicates = true;

	/** ASC Binding occurs in setupinput and onrep_playerstate due possibility of race condition
	 * bool prevents duplicate binding as it is called in both methods */
	bAbilitySystemComponentBound = false;
	//SpyDeadTag = FGameplayTag::RequestGameplayTag("State.Waiting");
	
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
	
	HatMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("HatMeshComponent");
	if (IsValid(HatMeshComponent))
	{
		HatMeshComponent->SetupAttachment(GetMesh(), FName("Hat_Socket"));
		HatMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
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

	/** Add delegate for server side */
	if (IsValid(SpyPlayerState) && !SpyPlayerState->OnSpyTeamUpdate.IsBoundToObject(this))
	{ SpyPlayerState->OnSpyTeamUpdate.AddUObject(this, &ThisClass::UpdateCharacterTeam); }
	
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

	/** Add delegate for client side */
	if (IsValid(SpyPlayerState) && !SpyPlayerState->OnSpyTeamUpdate.IsBoundToObject(this))
	{ SpyPlayerState->OnSpyTeamUpdate.AddUObject(this, &ThisClass::UpdateCharacterTeam); }

	AbilitySystemComponentInit();
	BindAbilitySystemComponentInput();
}

void ASpyCharacter::BeginPlay()
{
	Super::BeginPlay();

	/** InventoryComponent performs some logic depending on this value */
	PlayerInventoryComponent->SetInventoryOwnerType(EInventoryOwnerType::Player);
	
	// TODO check into whether this should be forced from server
	/** Hide opponents character on local client */
	GetLocalRole() == ROLE_SimulatedProxy ?
		bIsHiddenInGame = true :
		bIsHiddenInGame = false;
	SetSpyHidden(bIsHiddenInGame);

	// TODO test
	bHasTeleported = true;
	
	/** Add delegates for Character Overlaps */
	OnActorBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnOverlapBegin);
	OnActorEndOverlap.AddUniqueDynamic(this, &ThisClass::OnOverlapEnd);
	if (IsValid(PlayerInventoryComponent))
	{
		PlayerInventoryComponent->OnEquippedUpdated.AddUniqueDynamic(this, &ThisClass::EquippedItemUpdated);
	}
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
		if (!IsValid(CurrentRoom) || bHasTeleported)
		{
			/** Reset flag and process room change */
			bHasTeleported = false;
			ProcessRoomChange(RoomEntering);
		}
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
	}
}

void ASpyCharacter::OnCelebrationMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (!IsValid(SpyPlayerState) ||
		!SpyPlayerState->IsWinner() ||
		Montage != CelebrateMontage)
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
		if (NewRoom->IsRoomLocallyHidden() == false)
		{ SetSpyHidden(false); }
		/* Is old room occupied by local player */
		else if (IsValid(CurrentRoom) && !CurrentRoom->IsRoomLocallyHidden())
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

	/** Hide any weapons which are attached as actors */
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors);
	for (AActor* AttachedActor : AttachedActors)
	{ AttachedActor->SetActorHiddenInGame(bIsHiddenInGame); }
}

void ASpyCharacter::StartPrimaryAttackWindow()
{
	if (GetWorld()->GetNetMode() == NM_Client)
	{ return; }

	SetAttackActive(true);
}

void ASpyCharacter::CompletePrimaryAttackWindow()
{
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{ return; }

	/** Notify ability of zero hits */
	if (!bAttackHitFound)
	{
		SetAttackActive(false);
		/** Payload setup to send to GameplayEvent to Actor */
		const FGameplayTag ResultTag = FGameplayTag::RequestGameplayTag("Attack.NoHit");
		FGameplayEventData Payload = FGameplayEventData();
		Payload.Instigator = this;
		Payload.Target = nullptr;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, ResultTag, Payload);
	}
	bAttackHitFound = false;
}

bool ASpyCharacter::PlayCelebrateMontage()
{
	const bool bPlayedSuccessfully = PlayAnimMontage(CelebrateMontage, 1.0f) > 0.f;

	if (bPlayedSuccessfully)
	{
		if (!CelebrateMontageEndedDelegate.IsBound())
		{ CelebrateMontageEndedDelegate.BindUObject(this, &ThisClass::OnCelebrationMontageEnded); }
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		AnimInstance->Montage_SetEndDelegate(CelebrateMontageEndedDelegate, CelebrateMontage);
	}

	return bPlayedSuccessfully;
}

void ASpyCharacter::DisableSpyCharacter()
{
	SetSpyHidden(true);
	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);
}

UInventoryBaseAsset* ASpyCharacter::GetEquippedItemAsset() const
{
	if (IsValid(PlayerInventoryComponent))
	{ return PlayerInventoryComponent->GetEquippedItemAsset(); }
	return nullptr;
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
		static_cast<int32>(ESpyAbilityInputID::Confirm),
		static_cast<int32>(ESpyAbilityInputID::Cancel));
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
	else
	{
		UE_LOG(SVSLog, Warning, TEXT(
			"%s Character: %s could not get valid playerstate to init ability system component"),
			IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
			*GetName());
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
			const FGameplayAbilitySpecHandle AbilitySpecHandle = SpyAbilitySystemComponent->GiveAbility(
				FGameplayAbilitySpec(
					StartupAbility,
					1,
					static_cast<int32>(StartupAbility.GetDefaultObject()->AbilityInputID),
					this));
			/** Hang on to this to prevent array iteration via lookup when we need to call it later */
			if (static_cast<int32>(StartupAbility.GetDefaultObject()->AbilityInputID) ==
				static_cast<int32>(ESpyAbilityInputID::TrapTriggerAction))
			{ TrapTriggerSpecHandle = AbilitySpecHandle; }
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

// TODO OPTIMISATION - perhaps just send room ID instead of FVector
void ASpyCharacter::NM_SetEnableDeathState_Implementation(const bool bEnabled, const FVector RespawnLocation)
{
	SetEnableDeathState(bEnabled, RespawnLocation);
}

void ASpyCharacter::SetEnableDeathState(const bool bEnabled, const FVector& RespawnLocation)
{
	// TODO are these replicated as currently this func is called via NetMulticast
	if (bEnabled)
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->SetCollisionProfileName("Ragdoll", true);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetMesh()->SetAllBodiesSimulatePhysics(true);
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->WakeAllRigidBodies();
		return;
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetAllBodiesSimulatePhysics(false);
	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->PutAllRigidBodiesToSleep();
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetMesh()->SetCollisionProfileName(SpyMeshCollisionProfile, true);
	GetMesh()->AttachToComponent(GetCapsuleComponent(),
		FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true));
	GetMesh()->SetRelativeLocationAndRotation(
		FVector(0.0f, 0.0f, -90.0f),
		FRotator(0.0f, 270.0f, 0.0f));

	/** Set weapon back to default */
	InitializeEquippedItem();

	/** Process character relocation after death bHasTeleported provides
	 * entry into ProcessRoomChange for overlap delegates */
	bHasTeleported = true;
	if (GetWorld()->GetNetMode() != NM_Client)
	{ SetActorLocationAndRotation(RespawnLocation,FRotator::ZeroRotator); }
}

FVector ASpyCharacter::GetSpyRespawnLocation()
{
	const ASpyVsSpyGameMode* SpyGameMode = GetWorld()->GetAuthGameMode<ASpyVsSpyGameMode>();
	if (!IsValid(SpyGameMode))
	{ return FVector::ZeroVector; }
	
	/** Get a collection of unoccupied rooms from room manager */
	ARoomManager* RoomManager = SpyGameMode->GetRoomManager();
	TArray<FRoomListing> RoomCandidatesForSpyRelocation;
	RoomManager->GetRoomListingCollection(RoomCandidatesForSpyRelocation, false);

	FVector SpyRelocationTarget = FVector::ZeroVector;

	/** Try and get a random room from the collection to use as location of respawn */
	if (RoomCandidatesForSpyRelocation.Num() > 0)
	{
		const uint8 RandomRangeMax = RoomCandidatesForSpyRelocation.Num() - 1;
		const int32 RandomIntFromRange = FMath::RandRange(0, RandomRangeMax);
		if (const ASVSRoom* CandidateRoom = RoomCandidatesForSpyRelocation[RandomIntFromRange].Room)
		{ SpyRelocationTarget = CandidateRoom->GetActorLocation(); }
		else
		{
			UE_LOG(SVSLog, Warning, TEXT(
				"%s Character: %s room respawn candidate was not valid"),
				IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
				*GetName());
		}
	}
	else
	{
		UE_LOG(SVSLog, Warning, TEXT(
			"%s Character: %s room respawn got zero room listings"),
			IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
			*GetName());
	}
	return SpyRelocationTarget;
}

void ASpyCharacter::RequestDeath()
{
	// TODO refactor this as it is redundant
	if (GetWorld()->GetNetMode() != NM_Client)
	{ S_RequestDeath(); }
}

void ASpyCharacter::S_RequestDeath_Implementation()
{

	if (SpyPlayerState->GetCurrentStatus() != EPlayerGameStatus::Playing)
	{ return; }

	/** Apply a time penalty to the player for dying */
	SpyPlayerState->SetPlayerRemainingMatchTime(0.0f, true);
	NM_SetEnableDeathState(true);
	NM_RequestDeath();
	if (!GetSpyPlayerState()->IsPlayerRemainingMatchTimeExpired())
	{
		// TODO could get rid of timer and respond to a notify
		GetWorld()->GetTimerManager().SetTimer(
				FinishDeathTimerHandle,
				this,
				&ThisClass::FinishDeath,
				FinishDeathDelaySeconds,
				false);
	}
}

void ASpyCharacter::NM_RequestDeath_Implementation()
{
	// TODO maybe make this server only
	if (SpyPlayerState->GetCurrentStatus() == EPlayerGameStatus::Finished ||
		SpyPlayerState->GetCurrentStatus() == EPlayerGameStatus::MatchTimeExpired)
	{
		SetSpyHidden(true);
		// TODO there should be a more elegant way to get them out of the way, collision, etc...
		SetActorLocation(FVector(0.0f, 0.0f, 2000.0f));
	}
	
	/** Only runs on Server */
	if (GetLocalRole() == ROLE_Authority && GetLocalRole() != ROLE_AutonomousProxy)
	{ RemoveCharacterAbilities(); } // TODO is this needed with cancelallabilities below?

	if (GetLocalRole() != ROLE_SimulatedProxy || (IsValid(SpyAbilitySystemComponent)))
	{
		/** Ability Component System related work */
		SpyAbilitySystemComponent->CancelAllAbilities();
		FGameplayTagContainer EffectTagsToRemove;
		EffectTagsToRemove.AddTag(EffectRemoveOnDeathTag);
		int32 NumEffectsRemoved = SpyAbilitySystemComponent->RemoveActiveEffectsWithTags(
			EffectTagsToRemove);
		// TODO see about moving this to enabledeathstate
		SpyAbilitySystemComponent->AddLooseGameplayTag(SpyStateDeadTag);
	}
}

void ASpyCharacter::FinishDeath()
{
	/** Expected to run on Server */
	// TODO Verify HasAuthority check is sufficient for multiplayer server/client architecture
	if (GetWorld()->GetNetMode() == NM_Client)
	{ return; }
	
	/** Reset health to max */
	SpyPlayerState->GetAttributeSet()->SetHealth(
		SpyPlayerState->GetAttributeSet()->GetMaxHealth());

	/** Removed dead state tag */
	SpyAbilitySystemComponent->RemoveLooseGameplayTag(SpyStateDeadTag);
	
	/** Remove held weapon/trap */
	if (IsValid(PlayerInventoryComponent))
	{ PlayerInventoryComponent->ResetEquipped(); }

	/** Reset Character death state settings */
	if (SpyPlayerState->GetCurrentStatus() == EPlayerGameStatus::Playing)
	{ NM_SetEnableDeathState(false, GetSpyRespawnLocation()); }
}

void ASpyCharacter::SetAttackActive(const bool bEnabled) const
{
	GetPlayerInventoryComponent()->EnableWeaponAttackPhase(bEnabled);
	
	// TODO can we just tick hand bone or perhaps use replicated movement on the attack component?
	/** Required for using component collisions off of animation montages
	 * otherwise location of component is not replicated.
	 * This might conflict with ragdolling on simulated proxy */
	GetMesh()->VisibilityBasedAnimTickOption = bEnabled ?
		EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones :
		EVisibilityBasedAnimTickOption::AlwaysTickPose;
}

void ASpyCharacter::HandlePrimaryAttackHit(const FHitResult& HitResult)
{
// TODO might need to check on task count on simulated if we exclude them here
	if (IsRunningClientOnly() || GetLocalRole() == ROLE_SimulatedProxy)
	{ return; }
	
	/** check if we have a valid attack hit and target is not dead */
	ACharacter* HitCharacter = Cast<ACharacter>(HitResult.GetActor());
	if (!bAttackHitFound ||
		IsValid(HitCharacter) &&
		HitCharacter->ActorHasTag(CombatantTag) &&
		UKismetSystemLibrary::DoesImplementInterface(HitCharacter, USpyCombatantInterface::StaticClass()) &&
		UKismetSystemLibrary::DoesImplementInterface(HitCharacter, UAbilitySystemInterface::StaticClass()) &&
		!Cast<IAbilitySystemInterface>(HitCharacter)->
			GetAbilitySystemComponent()->
				HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("State.Dead")))
	{
		/** prevent additional runs if more hits occur during same ability run */
		bAttackHitFound = true;
		SetAttackActive(false);

		/** Payload setup to send to GameplayEvent to Actor */
		const FGameplayTag ResultTag = FGameplayTag::RequestGameplayTag("Attack.Hit");
		FGameplayEventData GameplayEventData = FGameplayEventData();
		GameplayEventData.Instigator = this;
		GameplayEventData.Target = HitCharacter;
		GameplayEventData.TargetData = new FGameplayAbilityTargetData_SingleTargetHit(HitResult); 

		GetAbilitySystemComponent()->HandleGameplayEvent(ResultTag, &GameplayEventData);
	}
	else
	{
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag("Attack.NoHit");
		FGameplayEventData Payload = FGameplayEventData();
		Payload.Instigator = this;
		Payload.TargetData = FGameplayAbilityTargetDataHandle();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetInstigator(), Tag, Payload);
	}
}

void ASpyCharacter::PlayAttackAnimation(UAnimMontage* AttackMontage, const float TimerValue)
{
	if (IsRunningDedicatedServer())
	{ NM_PlayAttackAnimation(AttackMontage, TimerValue); }
}

void ASpyCharacter::NM_PlayAttackAnimation_Implementation(UAnimMontage* AttackMontage, const float TimerValue)
{
	ResetAttackHitFound();
	const bool bMontagePlayedSuccessfully = GetMesh()->GetAnimInstance()->Montage_Play(AttackMontage, 1.3f) > 0;
	if (!AttackMontageEndedDelegate.IsBound())
	{ AttackMontageEndedDelegate.BindUObject(this, &ThisClass::OnAttackMontageEnded); }
	GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(AttackMontageEndedDelegate, AttackMontage);
}

void ASpyCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{

}

void ASpyCharacter::ApplyAttackImpactForce_Implementation(const FVector FromLocation, const FVector ImpactForce) const
{
	// TODO this will not get called as the caller is client side only
	// https://github.com/james65535/GASDocumentation?tab=readme-ov-file#456-granted-abilities
	if (GetLocalRole() != ROLE_Authority)
	{ return; }
	
	NM_ApplyAttackImpactForce(FromLocation, ImpactForce);
}

void ASpyCharacter::NM_ApplyAttackImpactForce_Implementation(const FVector FromLocation, const FVector ImpactForce) const
{
	const FVector TargetLocation = GetActorLocation();
	const FVector Direction = UKismetMathLibrary::GetDirectionUnitVector(FromLocation, TargetLocation);
	const FVector AdjustedLaunchVector = FVector(
			Direction.X * ImpactForce.X,
			Direction.Y * ImpactForce.Y,
			abs(Direction.Z + 1) * ImpactForce.Z); 
	
	UE_LOG(SVSLogDebug, Log, TEXT("%s Chracter: %s launched after attack - X: %f Y: %f Z: %f"),
		IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
		*GetName(),
		AdjustedLaunchVector.X,
		AdjustedLaunchVector.Y,
		AdjustedLaunchVector.Z);
	
	GetCharacterMovement()->Launch(AdjustedLaunchVector); // TODO was: abs(Direction.Z + 1) * InAttackForce)
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
	if (GetLocalRole() == ROLE_SimulatedProxy || !IsValid(GetAbilitySystemComponent()))
	{ return; }

	S_RequestPrimaryAttack();
}

void ASpyCharacter::S_RequestPrimaryAttack_Implementation()
{
	SpyAbilitySystemComponent->TryActivateAbility(
		SpyAbilitySystemComponent->FindAbilitySpecFromInputID(
			static_cast<int32>(ESpyAbilityInputID::PrimaryAttackAction))->Handle);
}

void ASpyCharacter::InitializeEquippedItem()
{
	S_RequestEquipItem(EItemRotationDirection::Initial);
}

void ASpyCharacter::RequestEquipNextInventoryItem(const FInputActionValue& Value)
{
	if (GetLocalRole() != ROLE_AutonomousProxy)
	{ return; }

	S_RequestEquipItem(EItemRotationDirection::Next);
}

void ASpyCharacter::RequestEquipPreviousInventoryItem(const FInputActionValue& Value)
{
	if (GetLocalRole() != ROLE_AutonomousProxy)
	{ return; }
	
	S_RequestEquipItem(EItemRotationDirection::Previous);
}

void ASpyCharacter::S_RequestEquipItem_Implementation(const EItemRotationDirection InItemRotationDirection)
{
	if (!IsValid(GetPlayerInventoryComponent()))
	{ return; }

	PlayerInventoryComponent->EquipInventoryItem(InItemRotationDirection);
}

void ASpyCharacter::RequestInteract()
{
	if (GetLocalRole() == ROLE_SimulatedProxy ||
		!IsValid(GetInteractionComponent()) ||
		!GetInteractionComponent()->CanInteract())
	{ return; }
	
	S_RequestInteract();
}

void ASpyCharacter::UpdateCharacterTeam(const EPlayerTeam InPlayerTeam)
{
	switch (InPlayerTeam)
	{
		case EPlayerTeam::None : {
			UE_LOG(SVSLog, Log, TEXT("%s SpyCharacter: %s updated team NONE!"),
				IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
				*GetName());
			break;}

		case EPlayerTeam::TeamA : {
				/** Cosmetic Change */
				if (!IsRunningDedicatedServer())
				{
					GetMesh()->SetMaterial(0, CoatTeamAMaterialInstance); // Coat
					GetMesh()->SetMaterial(2, TorsoTeamAMaterialInstance); // Vest
					GetMesh()->SetMaterial(3, LegsTeamAMaterialInstance); // Trousers
					GetMesh()->SetMaterial(4, GlovesTeamAMaterialInstance); // Gloves
					HatMeshComponent->SetMaterial(0, HatTeamAMaterialInstance);
				}
				
				SpyMeshCollisionProfile = "SpyCharacterMeshA";
				break;}

		case EPlayerTeam::TeamB : {
				/** Cosmetic Change */
				if (!IsRunningDedicatedServer())
				{
					GetMesh()->SetMaterial(0, CoatTeamBMaterialInstance); // Coat
					GetMesh()->SetMaterial(2, TorsoTeamBMaterialInstance); // Vest
					GetMesh()->SetMaterial(3, LegsTeamBMaterialInstance); // Trousers
					GetMesh()->SetMaterial(4, GlovesTeamBMaterialInstance); // Gloves
					HatMeshComponent->SetMaterial(0, HatTeamBMaterialInstance);
				}

				SpyMeshCollisionProfile = "SpyCharacterMeshB";
				break;}
	}

	/** This is a hack to be able to use blocking hits on the attack zone without self trigger */
	GetMesh()->SetCollisionProfileName(SpyMeshCollisionProfile, true);
}

void ASpyCharacter::EquippedItemUpdated()
{
	if (ASpyPlayerController* PlayerController = GetController<ASpyPlayerController>())
	{ PlayerController->C_DisplayCharacterInventory(); }
}

void ASpyCharacter::S_RequestInteract_Implementation()
{
	SpyAbilitySystemComponent->TryActivateAbility(
		SpyAbilitySystemComponent->FindAbilitySpecFromInputID(
			static_cast<int32>(ESpyAbilityInputID::InteractAction))->Handle);
}
