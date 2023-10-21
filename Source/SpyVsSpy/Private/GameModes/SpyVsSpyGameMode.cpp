// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/SpyVsSpyGameMode.h"

#include "SVSLogger.h"
#include "GameModes/SpyVsSpyGameState.h"
#include "Players/SpyAIController.h"
#include "Players/SpyPlayerState.h"
#include "Players/SpyPlayerController.h"
#include "Rooms/RoomManager.h"
#include "UObject/ConstructorHelpers.h"

ASpyVsSpyGameMode::ASpyVsSpyGameMode()
{

}

void ASpyVsSpyGameMode::BeginPlay()
{
	Super::BeginPlay();

	/** If Game is in Start Menu then Player's HUD will take care of the rest
	 * Otherwise we let the HUD of each player controller know the class of Widget to Display
	 * for this Game Mode */
	if (ASpyVsSpyGameState* SVSGameState = GetGameState<ASpyVsSpyGameState>())
	{
		SVSGameState->SetGameType(DesiredGameType);

		// TODO replace with desiredgametype
		if (bToggleInitialMainMenu)
		{ SVSGameState->SetGameState(ESpyGameState::None); }
		else
		{ SVSGameState->SetGameState(ESpyGameState::Waiting); }
	}
}

void ASpyVsSpyGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);
}

void ASpyVsSpyGameMode::PlayerNotifyIsReady(ASpyPlayerState* InPlayerState)
{
	UE_LOG(SVSLogDebug, Log, TEXT("Game Mode was notified Player %s is ready"), *InPlayerState->GetPlayerName());
	if(CheckAllPlayersStatus(EPlayerGameStatus::Ready))
	{
		UE_LOG(SVSLogDebug, Log, TEXT("All players are ready, attempting to start game"));
		AttemptStartGame();
	}
}

void ASpyVsSpyGameMode::SetMatchTime(const float InMatchTime)
{
	if (ASpyVsSpyGameState* SVSGameState = GetGameState<ASpyVsSpyGameState>())
	{
		
	}
}

void ASpyVsSpyGameMode::RestartGame()
{
	Super::RestartGame();
}

void ASpyVsSpyGameMode::AttemptStartGame()
{
	/** Run Countdown timer to start of match */
	if (GameCountDownDuration > SMALL_NUMBER)
	{
		GetWorld()->GetTimerManager().SetTimer(
			DelayStartTimerHandle,
			this,
			&ThisClass::DisplayCountDown,
			DelayStartDuration,
			false);
	}
	else
	{ StartGame(); }
}

void ASpyVsSpyGameMode::StartGame()
{
	if (!HasAuthority())
	{
		UE_LOG(SVSLog, Warning, TEXT("This System does not have authority to start game"));
		return;
	}
	
	for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
	{
		if(ASpyPlayerController* PlayerController = Cast<ASpyPlayerController>(Iterator->Get()))
		{
			if (!MustSpectate(PlayerController))
			{
				ASpyPlayerState* PlayerState = PlayerController->GetPlayerState<ASpyPlayerState>();
				check(PlayerState);
				PlayerState->SetCurrentStatus(EPlayerGameStatus::Playing);
				PlayerState->SetIsWinner(false);
			}
		}
		
		if (const ASpyAIController* SpyAIController = Cast<ASpyAIController>(Iterator->Get()))
		{
			ASpyPlayerState* AIPlayerState = SpyAIController->GetPlayerState<ASpyPlayerState>();
			check(AIPlayerState)
			AIPlayerState->SetCurrentStatus(EPlayerGameStatus::Playing);
			AIPlayerState->SetIsWinner(false);
		}
	}
	ASpyVsSpyGameState* SpyGameState = GetGameState<ASpyVsSpyGameState>();
	check(SpyGameState);
	SpyGameState->NM_MatchStart();
}

void ASpyVsSpyGameMode::DisplayCountDown()
{
	/**
	 * Perform Spectator Check prior to calling Game Countdown to minimise calls between
	 * When Timer is started on server and when it is started on client
	 */
	TArray<ASpyPlayerController*> NonSpectatingPlayers;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if(ASpyPlayerController* PlayerController = Cast<ASpyPlayerController>(Iterator->Get()))
		{
			if (!MustSpectate(PlayerController))
			{ NonSpectatingPlayers.Emplace(PlayerController); }
		}
	}
	
	GetWorld()->GetTimerManager().SetTimer(
		CountdownTimerHandle,
		this,
		&ASpyVsSpyGameMode::StartGame,
		GameCountDownDuration,
		false);

	for (ASpyPlayerController* NonSpectatingPlayer : NonSpectatingPlayers)
	{ NonSpectatingPlayer->C_StartGameCountDown(GameCountDownDuration); }
}

bool ASpyVsSpyGameMode::CheckAllPlayersStatus(const EPlayerGameStatus StateToCheck) const
{
	uint32 Count = 0;
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		if (const ASpyPlayerState* PlayerState =
			Cast<ASpyPlayerController>(Iterator->Get())->GetPlayerState<ASpyPlayerState>())
		{
			if (PlayerState->GetCurrentState() != StateToCheck)
			{
				UE_LOG(SVSLog, Warning, TEXT("PlayerID: %i did not match requested state"), PlayerState->GetPlayerId())
				return false;
			}
			Count++;
		}
	}
	if (Count <= 0) { return false; }
	return true;
}

ARoomManager* ASpyVsSpyGameMode::LoadRoomManager()
{
	ARoomManager* RoomManager = Cast<ARoomManager>(GetWorld()->SpawnActor(ARoomManager::StaticClass()));
	return RoomManager;
}
