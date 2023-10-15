// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SpyVsSpyGameState.generated.h"

class ASpyCharacter;
class ARoomManager;

// ENUM to track the current state of the game
UENUM(BlueprintType)
enum class ESpyGameState : uint8
{
	None		UMETA(DisplayName = "None"),
	Waiting		UMETA(DisplayName = "Waiting"),
	Playing		UMETA(DisplayName = "Playing"),
	Paused		UMETA(DisplayName = "Paused"),
	GameOver	UMETA(DisplayName = "GameOver"),
};

// ENUM to track the current type of game
UENUM(BlueprintType)
enum class ESVSGameType : uint8
{
	None		UMETA(DisplayName = "None"),
	Start		UMETA(DisplayName = "Start"),
	Versus		UMETA(DisplayName = "Versus"),
	TBD		UMETA(DisplayName = "TBD"),
};

USTRUCT(BlueprintType)
struct FGameResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FString Name = "Null";

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	float Time = 0.0f;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	bool bIsWinner = false;
};

/**
 * Track state related to online multiplayer versus game mode
 */

/** Begin Delegates */

/** Notify listeners such as player controller that game type has changed */
DECLARE_MULTICAST_DELEGATE_OneParam(FGameTypeUpdateDelegate, ESVSGameType);
/** Notify listeners match has started with the match start time */
DECLARE_MULTICAST_DELEGATE_OneParam(FStartMatch, const float);

UCLASS()
class SPYVSSPY_API ASpyVsSpyGameState : public AGameState
{
	GENERATED_BODY()

public:
	ASpyVsSpyGameState();
	
	ARoomManager* GetRoomManager() const;
	
	/** Class Overrides */
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "SVS")
	void SetGameState(const ESpyGameState InGameState);
	UFUNCTION(BlueprintCallable, Category = "SVS")
	ESpyGameState GetGameState() const { return SpyGameState; };
	/** Quick Check to Determine if Game State is Playing */
	UFUNCTION(BlueprintPure)
	bool IsGameInPlay() const { return SpyGameState == ESpyGameState::Playing;}

	/** Game Type Public Accessors */
	/** Set the Game Type - Should Correspond with GameMode */
	UFUNCTION(BlueprintCallable, Category = "SVS")
	void SetGameType(const ESVSGameType InGameType);
	/** Get the Game Type - Should Correspond with GameMode */
	UFUNCTION(BlueprintCallable, Category = "SVS")
	ESVSGameType GetGameType() const { return SVSGameType;}
	
	UFUNCTION(BlueprintCallable, Category = "Tantrumn")
	const TArray<FGameResult>& GetResults() { return Results; }
	UFUNCTION()
	void ClearResults();
	
	/** Methods relating to Match Start and Time Management */
	UFUNCTION(NetMulticast, Reliable, Category = "SVS")
	void NM_MatchStart();
	FStartMatch OnStartMatchDelegate;
	UFUNCTION(BlueprintCallable, Category = "SVS")
	float GetMatchStartTime() const { return MatchStartTime; }
	UFUNCTION(BlueprintCallable, Category = "SVS")
	float GetMatchDeltaTime() const { return GetServerWorldTimeSeconds() - MatchStartTime; }
	
	FGameTypeUpdateDelegate OnGameTypeUpdateDelegate;

private:
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, meta = (AllowPrivateAccess), ReplicatedUsing="OnRep_RoomManager", Category = "SVS|Room")
	ARoomManager* RoomManager;

	UFUNCTION()
	void OnRep_RoomManager();

	/** The State of the Game */
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_GameState, Category = "SVS")
	ESpyGameState SpyGameState = ESpyGameState::None;
	UPROPERTY()
	ESpyGameState OldSpyGameState = ESpyGameState::None;
	UFUNCTION()
	void OnRep_GameState() const;

	/** The type of game being played - is correlated to which gamemode is selected */
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_GameType, Category = "SVS")
	ESVSGameType SVSGameType = ESVSGameType::None;
	UFUNCTION()
	void OnRep_GameType() const;

	/** Game Results */
	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_ResultsUpdated, Category = "SVS")
	TArray<FGameResult> Results;
	UFUNCTION()
	void OnRep_ResultsUpdated();
	/** Can be called during and after play */
	void PlayerRequestSubmitResults(const ASpyCharacter* InSpyCharacter);
	/** Check if all results are in then let clients know the final results */
	void TryFinaliseScoreBoard();
	bool CheckAllResultsIn() const ;

	/** Game Time Values */
	UPROPERTY(VisibleAnywhere, Category = "SVS")
	float CountDownStartTime;
	/** Server - Client Time Sync handled by Player Controllers */ 
	UPROPERTY(VisibleAnywhere, Category = "SVS")
	float MatchStartTime;
	
};
