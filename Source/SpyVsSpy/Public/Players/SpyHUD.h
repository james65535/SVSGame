// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/UIElementAsset.h"
#include "GameFramework/HUD.h"
#include "GameModes/SpyVsSpyGameState.h"
#include "SpyHUD.generated.h"

class UInventoryComponent;
class ASpyPlayerController;
class ASpyVsSpyGameMode;
class UUIElementWidget;

enum class EPlayerMatchStatus : uint8;

/**
 * 
 */
UCLASS()
class SPYVSSPY_API ASpyHUD : public AHUD
{
	GENERATED_BODY()

	protected:

	virtual void BeginPlay() override;

public:
	
	/** Graphics Menu */
	/**
	 * Checks for available Screen Resolutions
	 * @param ScreenResOpts A reference to the data container with which to store the Screen Resolutions
	 */
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void CreateScreenResOpts(UPARAM(ref) TMap<FString, FIntPoint>& ScreenResOpts);
	/**
	 * Sets the given Screen Resolution
	 * @param InScreenRes The IntPoint with which to set the Screen Resolution
	 * @param bOverrideCommandLine Should the Game User Settings check for conflicting command line settings
	 */
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void SetScreenRes(const FIntPoint InScreenRes, const bool bOverrideCommandLine);
	/**
	 * Stores the Game User Settings to Disk
	 * @param bOverrideCommandLine Should the Game User Settings override conflicting command line settings
	 */
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void ConfirmGameUserSettings(const bool bOverrideCommandLine);
	
	/**
	 * UI Assets specify what elements are rendered in the Player's UI
	 * @param InGameUIElementsAssets The Data Asset Container Game UI Elements
	 */
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void SetGameUIAssets(const TSoftObjectPtr<UUIElementAsset> InGameUIElementsAssets);
	
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void DisplayUI() const {  BaseUIWidget->DisplayGameModeUI(); };
	
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void ToggleDisplayGameTime(const bool bIsDisplayed) const ;
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void SetMatchTimerSeconds(const float InMatchTimerSeconds) const ;

	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void DisplayMatchStartCountDownTime(const float InMatchStartCountDownTime) const ;
	
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void DisplayCharacterHealth(const float InCurrentHealth, const float InMaxHealth) const;

	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void DisplayCharacterInventory(const TMap<UObject*, bool>& InventoryItems) const;
	//void DisplayCharacterInventory(UInventoryComponent* InventoryComponent, const UInventoryWeaponAsset* SelectedWeaponAsset) const;
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void DisplaySelectedActorInventory(const UInventoryComponent* TargetInventoryComponent) const;

	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void CloseInventory() const;
	
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void UpdateUIOnFinish() const;

	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void DisplayResults(const TArray<FGameResult>& InResults) const;
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void UpdateResults(const TArray<FGameResult>& InResults) const;

	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void UpdateServerLobby(TArray<FServerLobbyEntry>& LobbyListings) const;

	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void RemoveResults();
	
	/** Display the Game Menu HUD within a Level */
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void DisplayLevelMenu();
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void CloseLevelMenu();

	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	void UpdateDisplayedPlayerStatus(const EPlayerGameStatus InPlayerStatus) const;

private:

	UPROPERTY()
	ASpyPlayerController* SpyPlayerController;

	/** Network latency affects precision of the float so it is better to trim the fractional when displaying */
	FNumberFormattingOptions FloatDisplayFormat;
	
	/** Level Specific UI */
	UPROPERTY(VisibleInstanceOnly, Category = "SVS|UI")
	UUIElementWidget* GameLevelWidget;
	/** Class - Level Specific UI Class */
	UPROPERTY(EditDefaultsOnly, Category = "SVS|UI")
	TSubclassOf<UUIElementWidget> GameLevelWidgetClass;
	/** Named Slot Widgets to Add Specific UI Content */
	UPROPERTY(EditDefaultsOnly, Category = "SVS|UI")
	FName LevelUINamedSlotName = "NS_LevelUI";

	/** Level End Menu UI */
	UPROPERTY(VisibleInstanceOnly, Category = "SVS|UI")
	UUIElementWidget* LevelEndWidget;
	/** Class - Level End Menu UI Class */
	UPROPERTY(EditDefaultsOnly, Category = "SVS|UI")
	TSubclassOf<UUIElementWidget> LevelEndWidgetClass;
	/** Named Slot Widgets to Add Specific UI Content */
	UPROPERTY(EditDefaultsOnly, Category = "SVS|UI")
	FName LevelEndWidgetNamedSlotName = "NS_LevelEndUI";
	
	/** Game Menu to be Displayed in Level */
	UPROPERTY(VisibleInstanceOnly, Category = "SVS|UI")
	UUIElementWidget* LevelMenuWidget;
	/** Class - Game Menu to be Displayed in Level */
	UPROPERTY(EditDefaultsOnly, Category = "SVS|UI")
	TSubclassOf<UUIElementWidget> LevelMenuWidgetClass;
	/** Named Slot Widget to Add Menu UI Content */
	UPROPERTY(EditDefaultsOnly, Category = "SVS|UI")
	FName MenuUINamedSlotName = "NS_MenuUI";
	
	/** Game Start Main Menu */
	UPROPERTY(VisibleInstanceOnly, Category = "SVS|UI")
	UUIElementWidget* GameStartScreenWidget;
	/** Class- General Game Widget */
	UPROPERTY(EditDefaultsOnly, Category = "SVS|UI")
	TSubclassOf<UUIElementWidget> GameStartScreenWidgetClass;
	UPROPERTY(EditDefaultsOnly, Category = "SVS|UI")
	FName StartMenuUINamedSlotName = "NS_StartMenuUI";

	/** Game Base UI Widget - Parent for all other UIs */
	UPROPERTY(VisibleInstanceOnly, Category = "SVS|UI")
	UUIElementWidget* BaseUIWidget;
	/** Class - Game Base UI Widget - Parent for all other UIs */
	UPROPERTY(EditDefaultsOnly, Category = "SVS|UI")
	TSubclassOf<UUIElementWidget> BaseUIWidgetClass;

	/** Methods to handle Widget Creation */
	/** Add Game Mode UI to Game Base UI */
	UFUNCTION(BlueprintCallable, Category = "SVS|UI")
	UUIElementWidget* AddSlotUI_Implementation(const TSubclassOf<UUIElementWidget> InWidgetClass, FName InSlotName);
	/** Add Widget to Player Viewport Wrapper */
	UUIElementWidget* AddWidget(const TSubclassOf<UUIElementWidget> InWidgetClass) const;
};
