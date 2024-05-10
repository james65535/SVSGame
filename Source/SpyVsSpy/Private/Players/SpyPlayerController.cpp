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
		/** Moving */
		EIPlayerComponent->BindAction(
			InputActions->MoveAction,
			ETriggerEvent::Triggered,
			this,
			&ThisClass::RequestMove);

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

		/** Close Inventory Menu */
		EIPlayerComponent->BindAction(
			InputActions->CloseInventoryAction,
			ETriggerEvent::Completed,
			this,
			&ThisClass::RequestCloseInventory);

		/** Character Menu Display */
		EIPlayerComponent->BindAction(
			InputActions->InputOpenMenuAction,
			ETriggerEvent::Completed,
			this,
			&ThisClass::RequestDisplayLevelMenu);

		/** Character Menu Display Remove */
		EIPlayerComponent->BindAction(
			InputActions->InputCloseMenuAction,
			ETriggerEvent::Completed,
			this,
			&ThisClass::RequestCloseLevelMenu);
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

void ASpyPlayerController::EndMatch()
{
	SpyCharacter->DisableSpyCharacter();
	if (!IsRunningDedicatedServer())
	{
		GetWorld()->GetTimerManager().ClearTimer(MatchClockDisplayTimerHandle);
		RequestInputMode(EPlayerInputMode::UIOnly);
		SpyPlayerHUD->DisplayResults(SpyGameState->GetResults());
		SpyPlayerHUD->ToggleDisplayGameTime(false);
	}
}

void ASpyPlayerController::RequestUpdatePlayerResults() const
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
	RequestInputMode(EPlayerInputMode::GameAndUI);
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
			SetInputContext(MenuInputMapping);
			const FInputModeGameAndUI InputMode;
			SetInputMode(InputMode);
			SetShowMouseCursor(true);
			break;
		}
	case (EPlayerInputMode::UIOnly):
		{
			SetInputContext(MenuInputMapping);
			const FInputModeUIOnly InputMode;
			SetInputMode(InputMode);
			SetShowMouseCursor(true);
			break;
		}
	}
}

void ASpyPlayerController::C_DisplayCharacterInventory_Implementation()
{
	if (GetLocalRole() != ROLE_AutonomousProxy || !IsValid(SpyCharacter))
	{ return; }

	/** Find out what items the Spy has in inventory */
	TArray<UInventoryBaseAsset*> InventoryAssets;
	SpyCharacter->GetPlayerInventoryComponent()->GetInventoryItems(InventoryAssets);
	
	/** Grab name of held weapon so we can flag up to UI what is selected */
	const FString HeldWeaponName =  IsValid(SpyCharacter->GetHeldWeapon()) ?
		SpyCharacter->GetHeldWeapon()->GetName() :
		"NotAName";
	
	TMap<UObject*, bool> DisplayedItems;
	for (UObject* InventoryAsset : InventoryAssets)
	{
		const bool bSelectedWeapon = InventoryAsset->GetName().Equals(HeldWeaponName) ?
			true :
			false;
		
		DisplayedItems.Add(InventoryAsset, bSelectedWeapon);
	}
	
	SpyPlayerHUD->DisplayCharacterInventory(DisplayedItems);
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
		!SpyCharacter->GetInteractionComponent()->CanInteract() ||
		!IsValid(SpyCharacter->GetPlayerInventoryComponent()))
	{
		UE_LOG(SVSLog, Warning, TEXT(
			"Character %s tried to take items but validation failed - caninteract: %s inventory: %s"),
			*SpyCharacter->GetName(),
			SpyCharacter->GetInteractionComponent()->CanInteract() ? *FString("True") : *FString("False"),
			IsValid(SpyCharacter->GetPlayerInventoryComponent()) ? *FString("True") : *FString("False"));
		return;
	}

	/** Get the collection of PIDs from target inventory via the interaction component */
	TArray<FPrimaryAssetId> TargetInventoryPrimaryAssetIdCollection;
	SpyCharacter->
		GetInteractionComponent()->GetLatestInteractableComponent()->Execute_GetInventoryListing(
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

bool ASpyPlayerController::RequestPlaceTrap() const
{
	TScriptInterface<IInteractInterface> TargetInteractionComponent = SpyCharacter->GetInteractionComponent()->GetLatestInteractableComponent();

	if (!IsValid(TargetInteractionComponent.GetObjectRef()) ||
		!IsValid(SpyCharacter->GetHeldWeapon()) ||
		IsRunningClientOnly())
	{ return false; }

	// TODO determine if refactor into controlled character's inventory component is required
	if (UInventoryTrapAsset* HeldTrap = Cast<UInventoryTrapAsset>(SpyCharacter->GetHeldWeapon()))
	{
		const bool bTrapSetSuccessful = TargetInteractionComponent->Execute_SetActiveTrap(TargetInteractionComponent.GetObjectRef(), HeldTrap);

		UE_LOG(SVSLogDebug, Warning, TEXT("Character %s was able to set trap on %s with success %s"),
			*SpyCharacter->GetName(),
			*TargetInteractionComponent->Execute_GetInteractableOwner(TargetInteractionComponent.GetObjectRef())->GetName(),
			bTrapSetSuccessful ? *FString("True") : *FString("False"));
		return bTrapSetSuccessful;
	}
	return false;
}

void ASpyPlayerController::CalculateGameTimeElapsedSeconds()
{
	const float ElapsedTime = SpyGameState->GetSpyMatchElapsedTime();
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

void ASpyPlayerController::StartMatchForPlayer(const float InMatchStartTime)
{
	RequestInputMode(EPlayerInputMode::GameOnly);
	LocalClientCachedMatchStartTime = InMatchStartTime - GetWorld()->DeltaTimeSeconds;

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		/** Handle Match Time - Most likely on 1 second repeat */
		GetWorld()->GetTimerManager().SetTimer(
			MatchClockDisplayTimerHandle,
			this,
			&ThisClass::CalculateGameTimeElapsedSeconds,
			MatchClockDisplayRateSeconds,
			true);
	
		/** Update Player Displays with character info */
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

void ASpyPlayerController::SetInputContext(const TSoftObjectPtr<UInputMappingContext> InMappingContext) const
{
	if(const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
	{
		UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		if(const UInputMappingContext* InMappingContextLoaded = InMappingContext.LoadSynchronous())
		{
			InputSystem->ClearAllMappings();
			InputSystem->AddMappingContext(InMappingContextLoaded, 0, FModifyContextOptions());
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
	RequestInputMode(EPlayerInputMode::GameAndUI);
}

void ASpyPlayerController::RequestCloseLevelMenu()
{
	SpyPlayerHUD->CloseLevelMenu();
	RequestInputMode(EPlayerInputMode::GameOnly);
}

void ASpyPlayerController::RequestCloseInventory()
{
	SpyPlayerHUD->CloseInventory();
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
	if (!CanProcessRequest() || !IsValid(SpyCharacter))
	{ return; }

	SpyCharacter->RequestNextTrap(ActionValue);
}

void ASpyPlayerController::RequestPreviousTrap(const FInputActionValue& ActionValue)
{
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
