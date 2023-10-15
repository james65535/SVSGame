// Fill out your copyright notice in the Description page of Project Settings.


#include "Players/SpyPlayerController.h"

//#include "EnhancedInputSubsystems.h"
#include "SVSLogger.h"
#include "EnhancedInput/Public/EnhancedInputComponent.h"
#include "EnhancedInput/Public/EnhancedInputSubsystems.h"
#include "EnhancedInput/Public/InputActionValue.h"
#include "Players/SpyHUD.h"
#include "Players/SpyPlayerState.h"
#include "Players/SpyCharacter.h"
#include "UI/GameUIElementsRegistry.h"
#include "UI/UIElementAsset.h"
#include "GameModes/SpyVsSpyGameState.h"
#include "GameModes/SpyVsSpyGameMode.h"
#include "Players/PlayerInputConfigRegistry.h"

void ASpyPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	SpyGameState = GetWorld()->GetGameState<ASpyVsSpyGameState>();
	check(SpyGameState);

	/** Player HUD related Tasks */
	if (!IsRunningDedicatedServer())
	{
		/** Specify HUD Representation at start of play */
		PlayerHUD = Cast<ASpyHUD>(GetHUD());
		checkfSlow(PlayerHUD, "SpyPlayerController HUD is a null pointer after cast")

		/** If Local game as listener or single player then Grab Gametype, otherwise use delegate to update on replication */
		UpdateHUDWithGameUIElements(SpyGameState->GetGameType());
		SpyGameState->OnGameTypeUpdateDelegate.AddUObject(this, &ThisClass::UpdateHUDWithGameUIElements);
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
			ETriggerEvent::Completed,
			this,
			&ThisClass::RequestNextTrap);

		/** Previous Trap */
		EIPlayerComponent->BindAction(
			InputActions->PrevTrapAction,
			ETriggerEvent::Completed,
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
	NM_SetControllerGameInputMode(EPlayerInputMode::UIOnly);

	/** Listen for match start announcements */
	SpyGameState->OnStartMatchDelegate.AddUObject(this, &ThisClass::StartMatchForPlayer);
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
		SpyPlayerState->SetCurrentStatus(EPlayerGameStatus::Finished);
		SetInputContext(MenuInputMapping);
		NM_SetControllerGameInputMode(EPlayerInputMode::UIOnly);
		PlayerHUD->DisplayResults(SpyGameState->GetResults());
		PlayerHUD->ToggleDisplayGameTime(false);
	}
}

void ASpyPlayerController::RequestDisplayFinalResults() const
{
	if (!IsRunningDedicatedServer())
	{
		PlayerHUD->DisplayResults(SpyGameState->GetResults());
	}
}

void ASpyPlayerController::S_RestartLevel_Implementation()
{
	ASpyVsSpyGameMode* SpyGameMode = GetWorld()->GetAuthGameMode<ASpyVsSpyGameMode>();
	if (ensureMsgf(SpyGameMode, TEXT("ASpyPlayerController::ServerRestartLevel_Implementation Invalid Game Mode")))
	{
		SpyGameMode->RestartGame();
	}
}

void ASpyPlayerController::ConnectToServer(const FString InServerAddress)
{
	if (InServerAddress.IsEmpty()) { return; }
	ClientTravel(InServerAddress, TRAVEL_Absolute, false);
}

void ASpyPlayerController::NM_SetControllerGameInputMode_Implementation(const EPlayerInputMode InRequestedInputMode)
{
	switch (InRequestedInputMode)
	{
	case (EPlayerInputMode::GameOnly):
		{
			SetInputContext(GameInputMapping);
			const FInputModeGameOnly InputMode;
			SetInputMode(InputMode);
			SetShowMouseCursor(false);
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
			const FInputModeUIOnly InputMode;
			SetInputMode(InputMode);
			SetShowMouseCursor(true);
			break;
		}
	}
}

void ASpyPlayerController::SetPlayerName(const FString& InPlayerName)
{
	SetName(InPlayerName);
	checkfSlow(SpyPlayerState, "Controller tried to set player name but player state was null");
	SpyPlayerState->SavePlayerInfo();
}

void ASpyPlayerController::HUDDisplayGameTimeElapsedSeconds() const
{
	PlayerHUD->SetMatchTimerSeconds(SpyGameState->GetServerWorldTimeSeconds() - CachedMatchStartTime);
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
	
	PlayerHUD->SetGameUIAssets(GameElementsRegistry->GameTypeUIMapping.Find(InGameType)->LoadSynchronous());
}

void ASpyPlayerController::RequestDisplayLevelMenu()
{
	if (CanProcessRequest())
	{
		SetInputContext(MenuInputMapping);
		PlayerHUD->DisplayLevelMenu();
		NM_SetControllerGameInputMode(EPlayerInputMode::GameAndUI);
	}
}

void ASpyPlayerController::RequestHideLevelMenu()
{
	if (CanProcessRequest())
	{
		SetInputContext(GameInputMapping);
		PlayerHUD->HideLevelMenu();
		NM_SetControllerGameInputMode(EPlayerInputMode::GameOnly);
	}
}

void ASpyPlayerController::StartMatchForPlayer(const float InMatchStartTime)
{
	NM_SetControllerGameInputMode(EPlayerInputMode::GameOnly);
	SpyPlayerState->SetCurrentStatus(EPlayerGameStatus::Playing);
	CachedMatchStartTime = InMatchStartTime - GetWorld()->DeltaTimeSeconds;
	if (!IsRunningDedicatedServer())
	{
		GetWorld()->GetTimerManager().SetTimer(
			MatchClockDisplayTimerHandle,
			this,
			&ThisClass::HUDDisplayGameTimeElapsedSeconds,
			MatchClockDisplayRateSeconds,
			true);
		PlayerHUD->ToggleDisplayGameTime(true);
	}
}

bool ASpyPlayerController::CanProcessRequest() const
{
	if (SpyGameState && SpyGameState->IsGameInPlay())
	{ return (SpyPlayerState->GetCurrentState() == EPlayerGameStatus::Playing); }
	return false;
}

void ASpyPlayerController::RequestMove(const FInputActionValue& ActionValue)
{
}

void ASpyPlayerController::RequestNextTrap(const FInputActionValue& ActionValue)
{
}

void ASpyPlayerController::RequestPreviousTrap(const FInputActionValue& ActionValue)
{
}

void ASpyPlayerController::RequestInteract(const FInputActionValue& ActionValue)
{
}

void ASpyPlayerController::RequestPrimaryAttack(const FInputActionValue& ActionValue)
{
}

void ASpyPlayerController::S_OnReadySelected_Implementation()
{
	if (GetWorld()->GetAuthGameMode<ASpyVsSpyGameMode>())
	{
		checkfSlow(SpyPlayerState, "Player Controller attempted to access Spy Player State to set ready but it was null");
		SpyPlayerState->SetCurrentStatus(EPlayerGameStatus::Ready);
	}
}

void ASpyPlayerController::C_ResetPlayer_Implementation()
{
	check(PlayerHUD);
	PlayerHUD->RemoveResults();
	PlayerHUD->ToggleDisplayGameTime(false);
	UpdateHUDWithGameUIElements(SpyGameState->GetGameType());
}

void ASpyPlayerController::C_StartGameCountDown_Implementation(const float InCountDownDuration)
{
	PlayerHUD->DisplayMatchStartCountDownTime(InCountDownDuration);
}
