// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "EnhancedInput/Public/InputMappingContext.h"
#include "GameFramework/PlayerController.h"
#include "SpyPlayerController.generated.h"

class ASpyCharacter;
class ASpyHUD;
class ASpyVsSpyGameState;
enum class ESVSGameType : uint8;
class UGameUIElementsRegistry;
class ASpyPlayerState;

/** To Specify Which type of InputMode to Request */
UENUM(BlueprintType)
enum class EPlayerInputMode : uint8
{
	GameOnly		UMETA(DisplayName = "GameOnly No Cursor"),
	GameAndUI		UMETA(DisplayName = "Game and UI With Cursor"),
	UIOnly			UMETA(DisplayName = "UI With Cursor"),
};

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerStateReceived);

UCLASS()
class SPYVSSPY_API ASpyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	
	UPROPERTY()
	ASpyPlayerState* SpyPlayerState;
	UPROPERTY(BlueprintAssignable, Category = "SVS|Player")
	FOnPlayerStateReceived OnPlayerStateReceived;
	void SetSpyPlayerState(ASpyPlayerState* InPlayerState) { SpyPlayerState = InPlayerState; }
	UFUNCTION(BlueprintCallable, Category = "SVS|Player")
	ASpyPlayerState* GetSpyPlayerState() const { return SpyPlayerState; }
	
	/** Called by Game Widget */
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void OnRetrySelected();

	/** Called by Game Widget */
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void OnReadySelected();
	/** Called by Client Ready */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "SVS|UI")
	void S_OnReadySelected();

	/** Restart the level on client */
	UFUNCTION(Client, Reliable, Category = "SVS|UI")
	void C_ResetPlayer();
	UFUNCTION(Client, Reliable, Category = "SVS|UI")
	void C_StartGameCountDown(const float InCountDownDuration);

	void FinishedMatch();
	
	/** Get the final results and call hud to display */
	void RequestDisplayFinalResults() const;
	
	/** Called by Server Authority to restart level */
	UFUNCTION(Server, Reliable, Category = "SVS|UI")
	void S_RestartLevel();

	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void ConnectToServer(const FString InServerAddress);

	/**
	 * Set the controller input mode and cursor show
	 * @param InRequestedInputMode GameOnly / ShowCursor False  GameAndUI, UIOnly / Show Cursor True
	 */
	UFUNCTION(NetMulticast, Reliable, Category = "SVS|Controller")
	void NM_SetControllerGameInputMode(const EPlayerInputMode InRequestedInputMode);

	/** UFUNCTION Wrapper for parent class SetName method */
	UFUNCTION(BlueprintCallable, Category = "SVS|Player")
	void SetPlayerName(const FString& InPlayerName);

private:

	/** Game Related */
	UPROPERTY()
	ASpyVsSpyGameState* SpyGameState;
	UPROPERTY()
	ASpyCharacter* SpyCharacter;
	
	
	/** Values Used for Display Match Time to the Player */
	FTimerHandle MatchClockDisplayTimerHandle;
	const float MatchClockDisplayRateSeconds = 0.1f;
	float CachedMatchStartTime = 0.0f;
	void HUDDisplayGameTimeElapsedSeconds() const;
	
	/** Player HUD */
	UPROPERTY(VisibleInstanceOnly, Category = "SVS|UI")
	ASpyHUD* PlayerHUD;
	UPROPERTY(EditDefaultsOnly, Category = "SVS|UI")
	UGameUIElementsRegistry* GameElementsRegistry;
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void UpdateHUDWithGameUIElements(const ESVSGameType InGameType) const;
	/** Level Menu Display Requests */
	UFUNCTION(BlueprintCallable, Category = "SVS|")
	void RequestDisplayLevelMenu();
	UFUNCTION(BlueprintCallable, Category = "SVS|")
	void RequestHideLevelMenu();
	
	/** Enhanced Input Setup */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SVS|Input", meta = (AllowPrivateAccess))
	TSoftObjectPtr<UInputMappingContext> GameInputMapping;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SVS|Input", meta = (AllowPrivateAccess))
	TSoftObjectPtr<UInputMappingContext> MenuInputMapping;	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SVS|Input", meta = (AllowPrivateAccess))
	class UPlayerInputConfigRegistry* InputActions;
	/** Call to change Input Mapping Contexts for Controller */
	UFUNCTION(BlueprintCallable, Category = "SVS|Input")
	void SetInputContext(TSoftObjectPtr<UInputMappingContext> InMappingContext);

	/** Delegate related to Game State match start of play */
	void StartMatchForPlayer(const float InMatchStartTime);
	
	/** Checks if player is allowed to input movement commands given current state of play */
	bool CanProcessRequest() const;

	/** Character Movement Requests */
	UFUNCTION(BlueprintCallable, Category = "SVS|Movement")
	void RequestMove(const FInputActionValue& ActionValue);
	UFUNCTION(BlueprintCallable, Category = "SVS|Movement")
	void RequestNextTrap(const FInputActionValue& ActionValue);
	UFUNCTION(BlueprintCallable, Category = "SVS|Movement")
	void RequestPreviousTrap(const FInputActionValue& ActionValue);
	UFUNCTION(BlueprintCallable, Category = "SVS|Movement")
	void RequestInteract(const FInputActionValue& ActionValue);
	UFUNCTION(BlueprintCallable, Category = "SVS|Movement")
	void RequestPrimaryAttack(const FInputActionValue& ActionValue);
	
	// UFUNCTION(BlueprintCallable, Category = "CharacterMovement")
	// void RequestJump();
	// UFUNCTION(BlueprintCallable, Category = "CharacterMovement")
	// void RequestStopJump();
	// UFUNCTION(BlueprintCallable, Category = "CharacterMovement")
	// void RequestCrouch();
	// UFUNCTION(BlueprintCallable, Category = "CharacterMovement")
	// void RequestStopCrouch();
	// UFUNCTION(BlueprintCallable, Category = "CharacterMovement")
	// void RequestSprint();
	// UFUNCTION(BlueprintCallable, Category = "CharacterMovement")
	// void RequestStopSprint();

	// UFUNCTION(BlueprintCallable, Category = "CharacterMovement")
	// void RequestLook(const FInputActionValue& ActionValue);
	// UFUNCTION(BlueprintCallable, Category = "CharacterMovement")
	// void RequestThrowObject(const FInputActionValue& ActionValue);
	// UFUNCTION(BlueprintCallable, Category = "CharacterMovement")
	// void RequestHoldObject();
	// UFUNCTION(BlueprintCallable, Category = "CharacterMovement")
	// void RequestStopHoldObject();
	/** Character Look Inputs */
	// UPROPERTY(EditAnywhere,Category = "CharacterMovement")
	// float BaseLookPitchRate = 90.0f;
	// UPROPERTY(EditAnywhere,Category = "CharacterMovement")
	// float BaseLookYawRate = 90.0f;
	// /** Base lookup rate, in deg/sec.  Other scaling may affect final lookup rate */
	// UPROPERTY(EditAnywhere, Category = "Look")
	// float BaseLookUpRate = 90.0f;
	// /** Base look right rate, in deg/sec.  Other scaling may affect final lookup rate */
	// UPROPERTY(EditAnywhere, Category = "Look")
	// float BaseLookRightRate = 90.0f;

protected:

	/** Class Overrides */
	virtual void BeginPlay() override;
	virtual void OnRep_Pawn() override;
};
