// Fill out your copyright notice in the Description page of Project Settings.


#include "Players/SpyPlayerController.h"

//#include "EnhancedInputSubsystems.h"
#include "EnhancedInput/Public/EnhancedInputComponent.h"
#include "EnhancedInput/Public/EnhancedInputSubsystems.h"
#include "EnhancedInput/Public/InputActionValue.h"
#include "Players/SpyHUD.h"
#include "UI/GameUIElementsRegistry.h"
#include "Players/SpyPlayerState.h"
#include "GameModes/SpyVsSpyGameState.h"
#include "GameModes/SpyVsSpyGameMode.h"

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

void ASpyPlayerController::UpdateHUDWithGameUIElements(ESVSGameType InGameType)
{
	checkfSlow(GameElementsRegistry, "PlayerController: Verify Controller Blueprint has a UI Elements registry set");
	if (InGameType == ESVSGameType::None) { return; }
	
	PlayerHUD->SetGameUIAssets(GameElementsRegistry->GameTypeUIMapping.Find(InGameType)->LoadSynchronous());
}

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

		/** If Local game as listener or single player then Grab Gametype, otherwise use delegate to update on replication */
		UpdateHUDWithGameUIElements(SpyGameState->GetGameType());
		SpyGameState->OnGameTypeUpdateDelegate.AddUObject(this, &ThisClass::UpdateHUDWithGameUIElements);
	}
}

void ASpyPlayerController::S_OnReadySelected_Implementation()
{
	if (GetWorld()->GetAuthGameMode<ASpyVsSpyGameMode>())
	{
		checkfSlow(TantrumnPlayerState, "Player Controller attempted to access tantrumn player state to set ready but it was null");
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
