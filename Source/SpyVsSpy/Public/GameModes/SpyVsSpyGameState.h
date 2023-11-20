// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "Players/SpyPlayerState.h"
#include "SpyVsSpyGameState.generated.h"

class ASpyPlayerState;
enum class EPlayerGameStatus : uint8;
class UInventoryBaseAsset;
class ASpyCharacter;
class ARoomManager;

// ENUM to track the current state of the game
UENUM(BlueprintType)
enum class ESpyMatchState : uint8
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

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	bool bCompletedMission = false;
};


USTRUCT(BlueprintType)
struct FServerLobbyEntry
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FString SpyName = "Null";

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	EPlayerGameStatus SpyPlayerStatus;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	float Ping;

	// TODO for multi-match score keeping
	// UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	// uint8 Score;

	FServerLobbyEntry()
	{
		SpyName = "Null";
		SpyPlayerStatus = EPlayerGameStatus::None;
		Ping = 0.0f;
	}

	FServerLobbyEntry(FString InSpyName, EPlayerGameStatus InSpyPlayerStatus, float InPing)
	{
		SpyName = InSpyName;
		SpyPlayerStatus = InSpyPlayerStatus;
		Ping = InPing;
	}
};

/**
 * Track state related to online multiplayer versus game mode
 */

/** Begin Delegates */

/** Notify listeners such as player controller that game type has changed */
DECLARE_MULTICAST_DELEGATE_OneParam(FGameTypeUpdateDelegate, ESVSGameType);
/** Notify listeners match has started with the match start time */
DECLARE_MULTICAST_DELEGATE_OneParam(FStartMatch, const float);
/** Notify listeners server player lobby has updated */
DECLARE_MULTICAST_DELEGATE(FServerLobbyUpdate);

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
	/** Add PlayerState to the PlayerArray */
	virtual void AddPlayerState(APlayerState* PlayerState) override;

	UFUNCTION(BlueprintCallable, Category = "SVS|GameState")
	void SetSpyMatchState(const ESpyMatchState InSpyMatchState);
	UFUNCTION(BlueprintCallable, Category = "SVS|GameState")
	ESpyMatchState GetSpyMatchState() const { return SpyMatchState; }
	/** Quick Check to Determine if Game State is Playing */
	UFUNCTION(BlueprintPure)
	bool IsMatchInPlay() const { return SpyMatchState == ESpyMatchState::Playing;}

	/** Game Type Public Accessors */
	/** Set the Game Type - Should Correspond with GameMode */
	UFUNCTION(BlueprintCallable, Category = "SVS|GameState")
	void SetGameType(const ESVSGameType InGameType);
	/** Get the Game Type - Should Correspond with GameMode */
	UFUNCTION(BlueprintCallable, Category = "SVS|GameState")
	ESVSGameType GetGameType() const { return SVSGameType;}

	/** Can be called during and after play */
	UFUNCTION(BlueprintCallable, Category = "SVS|GameState")
	void RequestSubmitMatchResult(ASpyPlayerState* InSpyPlayerState, bool bPlayerTimeExpired = false);
	UFUNCTION(BlueprintCallable, Category = "SVS|GameState")
	const TArray<FGameResult>& GetResults() { return Results; }
	UFUNCTION()
	void ClearResults();
	
	/** Methods relating to Match Start and Time Management */
	UFUNCTION(BlueprintCallable, Category = "SVS|GameState")
	void SetSpyMatchTimeLength(const float InSecondsTotal);
	UFUNCTION(NetMulticast, Reliable, Category = "SVS|GameState")
	void NM_MatchStart();
	FStartMatch OnStartMatchDelegate;
	UFUNCTION(BlueprintCallable, Category = "SVS|GameState")
	float GetSpyMatchStartTime() const { return SpyMatchStartTime; }
	UFUNCTION(BlueprintCallable, Category = "SVS|GameState")
	float GetSpyMatchElapsedTime() const { return GetServerWorldTimeSeconds() - SpyMatchStartTime; }
	
	void SetRequiredMissionItems(const TArray<UInventoryBaseAsset*>& InRequiredMissionItems);
	void GetRequiredMissionItems(TArray<UInventoryBaseAsset*>& RequestedRequiredMissionItems);
	
	FGameTypeUpdateDelegate OnGameTypeUpdateDelegate;

	UFUNCTION(BlueprintCallable, Category = "SVS|GameState")
	void SetAllPlayerGameStatus(const EPlayerGameStatus InPlayerGameStatus);

	/** manage listing of players and relevant info used by lobby UI element */
	UFUNCTION(BlueprintCallable, Category = "SVS|GameState")
	void SetServerLobbyEntry(const FString InPLayerName,
	const FUniqueNetIdRepl& PlayerStateUniqueId, EPlayerGameStatus SpyPLayerCurrentStatus, float Ping, const bool bRemoveEntry = false);
	UFUNCTION(BlueprintCallable, Category = "SVS|GameState")
	void GetServerLobbyEntry(TArray<FServerLobbyEntry>& LobbyListings);
	FServerLobbyUpdate OnServerLobbyUpdate;
protected:

	/** Item array with which a player must fully possess to complete the map */
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (AllowPrivateAccess), Category = "SVS|GameState")
	TArray<UInventoryBaseAsset*> RequiredMissionItems;

	/** Starting Match Time - Independent from game match time */
	UFUNCTION(BlueprintCallable, Category = "SVS|GameState")
	void SetSpyMatchStartTime(const float InMatchStartTime);

	/** manage listing of players and relevant info used by lobby UI element */
	TMap<const FUniqueNetIdRepl, FServerLobbyEntry> ServerLobbyEntries;

	/** Game Time Values */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, meta = (AllowPrivateAccess), Category = "SVS|GameState")
	float SpyMatchTimeLength = 0.0f;
	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, meta = (AllowPrivateAccess), Category = "SVS|GameState")
	float CountDownStartTime = 1.0f;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Replicated, meta = (AllowPrivateAccess), Category = "SVS|GameState")
	float SpyMatchStartTime = 0.0f;
	UFUNCTION()
	void UpdatePlayerStateWithMatchTimeLength();
	
private:
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, meta = (AllowPrivateAccess), ReplicatedUsing="OnRep_RoomManager", Category = "SVS|GameState|Room")
	ARoomManager* RoomManager;

	UFUNCTION()
	void OnRep_RoomManager();

	/** The State of the Game */
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_SpyMatchState, Category = "SVS|GameState")
	ESpyMatchState SpyMatchState = ESpyMatchState::None;
	UFUNCTION()
	void OnRep_SpyMatchState() const;
	UPROPERTY()
	ESpyMatchState OldSpyMatchState = ESpyMatchState::None;

	/** The type of game being played - is correlated to which gamemode is selected */
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_SVSGameType, Category = "SVS|GameState")
	ESVSGameType SVSGameType = ESVSGameType::None;
	UFUNCTION()
	void OnRep_SVSGameType() const;

	/** Game Results */
	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_ResultsUpdated, Category = "SVS|GameState")
	TArray<FGameResult> Results;
	UFUNCTION()
	void OnRep_ResultsUpdated();
	
	/** Check if all results are in then let clients know the final results */
	void TryFinaliseScoreBoard();
	bool CheckAllResultsIn() const ;
	
};
