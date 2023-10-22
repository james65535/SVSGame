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
	None		UMETA(DisplayName = "None"),
	Waiting		UMETA(DisplayName = "Waiting"),
	Ready		UMETA(DisplayName = "Ready"),
	Playing		UMETA(DisplayName = "Playing"),
	Finished	UMETA(DisplayName = "Finished"),
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
	EPlayerGameStatus GetCurrentState() const { return CurrentStatus; }
	UFUNCTION(BlueprintCallable, Category = "SVS|Player")
	void SetCurrentStatus(const EPlayerGameStatus PlayerGameStatus);

	UFUNCTION(BlueprintPure, Category = "SVS|Player")
	bool IsWinner() const { return bIsWinner; }
	void SetIsWinner(const bool IsWinner) { bIsWinner = IsWinner; }

	UFUNCTION(BlueprintCallable, Category = "SVS|Player")
	void SetPlayerRemainingMatchTime(const float InMatchTime);
	
	UFUNCTION(BlueprintPure, Category = "SVS|Player")
	float GetPlayerRemainingMatchTime() const { return PlayerRemainingMatchTime; }

	UFUNCTION(BlueprintCallable, Category = "SVS|Player")
	void ReduceRemainingMatchTime();
	
protected:

	virtual void BeginPlay() override;

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
	float TimeReductionInSeconds = 30.0f;

private:

	/** Class Overrides */
	virtual void OnRep_PlayerName() override;
	
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
