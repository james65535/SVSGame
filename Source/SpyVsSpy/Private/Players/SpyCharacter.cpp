// Copyright Epic Games, Inc. All Rights Reserved.

#include "Players/SpyCharacter.h"

#include "Players/IsoCameraActor.h"
#include "Players/IsoCameraComponent.h"
#include "Players/SpyPlayerController.h"
#include "Players/SpyInteractionComponent.h"
#include "Players/SpyCombatantInterface.h"
#include "Players/SpyPlayerState.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "SVSLogger.h"
#include "AbilitySystem/SpyGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "AbilitySystem/SpyAttributeSet.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Abilities/GameplayAbility.h"
#include "GameModes/SpyVsSpyGameMode.h"
#include "Items/InventoryComponent.h"
#include "Items/InteractInterface.h"
#include "Items/InventoryWeaponAsset.h"
#include "Items/InventoryTrapAsset.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Rooms/RoomManager.h"
#include "Rooms/SVSRoom.h"
#include "Rooms/SpyFurniture.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
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

	GetMesh()->SetCollisionProfileName("SpyCharacterMesh", true);
	GetMesh()->SetGenerateOverlapEvents(true); // TODO check into perf

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
	AttackZone->SetupAttachment(GetMesh(),"hand_rSocket");
	AttackZone->SetVisibility(false);
	AttackZone->SetIsReplicated(true);
	AttackZone->SetSphereRadius(20.0f);
	AttackZone->SetCollisionProfileName("CombatantPreset");
	/** Preset has Collision disabled - Query is enabled when attacking */
	AttackZone->SetCollisionObjectType(ECC_GameTraceChannel2);
	
	WeaponMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("WeaponMeshComponent");
	if (IsValid(WeaponMeshComponent))
	{
		WeaponMeshComponent->SetupAttachment(GetMesh(), FName("hand_rSocket"));
		WeaponMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		/** Visibility is enabled when the character has a weapon with a valid mesh in hand */
		WeaponMeshComponent->SetVisibility(false);
	}

	HatMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("HatMeshComponent");
	if (IsValid(HatMeshComponent))
	{
		HatMeshComponent->SetupAttachment(GetMesh(), FName("Hat_Socket"));
		HatMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
}

void ASpyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams GeneralSharedParams;
	GeneralSharedParams.bIsPushBased = true;
	GeneralSharedParams.RepNotifyCondition = REPNOTIFY_OnChanged;
	DOREPLIFETIME_WITH_PARAMS_FAST(ASpyCharacter, ActiveWeaponInventoryIndex, GeneralSharedParams);
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

	// TODO test
	bHasTeleported = true;
	
	/** Add delegates for Character Overlaps */
	OnActorBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnOverlapBegin);
	OnActorEndOverlap.AddUniqueDynamic(this, &ThisClass::OnOverlapEnd);
	
	AttackZone->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnAttackComponentBeginOverlap);
	AttackZone->OnComponentEndOverlap.AddUniqueDynamic(this, &ThisClass::OnAttackComponentEndOverlap);
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
			UE_LOG(SVSLogDebug, Log, TEXT("%s Character %s overlapped room %s at start of game"),
				IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
				*GetName(),
				*RoomEntering->GetName());
			
			/** Reset flag and process room change */
			bHasTeleported = false;
			ProcessRoomChange(RoomEntering);
		}
	}

	/** Setup team and mesh colours */
	// TODO setup a more dynamic option
	// SpyTeam = IsLocallyControlled();
	// if (SpyTeam == 0)
	// {
	// 	GetMesh()->SetMaterialByName(FName("M_torso"), TorsoTeamAMaterialInstance);
	// 	GetMesh()->SetMaterialByName(FName("M_HeadLegs"), LegsTeamAMaterialInstance);
	// 	HatMeshComponent->SetMaterial(0, HatTeamAMaterialInstance);
	// }
	// else if (SpyTeam == 1)
	// {
	// 	GetMesh()->SetMaterialByName(FName("M_torso"), TorsoTeamBMaterialInstance);
	// 	GetMesh()->SetMaterialByName(FName("M_HeadLegs"), LegsTeamBMaterialInstance);
	// 	HatMeshComponent->SetMaterial(0, HatTeamBMaterialInstance);
	// }
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

void ASpyCharacter::OnAttackComponentBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == this)
	{ return; }

	UE_LOG(SVSLogDebug, Log, TEXT("%s Character: %s has begun OnAttackComponentOverlap with other actor: %s"),
		IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
		*GetName(),
		*OtherActor->GetName());

	bAttackHitFound = true;
	HandlePrimaryAttackOverlap(OtherActor);

	// TODO move back to ability system component notify or use this for hitting any other object
	// UNiagaraFunctionLibrary::SpawnSystemAtLocation(this,
	// 	AttackImpactSpark,
	// 	AttackZone->GetComponentLocation(),
	// 	FRotator::ZeroRotator,
	// 	FVector::OneVector,
	// 	true,
	// 	true,
	// 	ENCPoolMethod::None,
	// 	true);
}

void ASpyCharacter::OnAttackComponentEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == this)
	{ return; }

	if (const ASpyCharacter* OpponentSpyCharacter = Cast<ASpyCharacter>(OtherActor))
	{
		if (OpponentSpyCharacter != this)
		{ UE_LOG(SVSLogDebug, Log, TEXT("%s Character: %s ended OnAttackComponentOverlap with %s attacker: %s"),
			IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
			*GetName(),
			OpponentSpyCharacter->IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
			*OpponentSpyCharacter->GetName()); }
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
}

void ASpyCharacter::PrimaryAttackWindowStarted()
{
	if (GetWorld()->GetNetMode() == NM_Client)
	{ return; }

	SetEnabledAttackState(true);
}

void ASpyCharacter::PrimaryAttackWindowCompleted()
{
	if (GetWorld()->GetNetMode() == NM_Client)
	{ return; }

	/** Notify ability of zero overlaps */
	if (!bAttackHitFound)
	{
		SetEnabledAttackState(false);
		HandlePrimaryAttackOverlap(nullptr);
	}
	bAttackHitFound = false;
	
	UE_LOG(SVSLog, Warning, TEXT("%s Character: %s PrimaryAttackWindowCompleted called"),
		IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
		*GetName());
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

void ASpyCharacter::SetWeaponMesh(UStaticMesh* InMesh)
{
	WeaponMeshComponent->SetStaticMesh(InMesh);
}

void ASpyCharacter::NM_FinishedMatch_Implementation()
{
	if (ASpyPlayerController* SpyPlayerController = Cast<ASpyPlayerController>(GetController()))
	{
		SpyPlayerController->FinishedMatch();
		GetCharacterMovement()->DisableMovement();
	}
	
	SetSpyHidden(true);
	
	// TODO refactor to remove server rpc
	/** Just run this on the server */
	if (GetWorld()->GetNetMode() != NM_Client)
	{ S_RequestDeath(); }
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
	} else
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
				static_cast<int32>(ESVSAbilityInputID::TrapTriggerAction))
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


void ASpyCharacter::NM_SetEnableDeathState_Implementation(const bool bEnabled, const FVector RespawnLocation)
{
	SetEnableDeathState(bEnabled, RespawnLocation);
}

void ASpyCharacter::SetEnableDeathState(const bool bEnabled, const FVector RespawnLocation)
{
	if (IsValid(GetAbilitySystemComponent()))
	{
		FString TagsToPrintMessage = "";
		FGameplayTagContainer TagsToPrintContainer;
		GetAbilitySystemComponent()->GetOwnedGameplayTags(TagsToPrintContainer);
		for (FGameplayTag TagToPrint : TagsToPrintContainer)
		{ TagsToPrintMessage.Appendf(TEXT("%s "), *TagToPrint.GetTagName().ToString()); }

		UE_LOG(SVSLog, Warning, TEXT(">>>>>>>>>>>%s Character: %s setenabledeathstate: %s with tags: %s"),
			IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
			*GetName(),
			bEnabled ? *FString("True") : *FString("False"),
			*TagsToPrintMessage);
	}
	else
	{
		UE_LOG(SVSLog, Warning, TEXT(
			">>>>>>>>>>>%s Character: %s setenabledeathstate: %s could not print tags as no valid asc"),
			IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
			*GetName(),
			bEnabled ? *FString("True") : *FString("False"));
	}
	// TODO are these replicated as currently this func is called via NetMulticast
	if (bEnabled)
	{
		//GetCharacterMovement()->SetIsReplicated(false); // TODO this may not be set by default
		GetMesh()->SetCollisionProfileName("Ragdoll", true);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		GetMesh()->SetAllBodiesSimulatePhysics(true);
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->WakeAllRigidBodies();
		return;
	}
	
	//GetCharacterMovement()->SetIsReplicated(true);
	GetMesh()->SetAllBodiesSimulatePhysics(false);
	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->PutAllRigidBodiesToSleep();
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GetMesh()->SetCollisionProfileName("SpyCharacterMesh", true);
	GetMesh()->AttachToComponent(GetCapsuleComponent(),
		FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true));
	GetMesh()->SetRelativeLocationAndRotation(
		FVector(0.0f, 0.0f, -90.0f),
		FRotator(0.0f, 270.0f, 0.0f));

	/** Process character relocation after death bHasTeleported provides
	 * entry into ProcessRoomChange for overlap delegates */
	bHasTeleported = true;
	if (GetWorld()->GetNetMode() != NM_Client)
	{
		SetActorLocationAndRotation(RespawnLocation,FRotator::ZeroRotator);	
	}
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
		if (ASVSRoom* CandidateRoom = RoomCandidatesForSpyRelocation[RandomIntFromRange].Room)
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
	// else
	// {
	// 	if(!Destroy())
	// 	{
	// 		UE_LOG(SVSLog, Warning, TEXT(
	// 			"%s Character %s was not able to be destroyed when requested"),
	// 			IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
	// 			*GetName());
	// 	}
	// 	//SetSpyHidden(true);
	// }
}

void ASpyCharacter::NM_RequestDeath_Implementation()
{
	// TODO maybe make this server only
	UE_LOG(SVSLog, Warning, TEXT("%s Character: %s has requested death, may they be at peace"),
		IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
		*GetName());

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
		// OnCharacterDied.Broadcast(this); // TODO borrowing for finishdeath

		/** Ability Component System related work */
		SpyAbilitySystemComponent->CancelAllAbilities();
		FGameplayTagContainer EffectTagsToRemove;
		EffectTagsToRemove.AddTag(EffectRemoveOnDeathTag);
		int32 NumEffectsRemoved = SpyAbilitySystemComponent->RemoveActiveEffectsWithTags(
			EffectTagsToRemove);
		// TODO see about moving this to enabledeathstate
		SpyAbilitySystemComponent->AddLooseGameplayTag(SpyDeadTag);
	}
}

void ASpyCharacter::FinishDeath()
{
	UE_LOG(SVSLog, Warning, TEXT("%s Character: %s is finishing their death process - RIP"),
		IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
		*GetName());
	
	// This is called from Server RPC
	// TODO Verify HasAuthority check is sufficient for multiplayer server/client architecture
	if (GetWorld()->GetNetMode() == NM_Client)
	{ return; }
	
	/** Reset health to max */
	SpyPlayerState->GetAttributeSet()->SetHealth(SpyPlayerState->GetAttributeSet()->GetMaxHealth());

	/** Removed dead state tag */
	SpyAbilitySystemComponent->RemoveLooseGameplayTag(SpyDeadTag);

	if (SpyPlayerState->GetCurrentStatus() == EPlayerGameStatus::Playing)
	{
		/** Reset Character death state settings */
		NM_SetEnableDeathState(false, GetSpyRespawnLocation());
	}
}

void ASpyCharacter::HandlePrimaryAttackOverlap(AActor* OverlappedSpyCombatant)
{
	HandlePrimaryAttackAbility(OverlappedSpyCombatant);
}

void ASpyCharacter::SetEnabledAttackState(const bool bEnabled) const
{
	UE_LOG(SVSLogDebug, Log, TEXT("%s Character: %s SetEnableAttack: %s"),
		IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
		*GetName(),
		bEnabled ? *FString("True") : *FString("False"));
	
	if (bEnabled)
	{
		AttackZone->ShapeColor = FColor::Red;
		AttackZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		// TODO can we just tick hand bone or perhaps use replicated movement on the attack component?
		/** Required for using collisions off of animation montages
		 * This might conflict with ragdolling on simulated proxy */
		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

		return;
	}
	
	AttackZone->ShapeColor = FColor::Blue;
	AttackZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
}

void ASpyCharacter::HandlePrimaryAttackAbility(AActor* OverlappedSpyCombatant)
{
	// TODO might need to check on task count on simulated if we exclude them here
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{ return; }

	/** check if we have a valid attack hit */
	if (IsValid(OverlappedSpyCombatant) &&
		OverlappedSpyCombatant != this &&
		OverlappedSpyCombatant->ActorHasTag(CombatantTag) &&
		UKismetSystemLibrary::DoesImplementInterface(OverlappedSpyCombatant, USpyCombatantInterface::StaticClass()) &&
		UKismetSystemLibrary::DoesImplementInterface(OverlappedSpyCombatant, UAbilitySystemInterface::StaticClass()))
	{
		/** prevent additional runs if overlaps occur during same ability run */
		bAttackHitFound = true;
// TODO is this the right place?
		SetEnabledAttackState(false);
		// UE_LOG(SVSLogDebug, Log, TEXT(
		// 	"Handling Attack check - %s ActorHasTag: %s Implements SpyCombat: %s Implements UbilitySystem: %s"),
		// 	*OverlappedSpyCombatant->GetName(),
		// 	OverlappedSpyCombatant->ActorHasTag(CombatantTag) ? *FString("True") : *FString("False"),
		// 	UKismetSystemLibrary::DoesImplementInterface(
		// 		OverlappedSpyCombatant, USpyCombatantInterface::StaticClass()) ? *FString("True") : *FString("False"),
		// 	UKismetSystemLibrary::DoesImplementInterface(
		// 		OverlappedSpyCombatant, UAbilitySystemInterface::StaticClass()) ? *FString("True") : *FString("False"));
		
		/** Don't process attack if enemy is dead */
		if (Cast<IAbilitySystemInterface>(OverlappedSpyCombatant)->
			GetAbilitySystemComponent()->
			HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("State.Dead")))
		{ UE_LOG(SVSLogDebug, Log, TEXT("Found IsDead")); } // TODO Revisit this to remove or buildout
		
		// TODO sort out attack force and consider multiplayer aspect/multicast
		constexpr float AttackForce = 750.0f;
		const FVector AttackForceVector = FVector(AttackForce, AttackForce, 1.0f);
		Cast<ISpyCombatantInterface>(OverlappedSpyCombatant)->ApplyAttackImpactForce(GetActorLocation(), AttackForceVector);

		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag("Attack.Hit");
		FGameplayEventData Payload = FGameplayEventData();
		Payload.Instigator = GetInstigator();
		Payload.Target = OverlappedSpyCombatant;
		Payload.TargetData = UAbilitySystemBlueprintLibrary::AbilityTargetDataFromActor(OverlappedSpyCombatant);
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetInstigator(), Tag, Payload);
	}
	else
	{
		UE_LOG(SVSLogDebug, Log, TEXT(
			"%s Spy: %s Attack hit zero combatants"),
			this->IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
			*this->GetName());
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag("Attack.NoHit");
		FGameplayEventData Payload = FGameplayEventData();
		Payload.Instigator = IsValid(GetInstigator()) ? GetInstigator() : this;
		Payload.TargetData = FGameplayAbilityTargetDataHandle();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetInstigator(), Tag, Payload);
	}
}

void ASpyCharacter::PlayAttackAnimation(const float TimerValue)
{
	if(IsRunningDedicatedServer())
	{ NM_PlayAttackAnimation(TimerValue); }
}

void ASpyCharacter::NM_PlayAttackAnimation_Implementation(const float TimerValue)
{
	// TODO Remove after testing
	//SetEnabledAttackState(true);
	
	const bool bMontagePlayedSuccessfully = GetMesh()->GetAnimInstance()->Montage_Play(AttackMontage, 1.3f) > 0;
	if (!AttackMontageEndedDelegate.IsBound())
	{ AttackMontageEndedDelegate.BindUObject(this, &ThisClass::OnAttackMontageEnded); }
	GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(AttackMontageEndedDelegate, AttackMontage);
}

void ASpyCharacter::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(SVSLogDebug, Log, TEXT(
		"%s Character: %s received on attack montage ended and collision enabled: %s"),
		IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
		*GetName(),
		AttackZone->IsCollisionEnabled() ? *FString("True") : *FString("False"));

// TODO remove after testing
	/** Notify ability of zero overlaps */
	// if (!bAttackHitFound)
	// {
	// 	SetEnabledAttackState(false);
	// 	HandlePrimaryAttackOverlap(nullptr);
	// }
}

void ASpyCharacter::HandleTrapTrigger()
{
	// TODO might need to check on task count on simulated if we exclude them here
	if (GetLocalRole() == ROLE_SimulatedProxy)
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
	
	UE_LOG(SVSLogDebug, Log, TEXT("Handling Trap Trigger %s Victim: %s and %s Victim: %s"),
		IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
		*GetName(),
		GetInstigator()->IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
		*GetInstigator()->GetName());
	
	// TODO sort out attack force and consider multiplayer aspect/multicast
	// perhaps move this to gameplaycue and utilise magnitude...
	constexpr float AttackForce = 700.0f;
	const FVector AttackForceVector = FVector(
		AttackForce,
		AttackForce,
		1.0f);
	FVector AttackOrigin = FVector::ZeroVector;

	if (GetInteractionComponent()->CanInteractWithKnownInteractionInterface())
	{
		AttackOrigin = GetInteractionComponent()->
			GetLatestInteractableComponent()->
				Execute_GetInteractableOwner(
					GetInteractionComponent()->
						GetLatestInteractableComponent().GetObjectRef())->
							GetActorLocation();
	}

	Cast<ISpyCombatantInterface>(this)->
	ApplyAttackImpactForce(AttackOrigin, AttackForceVector);

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
}

void ASpyCharacter::NM_PlayTrapTriggerAnimation_Implementation(const float TimerValue)
{
	if (IsRunningDedicatedServer())
	{ return; }
	
	UE_LOG(SVSLogDebug, Log, TEXT("%s Character: %s Triggered Play Trap Trigger Anim Montage"),
		IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
		*GetName());
	PlayAnimMontage(AttackMontage); // TODO update
}

void ASpyCharacter::ApplyAttackImpactForce(const FVector FromLocation, const FVector InAttackForce) const
{
	if (GetLocalRole() != ROLE_Authority)
	{ return; }
	
	NM_ApplyAttackImpactForce(FromLocation, InAttackForce);
}

void ASpyCharacter::NM_ApplyAttackImpactForce_Implementation(const FVector FromLocation,
	const FVector InAttackForce) const
{
	const FVector TargetLocation = GetActorLocation();
	const FVector Direction = UKismetMathLibrary::GetDirectionUnitVector(FromLocation, TargetLocation);
	const FVector AdjustedLaunchVector = FVector(
			Direction.X * InAttackForce.X,
			Direction.Y * InAttackForce.Y,
			abs(Direction.Z + 1) * InAttackForce.Z); 

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
	if (GetLocalRole() == ROLE_SimulatedProxy)
	{ return; }
	
	/** Start ability from client */
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
	if (GetLocalRole() != ROLE_AutonomousProxy)
	{ return; }

	S_RequestEquipWeapon(EItemRotationDirection::Next);
}

void ASpyCharacter::RequestPreviousTrap(const FInputActionValue& Value)
{
	if (GetLocalRole() != ROLE_AutonomousProxy)
	{ return; }
	
	S_RequestEquipWeapon(EItemRotationDirection::Previous);
}

void ASpyCharacter::S_RequestEquipWeapon_Implementation(const EItemRotationDirection InItemRotationDirection)
{
	// TODO replace with WeaponMesh = GetPlayerInventoryComponent()->SelectWeapon(bChoosePrevious);
	
	if (!IsValid(GetPlayerInventoryComponent()))
	{ return; }
	
	TArray<UInventoryBaseAsset*> InventoryAssets;
	GetPlayerInventoryComponent()->GetInventoryItems(InventoryAssets);
	int StartIndex = ActiveWeaponInventoryIndex;
	if (InventoryAssets.Num() < 1)
	{ return; }

	if (InItemRotationDirection == EItemRotationDirection::Next)
	{
		StartIndex = ActiveWeaponInventoryIndex + 1;
		if (StartIndex < InventoryAssets.Num() && StartIndex > 0)
		{
			for (; StartIndex < InventoryAssets.Num();StartIndex++)
			{
				UInventoryWeaponAsset* WeaponAsset = Cast<UInventoryWeaponAsset>(InventoryAssets[StartIndex]);
				UInventoryWeaponAsset* OldWeaponAsset = Cast<UInventoryWeaponAsset>(InventoryAssets[StartIndex-1]);
				/** Quantities might be -1 which means unlimited */
				if (IsValid(WeaponAsset))
				{
					UE_LOG(SVSLogDebug, Log, TEXT("%s Character: %s potential trap: %s and old: %s"),
						IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
						*GetName(),
						*WeaponAsset->InventoryItemName.ToString(),
						*OldWeaponAsset->InventoryItemName.ToString());
					
					if (WeaponAsset->Quantity != 0)
					{
						CurrentHeldWeapon = WeaponAsset;
						SetWeaponMesh(WeaponAsset->Mesh);
					}
					else
					{
						CurrentHeldWeapon = nullptr;
						SetWeaponMesh(nullptr);
					}
				
					ActiveWeaponInventoryIndex = StartIndex;
					MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ActiveWeaponInventoryIndex, this);
					UE_LOG(SVSLogDebug, Log, TEXT("%s Character: %s Has requested next trap with new index: %i with valid mesh: %s and name: %s"),
						IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
						*GetName(),
						ActiveWeaponInventoryIndex,
						IsValid(WeaponMeshComponent->GetStaticMesh()) ? *FString("True") : *FString("False"),
						IsValid(CurrentHeldWeapon) ? *CurrentHeldWeapon->InventoryItemName.ToString() : *FString("null weapon"));
					return;
				}
			}
		}
	}
	else
	{
		if (ActiveWeaponInventoryIndex-1 >= 0)
		{ StartIndex = ActiveWeaponInventoryIndex-1; }

		for (; StartIndex >= 0;StartIndex--)
		{
			UInventoryWeaponAsset* WeaponAsset = Cast<UInventoryWeaponAsset>(InventoryAssets[StartIndex]);
			/** Quantities might be -1 which means unlimited */
			if (IsValid(WeaponAsset))
			{
				if (WeaponAsset->Quantity != 0)
				{ CurrentHeldWeapon = WeaponAsset; }
				else
				{ CurrentHeldWeapon = nullptr; }
				
				ActiveWeaponInventoryIndex = StartIndex;
				MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, ActiveWeaponInventoryIndex, this);
				SetWeaponMesh(WeaponAsset->Mesh);
				UE_LOG(SVSLogDebug, Log, TEXT("%s Character: %s Has requested previous trap with new index: %i with valid mesh: %s and name: %s"),
					IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
					*GetName(),
					ActiveWeaponInventoryIndex,
					IsValid(WeaponMeshComponent->GetStaticMesh()) ? *FString("True") : *FString("False"),
					IsValid(CurrentHeldWeapon) ? *CurrentHeldWeapon->InventoryItemName.ToString() : *FString("null weapon"));
				return;
			}
		}
	}
}

void ASpyCharacter::OnRep_ActiveWeaponInventoryIndex()
{
	if (!IsValid(GetPlayerInventoryComponent()))
	{ return; }

	// TODO add getting for singular asset so we don't have to use arrays here
	TArray<UInventoryBaseAsset*> InventoryAssets;
	GetPlayerInventoryComponent()->GetInventoryItems(InventoryAssets);
	if (InventoryAssets.Num() > 0 &&
		ActiveWeaponInventoryIndex >= 0 &&
		ActiveWeaponInventoryIndex < InventoryAssets.Num() &&
		IsValid(InventoryAssets[ActiveWeaponInventoryIndex]))
	{
		if (UInventoryWeaponAsset* FoundWeaponItem = Cast<UInventoryWeaponAsset>(
			InventoryAssets[ActiveWeaponInventoryIndex]))
		{
			if (FoundWeaponItem->Quantity != 0)
			{ CurrentHeldWeapon = FoundWeaponItem; }
			else
			{ CurrentHeldWeapon = nullptr; }
			
			SetWeaponMesh(FoundWeaponItem->Mesh);
			/** So that we do not see mesh wireframe for the null default weapon */
			WeaponMeshComponent->SetVisibility(
				(WeaponMeshComponent->GetStaticMesh() != nullptr));

			UE_LOG(LogTemp, Warning, TEXT(
				"Spy replicated weaponindex: %i heldweapon: %s and has valid weapon mesh: %s"),
				ActiveWeaponInventoryIndex,
				IsValid(CurrentHeldWeapon) ? *CurrentHeldWeapon->InventoryItemName.ToString() : *FString("null weapon"),
				IsValid(WeaponMeshComponent->GetStaticMesh()) ? *FString("True") : *FString("False"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT(
			"%s Spy: %s replicated weaponindex: %i with collection total %i"),
			IsLocallyControlled() ? *FString("Local") : *FString("Remote"),
			*GetName(),
			ActiveWeaponInventoryIndex,
			InventoryAssets.Num());
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

void ASpyCharacter::RequestInteract()
{
	// TODO refactor this more cleanly across character and controller
	if (!IsValid(GetInteractionComponent()) ||
		!GetInteractionComponent()->CanInteractWithKnownInteractionInterface())
	{ return; }
	
	TScriptInterface<IInteractInterface> InteractableActor = GetInteractionComponent()->RequestInteractWithObject();
	if (IsValid(InteractableActor.GetObjectRef()))
	{ S_RequestInteract(InteractableActor.GetObject()); }
}

void ASpyCharacter::S_RequestInteract_Implementation(UObject* InInteractableActor)
{
	const IInteractInterface* InteractableInterface = Cast<IInteractInterface>(InInteractableActor);
	ASpyPlayerController* SpyPlayerController = GetController<ASpyPlayerController>();
	if (!IsValid(SpyPlayerController) ||
		!IsValid(InteractableInterface->_getUObject()))
	{ return; }
	
	if (UInventoryTrapAsset* ActiveTrap = InteractableInterface->
		Execute_GetActiveTrap(InteractableInterface->_getUObject()))
	{
		// TODO move more of this server side for authorative info like trap damage etc..
		RequestTrapTrigger();
		InteractableInterface->
			Execute_RemoveActiveTrap(InteractableInterface->_getUObject(), ActiveTrap);
		return;
	}

	InteractableInterface->Execute_Interact(InteractableInterface->_getUObject(), this);

	if (InteractableInterface->Execute_HasInventory(InteractableInterface->_getUObject()))
	{
		SpyPlayerController->C_DisplayTargetInventory(
			InteractableInterface->Execute_GetInventory(InteractableInterface->_getUObject()));
	}
}

void ASpyCharacter::RequestTrapTrigger()
{
	C_RequestTrapTrigger();
}

void ASpyCharacter::C_RequestTrapTrigger_Implementation()
{
	SpyAbilitySystemComponent->AbilityLocalInputPressed(static_cast<int32>(ESVSAbilityInputID::TrapTriggerAction));
}
