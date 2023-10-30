// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/SpyAbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "SpyPlayerState.generated.h"

class USpyAbilitySystemComponent;
class USpyAttributeSet;
class USaveGame;

/** Enum to track the current state of the player */
UENUM(BlueprintType)
enum class EPlayerGameStatus : uint8
{
	None							UMETA(DisplayName = "None"),
	WaitingForStart					UMETA(DisplayName = "Waiting For Start"),
	Ready							UMETA(DisplayName = "Ready"),
	Playing							UMETA(DisplayName = "Playing"),
	MatchTimeExpired				UMETA(DisplayName = "MatchTimeExpired"),
	WaitingForAllPlayersFinish		UMETA(DisplayName = "Waiting For All Players Finish"),
	Finished						UMETA(DisplayName = "Finished"),
	LoadingLevel					UMETA(DisplayName = "Loading Level"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSaveGameUpdate);

/**
 * 
 */

UCLASS()
class SPYVSSPY_API ASpyPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:

	ASpyPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UFUNCTION(BlueprintCallable, Category = "SVS|Abilities")
	USpyAbilitySystemComponent* GetSpyAbilitySystemComponent() const { return AbilitySystemComponent; }

	USpyAttributeSet* GetAttributeSet() const;

	/** Health Attribute Getters */
	UFUNCTION(BlueprintCallable, Category = "SVS|Attributes")
	bool IsAlive() const;
	UFUNCTION(BlueprintCallable, Category = "SVS|Attributes")
	float GetHealth() const;
	UFUNCTION(BlueprintCallable, Category = "SVS|Attributes")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category = "SVS|Save")
	void SavePlayerInfo();
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "SVS|Save")
	void LoadSavedPlayerInfo();
	UPROPERTY(BlueprintAssignable, Category = "SVS|Save")
	FOnSaveGameUpdate OnSaveGameLoad;

	UFUNCTION(BlueprintPure, Category = "SVS|Player")
	EPlayerGameStatus GetCurrentStatus() const { return CurrentStatus; }
	UFUNCTION(BlueprintCallable, Category = "SVS|Player")
	void SetCurrentStatus(const EPlayerGameStatus PlayerGameStatus);

	UFUNCTION(BlueprintPure, Category = "SVS|Player")
	bool IsWinner() const { return bIsWinner; }
	void SetIsWinner(const bool IsWinner) { bIsWinner = IsWinner; }
	void OnPlayerReachedEnd();
	
	UFUNCTION(BlueprintCallable, Category = "SVS|Player")
	void SetPlayerRemainingMatchTime(const float InMatchTimeLength = 0.0f, const bool bIncludeTimePenalty = false);
	
	UFUNCTION(BlueprintPure, Category = "SVS|Player")
	float GetPlayerRemainingMatchTime() const;
	
protected:
	
	/** Class Overrides */
	virtual void BeginPlay() override;
	virtual void OnRep_PlayerName() override;
	virtual void OnRep_PlayerId() override;

	/** Ability System */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SVS|Abilities")
	USpyAbilitySystemComponent* AbilitySystemComponent;

	/** Attributes */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SVS|Attributes")
	USpyAttributeSet* AttributeSet;

	FDelegateHandle HealthChangedDelegateHandle;
	virtual void HealthChanged(const FOnAttributeChangeData& Data);

	/** Ability System Tags */
	FGameplayTag SpyDeadTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess), Replicated, Category = "SVS|Player")
	float PlayerRemainingMatchTime = 0.0f;
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess), Category = "SVS|Player")
	float PlayerMatchTimePenaltyInSeconds = 30.0f;
	float GetPlayerSpyMatchSecondsRemaining() const;
	
	/** Values Used for Display Match Time to the Player */
	FTimerHandle PlayerMatchTimerHandle;
	void PlayerMatchTimeExpired();
	void SetPlayerMatchTimer();

private:
	
	/** Player Status in the game */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess), ReplicatedUsing = OnRep_CurrentStatus, Category = "SVS|Player")
	EPlayerGameStatus CurrentStatus = EPlayerGameStatus::None;
	UFUNCTION()
	virtual void OnRep_CurrentStatus();

	/** Game result winner state */
	bool bIsWinner = false;

	/** Save and Load Delegates */
	void SavePlayerDelegate(const FString& SlotName, const int32 UserIndex, bool bSuccess);
	void LoadPlayerSaveDelegate(const FString& SlotName, const int32 UserIndex, USaveGame* LoadedGameData);
	// TODO Build these properties out instead of being static
	const FString SaveSlotName = TEXT("GeneralSaveSlot");
	const uint32 SaveUserIndex = 0;
	
};
