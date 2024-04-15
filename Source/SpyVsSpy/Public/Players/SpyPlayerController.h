// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "EnhancedInput/Public/InputMappingContext.h"
#include "GameFramework/PlayerController.h"
#include "SpyPlayerController.generated.h"

class UInventoryComponent;
class UInventoryWeaponAsset;
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

	UPROPERTY(BlueprintAssignable, Category = "SVS|Player")
	FOnPlayerStateReceived OnPlayerStateReceived;
	void SetSpyPlayerState(ASpyPlayerState* InPlayerState) { SpyPlayerState = InPlayerState; }
	UFUNCTION(BlueprintCallable, Category = "SVS|Player")
	ASpyPlayerState* GetSpyPlayerState() const { return SpyPlayerState; }
	UFUNCTION(BlueprintCallable, Category = "SVS|Player")
	ASpyCharacter* GetSpyCharacter() const { return SpyCharacter; }
	
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

	void EndMatch();
	
	/** Get the final results and call hud to display */
	void RequestUpdatePlayerResults() const;
	
	/** Called by Server Authority to restart level */
	UFUNCTION(Server, Reliable, Category = "SVS|UI")
	void S_RestartLevel();

	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void ConnectToServer(const FString InServerAddress);

	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void OnServerLobbyUpdateDelegate();

	/** UFUNCTION Wrapper for parent class SetName method */
	UFUNCTION(BlueprintCallable, Category = "SVS|Player")
	void SetPlayerName(const FString& InPlayerName);

	/** Take inventory items from a target and place them in character inventory */
	UFUNCTION(BlueprintCallable, Category = "SVS|Player")
	void RequestTakeAllFromTargetInventory();

	/** Place currently held trap in current interactable actor */
	UFUNCTION(BlueprintCallable, Category = "SVS|Player")
	void RequestPlaceTrap();
	
	UFUNCTION(BlueprintCallable, Client, Reliable, Category = "SVS|Player")
	void C_DisplayCharacterInventory();
	UFUNCTION(BlueprintCallable, Client, Reliable, Category = "SVS|Player")
	void C_DisplayTargetInventory(UInventoryComponent* TargetInventory);

	/**
	 *Set the controller input mode and cursor show
	 *@param DesiredInputMode GameOnly / ShowCursor False  GameAndUI, UIOnly / Show Cursor True
	 */
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void RequestInputMode(const EPlayerInputMode DesiredInputMode);

protected:

	/** Class Overrides */
	virtual void BeginPlay() override;
	virtual void OnRep_Pawn() override;
	virtual void OnPossess(APawn* InPawn) override;

#pragma region="HUD"
	/** Player HUD */
	UPROPERTY(VisibleInstanceOnly, Category = "SVS|UI")
	ASpyHUD* SpyPlayerHUD;
	UPROPERTY(EditDefaultsOnly, Category = "SVS|UI")
	UGameUIElementsRegistry* GameElementsRegistry;
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void UpdateHUDWithGameUIElements(const ESVSGameType InGameType) const;
	/** Level Menu Display Requests */
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void RequestDisplayLevelMenu();
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void RequestCloseLevelMenu();
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void RequestCloseInventory();
#pragma endregion="HUD"
	
#pragma region="Input"
	/** Enhanced Input Setup */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SVS|Input", meta = (AllowPrivateAccess))
	TSoftObjectPtr<UInputMappingContext> GameInputMapping;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "SVS|Input", meta = (AllowPrivateAccess))
	TSoftObjectPtr<UInputMappingContext> MenuInputMapping;	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SVS|Input", meta = (AllowPrivateAccess))
	class UPlayerInputConfigRegistry* InputActions;

	/** Call to change Input Mapping Contexts for Controller */
	UFUNCTION(BlueprintCallable, Category = "SVS|Input")
	void SetInputContext(const TSoftObjectPtr<UInputMappingContext> InMappingContext) const;
	
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
	
	UFUNCTION(BlueprintCallable, Client, Reliable, Category = "SVS|UI")
	void C_RequestInputMode(const EPlayerInputMode DesiredInputMode);

	/** Place currently held trap in current interactable actor */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "SVS|Player")
	void S_RequestPlaceTrap();

	/** Server function to take inventory items from a target and place them in character inventory */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "SVS|Player")
	void S_RequestTakeAllFromTargetInventory();
#pragma endregion="Input"

#pragma region="Game"
	/** Game Related */
	UPROPERTY()
	ASpyVsSpyGameState* SpyGameState;
	UPROPERTY()
	ASpyCharacter* SpyCharacter;
	UPROPERTY()
	ASpyPlayerState* SpyPlayerState;

	/** Delegate related to Game State match start of play */
	void StartMatchForPlayer(const float InMatchStartTime);

	/** Values Used for Display Match Time to the Player */
	FTimerHandle MatchClockDisplayTimerHandle;
	const float MatchClockDisplayRateSeconds = 1.0f;
	float LocalClientCachedMatchStartTime = 0.0f;
	void CalculateGameTimeElapsedSeconds();
	void HUDDisplayGameTimeElapsedSeconds(const float InTimeToDisplay) const;
#pragma endregion="Game"
};
