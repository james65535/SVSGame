// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UIElementWidget.generated.h"

class UUIElementWidget;
class UInventoryComponent;
struct FServerLobbyEntry;
struct FGameResult;
enum class EPlayerGameStatus : uint8;

USTRUCT(BlueprintType, Category = "SVS|UI")
struct FGameUIClassInfo
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "SVS|UI")
	TSubclassOf<UUIElementWidget> WidgetClass;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "SVS|UI")
	FName WidgetSlot;
};

/**
 * 
 */
USTRUCT(BlueprintType, Category = "SVS|UI")
struct FGameUI
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "SVS|UI")
	FGameUIClassInfo GamePlayWidget;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "SVS|UI")
	FGameUIClassInfo GameMenuWidget;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "SVS|UI")
	FGameUIClassInfo GameEndScreenWidget;
};

/**
 *This class is a good place to trace back from when trying to find which game logic impacts what is
 * displayed in the HUD
 */
UCLASS()
class SPYVSSPY_API UUIElementWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintImplementableEvent, Category = "SVS|UI")
	void InitiateMatchStartTimer(float CountDownTime);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "SVS|UI")
	void DisplayGameTimer();
	UFUNCTION(BlueprintImplementableEvent, Category = "SVS|UI")
	void HideGameTimer();

	UFUNCTION(BlueprintImplementableEvent, Category = "SVS|UI")
	void UpdateOnFinish();

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FText DisplayedMatchTime = FText();

	/**
	 * Display Results of Finished Game
	 * @param InResults A Collection of listings and if the corresponding listing is a winner
	 */ 
	UFUNCTION(BlueprintImplementableEvent, Category = "SVS|UI")
	void DisplayResults(const TArray<FGameResult>& InResults);
	UFUNCTION(BlueprintImplementableEvent, Category = "SVS|UI")
	void UpdateResults(const TArray<FGameResult>& InResults);

	UFUNCTION(BlueprintImplementableEvent, Category = "SVS|UI")
	void RemoveResults();

	UFUNCTION(BlueprintImplementableEvent, Category = "SVS|UI")
	void DisplayStartMenu();

	UFUNCTION(BlueprintImplementableEvent, Category = "SVS|UI")
	void DisplayGameModeUI();
	UFUNCTION(BlueprintImplementableEvent, Category = "SVS|UI")
	void UpdateDisplayedPlayerStatus(EPlayerGameStatus InPlayerStatus);

	UFUNCTION(BlueprintImplementableEvent, Category = "SVS|UI")
	void UpdatePlayerLobby(const TArray<FServerLobbyEntry>& LobbyListing);

	UFUNCTION(BlueprintImplementableEvent, Category = "SVS|UI")
	void DisplayGameMenu();
	UFUNCTION(BlueprintImplementableEvent, Category = "SVS|UI")
	void CloseGameMenu();

	UFUNCTION(BlueprintImplementableEvent, Category = "SVS|UI")
	void DisplayCharacterInventory(const TMap<UObject*, bool>& InventoryItems);
	UFUNCTION(BlueprintImplementableEvent, Category = "SVS|UI")
	void DisplaySelectedActorInventory(const UInventoryComponent* InventoryComponent);
	UFUNCTION(BlueprintImplementableEvent, Category = "SVS|UI")
	void CloseInventory();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "SVS|UI")
	void DisplayCharacterHealth(const float InCurrentHealth, const float InMaxHealth);
	
};
