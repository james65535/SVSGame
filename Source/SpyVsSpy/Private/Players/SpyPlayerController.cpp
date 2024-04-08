// Fill out your copyright notice in the Description page of Project Settings.


#include "Players/SpyPlayerController.h"

#include "SVSLogger.h"
#include "AbilitySystem/SpyAttributeSet.h"
#include "EnhancedInput/Public/EnhancedInputComponent.h"
#include "EnhancedInput/Public/EnhancedInputSubsystems.h"
#include "EnhancedInput/Public/InputActionValue.h"
#include "Players/SpyHUD.h"
#include "Players/SpyPlayerState.h"
#include "Players/SpyCharacter.h"
#include "Players/SpyInteractionComponent.h"
#include "Players/PlayerInputConfigRegistry.h"
#include "Items/InventoryComponent.h"
#include "Items/InteractInterface.h"
#include "Items/InventoryWeaponAsset.h"
#include "Items/InventoryTrapAsset.h"
#include "UI/GameUIElementsRegistry.h"
#include "UI/UIElementAsset.h"
#include "GameModes/SpyVsSpyGameState.h"
#include "GameModes/SpyVsSpyGameMode.h"

void ASpyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	SpyGameState = GetWorld()->GetGameState<ASpyVsSpyGameState>();

	/** Player HUD related Tasks */
	if (!IsRunningDedicatedServer())
	{
		/** Specify HUD Representation at start of play */
		SpyPlayerHUD = Cast<ASpyHUD>(GetHUD());
		checkfSlow(SpyPlayerHUD, "SpyPlayerController HUD is a null pointer after cast")

		/** If Local game as listener or single player then Grab Gametype, otherwise use delegate to update on replication */
		UpdateHUDWithGameUIElements(SpyGameState->GetGameType());
		SpyGameState->OnGameTypeUpdateDelegate.AddUObject(this, &ThisClass::UpdateHUDWithGameUIElements);
		SpyGameState->OnServerLobbyUpdate.AddUObject(this, &ThisClass::OnServerLobbyUpdateDelegate);
	}

	/* Set Enhanced Input Mapping Context to Game Context */
	SetInputContext(GameInputMapping);

	/** Get Input Component as Enhanced Input Component */
	if (UEnhancedInputComponent* EIPlayerComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Enhanced Input Action Bindings
		// Character locomotion
		// EIPlayerComponent->BindAction(InputActions->InputMove,
		// 	ETriggerEvent::Triggered,
		// 	this,
		// 	&ThisClass::RequestMove);
  //
		// // Character Look
		// EIPlayerComponent->BindAction(InputActions->InputLook,
		// 	ETriggerEvent::Triggered,
		// 	this,
		// 	&ThisClass::RequestLook);
  //
		// // Character Jump
		// EIPlayerComponent->BindAction(InputActions->InputJump,
		// 	ETriggerEvent::Triggered,
		// 	this,
		// 	&ThisClass::RequestJump);
		//
		// EIPlayerComponent->BindAction(InputActions->InputJump,
		// 	ETriggerEvent::Completed,
		// 	this,
		// 	&ThisClass::RequestStopJump);
  //
		// // Character Crouch
		// EIPlayerComponent->BindAction(InputActions->InputCrouch,
  //           ETriggerEvent::Triggered,
  //           this,
  //           &ThisClass::RequestCrouch);
  //
		// EIPlayerComponent->BindAction(InputActions->InputCrouch,
		// 	ETriggerEvent::Completed,
		// 	this,
		// 	&ThisClass::RequestStopCrouch);
  //
		// // Character Sprint
		// EIPlayerComponent->BindAction(InputActions->InputSprint,
		// 	ETriggerEvent::Triggered,
		// 	this,
		// 	&ThisClass::RequestSprint);
  //
		// EIPlayerComponent->BindAction(InputActions->InputSprint,
		// 	ETriggerEvent::Completed,
		// 	this,
		// 	&ThisClass::RequestStopSprint);
  //
		// // Character Throw
		// EIPlayerComponent->BindAction(InputActions->InputThrowObject,
		// 	ETriggerEvent::Triggered,
		// 	this,
		// 	&ThisClass::RequestThrowObject);
  //
		// // Character Pull
		// EIPlayerComponent->BindAction(InputActions->InputPullObject,
		// 	ETriggerEvent::Triggered,
		// 	this,
		// 	&ThisClass::RequestHoldObject);
  //
		// EIPlayerComponent->BindAction(InputActions->InputPullObject,
		// 	ETriggerEvent::Completed,
		// 	this,
		// 	&ThisClass::RequestStopHoldObject);

		
		
		/** Jumping */
		//EIPlayerComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		//EIPlayerComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		/** Moving */
		EIPlayerComponent->BindAction(
			InputActions->MoveAction,
			ETriggerEvent::Triggered,
			this,
			&ThisClass::RequestMove);

		/** Looking */
		//EnhancedInputComponent->BindAction(
		//InputActions->LookAction,
		//ETriggerEvent::Triggered,
		//this,
		//&ThisClass::RequestLook);

		/** Sprinting */
		//EIPlayerComponent->BindAction(
		//InputActions->SprintAction,
		//ETriggerEvent::Completed,
		//this,
		//&ThisClass::RequestSprint);

		/** Primary Attack */
		EIPlayerComponent->BindAction(
			InputActions->PrimaryAttackAction,
			ETriggerEvent::Started,
			this,
			&ThisClass::RequestPrimaryAttack);

		/** Next Trap */
		EIPlayerComponent->BindAction(
			InputActions->NextTrapAction,
			ETriggerEvent::Started,
			this,
			&ThisClass::RequestNextTrap);

		/** Previous Trap */
		EIPlayerComponent->BindAction(
			InputActions->PrevTrapAction,
			ETriggerEvent::Started,
			this,
			&ThisClass::RequestPreviousTrap);
		
		/** Interacting */
		EIPlayerComponent->BindAction(
			InputActions->InteractAction,
			ETriggerEvent::Completed,
			this,
			&ThisClass::RequestInteract);

		/** Character Menu Display */
		EIPlayerComponent->BindAction(
			InputActions->InputOpenMenu,
			ETriggerEvent::Completed,
			this,
			&ThisClass::RequestDisplayLevelMenu);

		/** Character Menu Display Remove */
		EIPlayerComponent->BindAction(
			InputActions->InputCloseMenu,
			ETriggerEvent::Completed,
			this,
			&ThisClass::RequestHideLevelMenu);
	}

	/** Game Starts with a UI */
	C_RequestInputMode(EPlayerInputMode::UIOnly);

	/** Listen for match start announcements */
	SpyGameState->OnStartMatchDelegate.AddUObject(this, &ThisClass::StartMatchForPlayer);
}

void ASpyPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	
	if (!IsValid(SpyCharacter))
	{ SpyCharacter = Cast<ASpyCharacter>(GetCharacter()); }
}

void ASpyPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (IsRunningDedicatedServer())
	{ SpyCharacter = Cast<ASpyCharacter>(GetCharacter()); }
}

void ASpyPlayerController::OnRetrySelected()
{
	S_RestartLevel();
}

void ASpyPlayerController::OnReadySelected()
{
	S_OnReadySelected();
}

void ASpyPlayerController::FinishedMatch()
{
	if (!IsRunningDedicatedServer())
	{
		GetWorld()->GetTimerManager().ClearTimer(MatchClockDisplayTimerHandle);
		RequestInputMode(EPlayerInputMode::UIOnly);
		SpyPlayerHUD->DisplayResults(SpyGameState->GetResults());
		SpyPlayerHUD->ToggleDisplayGameTime(false);
	}
}

void ASpyPlayerController::RequestUpdatePlayerResults()
{
	if (!IsRunningDedicatedServer())
	{ SpyPlayerHUD->UpdateResults(SpyGameState->GetResults()); }
}

void ASpyPlayerController::S_RestartLevel_Implementation()
{
	ASpyVsSpyGameMode* SpyGameMode = GetWorld()->GetAuthGameMode<ASpyVsSpyGameMode>();
	if (ensureMsgf(SpyGameMode, TEXT("ASpyPlayerController::ServerRestartLevel_Implementation Invalid Game Mode")))
	{ SpyGameMode->RestartGame(); }
}

void ASpyPlayerController::ConnectToServer(const FString InServerAddress)
{
	if (InServerAddress.IsEmpty()) { return; }
	ClientTravel(InServerAddress, TRAVEL_Absolute, false);
}

void ASpyPlayerController::OnServerLobbyUpdateDelegate()
{
	SpyGameState = GetWorld()->GetGameState<ASpyVsSpyGameState>();
	if (!IsValid(SpyGameState) || !IsValid(SpyPlayerHUD))
	{ return; }

	TArray<FServerLobbyEntry> LobbyListings;
	SpyGameState->GetServerLobbyEntry(LobbyListings);
	SpyPlayerHUD->UpdateServerLobby(LobbyListings);
}

void ASpyPlayerController::SetPlayerName(const FString& InPlayerName)
{
	SetName(InPlayerName);
	checkfSlow(SpyPlayerState, "Controller tried to set player name but player state was null");
	SpyPlayerState->SavePlayerInfo();
}

void ASpyPlayerController::C_DisplayTargetInventory_Implementation(UInventoryComponent* TargetInventory)
{
	SpyPlayerHUD->DisplaySelectedActorInventory(TargetInventory);
	RequestInputMode(EPlayerInputMode::UIOnly);
}

void ASpyPlayerController::RequestInputMode(const EPlayerInputMode DesiredInputMode)
{
	C_RequestInputMode(DesiredInputMode);
}

void ASpyPlayerController::C_RequestInputMode_Implementation(const EPlayerInputMode DesiredInputMode)
{
	switch (DesiredInputMode)
	{
	case (EPlayerInputMode::GameOnly):
		{
			SetInputContext(GameInputMapping);
			const FInputModeGameOnly InputMode;
			SetInputMode(InputMode);
			SetShowMouseCursor(false); // TODO setting to true for this game or perhaps should select gameandui for whole game
			break;
		}
	case (EPlayerInputMode::GameAndUI):
		{
			const FInputModeGameAndUI InputMode;
			SetInputMode(InputMode);
			SetShowMouseCursor(true);
			break;
		}
	case (EPlayerInputMode::UIOnly):
		{
			SetInputContext(GameInputMapping);
			const FInputModeUIOnly InputMode;
			SetInputMode(InputMode);
			SetShowMouseCursor(true);
			break;
		}
	}
}

void ASpyPlayerController::C_DisplayCharacterInventory_Implementation()
{
	if (GetLocalRole() != ROLE_AutonomousProxy)
	{ return; }

	UE_LOG(SVSLogDebug, Log, TEXT("%s Character: %s called displaycharinventory"),
		IsLocalController() ? *FString("Local") : *FString("Not Local"),
		*SpyCharacter->GetName());
	SpyPlayerHUD->DisplayCharacterInventory(SpyCharacter->GetPlayerInventoryComponent());
}

void ASpyPlayerController::RequestTakeAllFromTargetInventory()
{
	SetInputContext(GameInputMapping);
	RequestInputMode(EPlayerInputMode::GameOnly);
	S_RequestTakeAllFromTargetInventory();
}

void ASpyPlayerController::S_RequestTakeAllFromTargetInventory_Implementation()
{
	// TODO refactor this more cleanly across controller and character
	if (!IsValid(SpyCharacter->GetInteractionComponent()) ||
		!SpyCharacter->GetInteractionComponent()->CanInteractWithKnownInteractionInterface() ||
		!IsValid(SpyCharacter->GetPlayerInventoryComponent()))
	{
		UE_LOG(SVSLog, Warning, TEXT(
			"Character %s tried to take items but validation failed - caninteract: %s inventory: %s"),
			*SpyCharacter->GetName(),
			SpyCharacter->GetInteractionComponent()->CanInteractWithKnownInteractionInterface() ? *FString("True") : *FString("False"),
			IsValid(SpyCharacter->GetPlayerInventoryComponent()) ? *FString("True") : *FString("False"));
		return;
	}

	/** Get the collection of PIDs from target inventory via the interaction component */
	TArray<FPrimaryAssetId> TargetInventoryPrimaryAssetIdCollection;
	SpyCharacter->
		GetInteractionComponent()->
		GetLatestInteractableComponent()->
		Execute_GetInventoryListing(
			SpyCharacter->GetInteractionComponent()->GetLatestInteractableComponent().GetObjectRef(),
			TargetInventoryPrimaryAssetIdCollection, FPrimaryAssetType(FName("InventoryMissionAsset")));
	
	if (TargetInventoryPrimaryAssetIdCollection.Num() < 1)
	{
		UE_LOG(SVSLog, Warning, TEXT(
			"Character %s tried to take items but inventory is empty"),
			*SpyCharacter->GetName());
		return;
	}

	/** sets the collection of primary asset ids to load which then replicates to clients for load procedures */
	SpyCharacter->GetPlayerInventoryComponent()->SetPrimaryAssetIdsToLoad(TargetInventoryPrimaryAssetIdCollection);
}

void ASpyPlayerController::RequestPlaceTrap()
{
	S_RequestPlaceTrap();
}

void ASpyPlayerController::S_RequestPlaceTrap_Implementation()
{
	UE_LOG(SVSLogDebug, Log,
		TEXT("%s Character: %s is attempting to place a trap in an interactable actor"),
		IsLocalController() ? *FString("Local") : *FString("Not Local"),
		*GetCharacter()->GetName());

	TScriptInterface<IInteractInterface> TargetInteractionComponent = SpyCharacter->GetInteractionComponent()->GetLatestInteractableComponent();

	if (!IsValid(TargetInteractionComponent.GetObjectRef()) ||
		!IsValid(SpyCharacter->GetHeldWeapon()) ||
		SpyCharacter->GetHeldWeapon()->WeaponType != EWeaponType::Trap)
	{
		UE_LOG(SVSLogDebug, Log,
			TEXT("Place trap invalid check - HasInterComp: %s HasInvComp %s Target Furniture %s HasWeapon %s WeaponNotTrap: %s"),
			IsValid(SpyCharacter->GetInteractionComponent()) ?
				*FString("True") : *FString("False"),
			IsValid(SpyCharacter->GetPlayerInventoryComponent()) ?
				*FString("True") : *FString("False"),
			IsValid(TargetInteractionComponent.GetObjectRef()) ?
				*FString("True") : *FString("False"), 
			IsValid(SpyCharacter->GetHeldWeapon()) ?
				*FString("True") : *FString("False"),
			IsValid(SpyCharacter->GetHeldWeapon()) ?
				*FString("True") : *FString("False"));
		return;
	}
	
	if (UInventoryTrapAsset* HeldTrap = Cast<UInventoryTrapAsset>(SpyCharacter->GetHeldWeapon()))
	{
		TargetInteractionComponent->Execute_GetInventory(
			TargetInteractionComponent.GetObjectRef())->
		SetActiveTrap(HeldTrap);
	}
}

void ASpyPlayerController::CalculateGameTimeElapsedSeconds()
{
	const float ElapsedTime = SpyGameState->GetSpyMatchElapsedTime();
//	const float TimeLeft = SpyPlayerState->GetPlayerRemainingMatchTime() - ElapsedTime;

	const float TimeLeft = SpyPlayerState->GetPlayerRemainingMatchTime() - ElapsedTime;
	
	/** Player ran out of time so notify game that their match has ended */
	if (TimeLeft <= 0.0f)
	{ GetWorld()->GetTimerManager().ClearTimer(MatchClockDisplayTimerHandle); }

	if (!IsRunningDedicatedServer())
	{ HUDDisplayGameTimeElapsedSeconds(TimeLeft); }
}

void ASpyPlayerController::HUDDisplayGameTimeElapsedSeconds(const float InTimeToDisplay) const
{
	SpyPlayerHUD->SetMatchTimerSeconds(InTimeToDisplay);
}

void ASpyPlayerController::SetInputContext(TSoftObjectPtr<UInputMappingContext> InMappingContext)
{
	if(const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
	{
		UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		if(const UInputMappingContext* InMappingContextLoaded = InMappingContext.LoadSynchronous())
		{
			if (!InputSystem->HasMappingContext(InMappingContextLoaded))
			{
				InputSystem->ClearAllMappings();
				InputSystem->AddMappingContext(InMappingContextLoaded, 0, FModifyContextOptions());
			}
		}
	}
}

void ASpyPlayerController::UpdateHUDWithGameUIElements(const ESVSGameType InGameType) const
{
	checkfSlow(GameElementsRegistry, "PlayerController: Verify Controller Blueprint has a UI Elements registry set");
	if (InGameType == ESVSGameType::None) { return; }
	
	SpyPlayerHUD->SetGameUIAssets(GameElementsRegistry->GameTypeUIMapping.Find(InGameType)->LoadSynchronous());
}

void ASpyPlayerController::RequestDisplayLevelMenu()
{
	SpyPlayerHUD->DisplayLevelMenu();
	RequestInputMode(EPlayerInputMode::UIOnly);
}

void ASpyPlayerController::RequestHideLevelMenu()
{
	SpyPlayerHUD->HideLevelMenu();
	RequestInputMode(EPlayerInputMode::GameOnly);
}

void ASpyPlayerController::StartMatchForPlayer(const float InMatchStartTime)
{
	RequestInputMode(EPlayerInputMode::GameOnly);
	LocalClientCachedMatchStartTime = InMatchStartTime - GetWorld()->DeltaTimeSeconds;

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		UE_LOG(LogTemp, Warning, TEXT("running match start as automous proxy"));
		/** Handle Match Time - Most likely on 1 second repeat */
		GetWorld()->GetTimerManager().SetTimer(
			MatchClockDisplayTimerHandle,
			this,
			&ThisClass::CalculateGameTimeElapsedSeconds,
			MatchClockDisplayRateSeconds,
			true);
	}
	
	/** Update Player Displays with character info */
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		SpyPlayerHUD->ToggleDisplayGameTime(true);
		SpyPlayerHUD->DisplayCharacterHealth(
			SpyPlayerState->GetAttributeSet()->GetHealth(),
			SpyPlayerState->GetAttributeSet()->GetMaxHealth());
		C_DisplayCharacterInventory();
	}
}

bool ASpyPlayerController::CanProcessRequest() const
{
	if (SpyGameState && SpyGameState->IsMatchInPlay())
	{ return (SpyPlayerState->GetCurrentStatus() == EPlayerGameStatus::Playing); }
	return false;
}

void ASpyPlayerController::RequestMove(const FInputActionValue& ActionValue)
{
	if (!CanProcessRequest() || !IsValid(SpyCharacter))
	{ return; }
	
	/** input is a Vector2D */
	const FVector2D MovementVector = ActionValue.Get<FVector2D>();
	
	/** find out which way is forward */
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	/** get forward vector */
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

	/** get right vector */
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	/** add movement */
	SpyCharacter->AddMovementInput(ForwardDirection, MovementVector.Y);
	SpyCharacter->AddMovementInput(RightDirection, MovementVector.X);
}

void ASpyPlayerController::RequestNextTrap(const FInputActionValue& ActionValue)
{
	UE_LOG(SVSLogDebug, Log, TEXT("Controller Next trap"));
	if (!CanProcessRequest() || !IsValid(SpyCharacter))
	{ return; }

	SpyCharacter->RequestNextTrap(ActionValue);
}

void ASpyPlayerController::RequestPreviousTrap(const FInputActionValue& ActionValue)
{
	UE_LOG(SVSLogDebug, Log, TEXT("Controller Prev trap"));
	if (!CanProcessRequest() || !IsValid(SpyCharacter))
	{ return; }

	SpyCharacter->RequestPreviousTrap(ActionValue);
}

void ASpyPlayerController::RequestInteract(const FInputActionValue& ActionValue)
{
	if (!CanProcessRequest() || !IsValid(SpyCharacter))
	{ return; }

	SpyCharacter->RequestInteract();
}

void ASpyPlayerController::RequestPrimaryAttack(const FInputActionValue& ActionValue)
{
	if (!CanProcessRequest() || !IsValid(SpyCharacter))
	{ return; }

	SpyCharacter->RequestPrimaryAttack(ActionValue);
}

void ASpyPlayerController::S_OnReadySelected_Implementation()
{
	if (GetWorld()->GetNetMode() != NM_Client)
	{
		checkfSlow(SpyPlayerState, "Player Controller attempted to access Spy Player State to set ready but it was null");
		SpyPlayerState->SetCurrentStatus(EPlayerGameStatus::Ready);
	}
}

void ASpyPlayerController::C_ResetPlayer_Implementation()
{
	check(SpyPlayerHUD);
	SpyPlayerHUD->RemoveResults();
	SpyPlayerHUD->ToggleDisplayGameTime(false);
	UpdateHUDWithGameUIElements(SpyGameState->GetGameType());
}

void ASpyPlayerController::C_StartGameCountDown_Implementation(const float InCountDownDuration)
{
	SpyPlayerHUD->DisplayMatchStartCountDownTime(InCountDownDuration);
}
