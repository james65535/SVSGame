// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameModes/SpyVsSpyGameState.h"
#include "Players/SpyPlayerState.h"
#include "GameFramework/GameMode.h"
#include "SpyVsSpyGameMode.generated.h"

class ARoomManager;
class ASpyPlayerController;

UCLASS(minimalapi)
class ASpyVsSpyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ASpyVsSpyGameMode();
	virtual void BeginPlay() override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void RestartGame() override; // TODO review to see if controller resets are needed

	ARoomManager* LoadRoomManager();

	/** Check if Game is In Start Menu */
	UFUNCTION(BlueprintPure, Category = "SVS|GameMode")
	bool IsInStartMenu() const { return bToggleInitialMainMenu; }

	/** Method for Player's to notify they are ready to play */
	UFUNCTION(BlueprintCallable, Category = "SVS|GameMode")
	void PlayerNotifyIsReady(ASpyPlayerState* InPlayerState );

	/** Set the player match starting time - not the game match time */
	UFUNCTION(BlueprintCallable, Category = "SVS|GameMode")
	void SetMatchTime(const float InMatchTime) const;

private:

	// TODO this can be dropped in favour of DesiredGameType
	/** Is this game mode used for the initial main menu screen */
	UPROPERTY(EditDefaultsOnly, Category = "SVS|GameMode")
	bool bToggleInitialMainMenu = true;

	/** Determine what type of game this mode should communicate to game state and downstream clients */
	UPROPERTY(EditDefaultsOnly, NoClear, Category = "SVS|GameMode")
	ESVSGameType DesiredGameType = ESVSGameType::None;

	/** Used to offset start to avoid race conditions as game loads up */
	float DelayStartDuration = 0.5f;
	FTimerHandle DelayStartTimerHandle;

	/** Game Mode will poll for all players ready and if not then try again in a specified time */
	float MatchTryStartWaitDuration = 2.0f;
	FTimerHandle MatchTryStartTimerHandle;
	
	/** Countdown before the gameplay state begins.  Exposed for BPs to change in editor */
	UPROPERTY(EditAnywhere, Category = "SVS|GameMode")
	int32 GameCountDownDuration = 1;
	FTimerHandle CountdownTimerHandle;
	
	/** Allows Game Mode to determine if a single or multiplayer game is intended */
	UPROPERTY(EditAnywhere, Category = "SVS|GameMode")
	int32 NumExpectedPlayers = 1;
	/** Setter to specify if a single or multiplayer game is intended */
	UFUNCTION(BlueprintCallable, Category = "SVS|GameMode")
	void SetNumExpectedPlayers(const int32 InNumExpectedPlayers) { NumExpectedPlayers = InNumExpectedPlayers; }
	
	void AttemptStartGame();
	void DisplayCountDown();
	void StartGame();

	bool CheckAllPlayersStatus(const EPlayerGameStatus StateToCheck) const;
};



