// Fill out your copyright notice in the Description page of Project Settings.


#include "Players/SpyHUD.h"

#include "SVSLogger.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/UIElementWidget.h"
#include "UI/UIElementAsset.h"
#include "Players/SpyPlayerController.h"
#include "Items/InventoryComponent.h"

void ASpyHUD::BeginPlay()
{
	Super::BeginPlay();

	/** Get the GameUserSettings data container with which our work will depend upon */
	UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings();
	/** Deserialise settings from Disk - Ex: DefaultGameUserSettings.ini */
	GameUserSettings->LoadSettings();
	/** Overwrite any garbage with default settings if need be */
	GameUserSettings->ValidateSettings();

	SpyPlayerController = Cast<ASpyPlayerController>(GetOwningPlayerController());
	check(SpyPlayerController);

	/** Reduce displayed timer to max fractional digit, ex from 2.54 to 2.5 */
	FloatDisplayFormat.SetMaximumFractionalDigits(0);
}

void ASpyHUD::CreateScreenResOpts(TMap<FString, FIntPoint>& ScreenResOpts)
{
	/** Use Kismet Library to retrieve a list of Screen Resolutions */
	TArray<FIntPoint> SupportedScreenResolutions;
	UKismetSystemLibrary::GetSupportedFullscreenResolutions(SupportedScreenResolutions);

	/** Clear out data container since we are unsure of what is in there */
	ScreenResOpts.Empty();

	/** Iterate over possible Screen Resolutions and populate the data container */
	for (FIntPoint SupportedScreenResolution : SupportedScreenResolutions)
	{
		/** Derive a KeyName which is human friendly, ex: '1024 x 768' */
		FString KeyName = FString::Printf(TEXT("%i x %i"), SupportedScreenResolution.X, SupportedScreenResolution.Y);
		/** Use Emplace instead of Add to overwrite duplicate keys just in case they occur */
		ScreenResOpts.Emplace(KeyName, SupportedScreenResolution);
	}
}

void ASpyHUD::SetScreenRes(const FIntPoint InScreenRes, const bool bOverrideCommandLine)
{
	if(UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings())
	{
		GameUserSettings->SetScreenResolution(InScreenRes);
		/** We need to apply the settings before they take effect  */
		GameUserSettings->ApplyResolutionSettings(bOverrideCommandLine);
	}
}

void ASpyHUD::ConfirmGameUserSettings(const bool bOverrideCommandLine)
{
	if(UGameUserSettings* GameUserSettings = UGameUserSettings::GetGameUserSettings())
	{ GameUserSettings->ApplySettings(bOverrideCommandLine); }
}

void ASpyHUD::SetGameUIAssets(const TSoftObjectPtr<UUIElementAsset> InGameUIElementsAssets)
{
	checkfSlow(InGameUIElementsAssets, "PlayerHUD: Received Null UI Element Assets Soft Ptr");
	const UUIElementAsset* UIElementAssets = InGameUIElementsAssets.LoadSynchronous();
	checkfSlow(UIElementAssets, "PlayerHUD: Could not load UI Element Assets");

	// TODO Develop ENUM Iterator
	/** Map Asset Settings to UI Element Types and Their Corresponding Slots */
	const FName MenuSlotName = UIElementAssets->GameWidgetClasses.GameMenuWidget.WidgetSlot;
	LevelMenuWidget = AddSlotUI_Implementation(UIElementAssets->GameWidgetClasses.GameMenuWidget.WidgetClass, MenuSlotName);
	
	const FName PlaySlotName = UIElementAssets->GameWidgetClasses.GamePlayWidget.WidgetSlot;
	GameLevelWidget = AddSlotUI_Implementation(UIElementAssets->GameWidgetClasses.GamePlayWidget.WidgetClass, PlaySlotName);
	
	const FName EndSlotName = UIElementAssets->GameWidgetClasses.GameEndScreenWidget.WidgetSlot;
	LevelEndWidget = AddSlotUI_Implementation(UIElementAssets->GameWidgetClasses.GameEndScreenWidget.WidgetClass, EndSlotName);

	DisplayUI();
}

void ASpyHUD::ToggleDisplayGameTime(const bool bIsDisplayed) const
{
	check(GameLevelWidget)
	bIsDisplayed ? GameLevelWidget->DisplayGameTimer() : GameLevelWidget->HideGameTimer();
}

void ASpyHUD::SetMatchTimerSeconds(const float InMatchTimerSeconds) const
{
	check(GameLevelWidget)
	GameLevelWidget->DisplayedMatchTime = FText::AsNumber(InMatchTimerSeconds, &FloatDisplayFormat);
}

void ASpyHUD::DisplayMatchStartCountDownTime(const float InMatchStartCountDownTime) const
{
	check(GameLevelWidget)
	GameLevelWidget->InitiateMatchStartTimer(InMatchStartCountDownTime);
	check(LevelMenuWidget)
	LevelMenuWidget->HideGameMenu();
}

void ASpyHUD::DisplayCharacterHealth(const float InCurrentHealth, const float InMaxHealth) const
{
	UE_LOG(SVSLogDebug, Log, TEXT("InCurrent Health: %f InMaxHealth: %f"), InCurrentHealth, InMaxHealth);

	if (IsValid(GameLevelWidget))
	{ GameLevelWidget->DisplayCharacterHealth(InCurrentHealth, InMaxHealth); }
}

void ASpyHUD::DisplayCharacterInventory(UInventoryComponent* InventoryComponent) const
{
	if (!IsValid(GameLevelWidget) || !IsValid(InventoryComponent))
	{ return; }

	GameLevelWidget->DisplayCharacterInventory(InventoryComponent);
}

void ASpyHUD::DisplaySelectedActorInventory(const UInventoryComponent* TargetInventoryComponent) const
{
	if (!IsValid(GameLevelWidget) || !IsValid(TargetInventoryComponent))
	{ return; }

	GameLevelWidget->DisplaySelectedActorInventory(TargetInventoryComponent);
}

void ASpyHUD::UpdateUIOnFinish() const
{
	if (GameLevelWidget)
	{ GameLevelWidget->UpdateOnFinish(); }
}

void ASpyHUD::DisplayResults(const TArray<FGameResult>& InResults) const
{
	checkfSlow(LevelEndWidget, "PlayerHUD attempted to display results but LevelEndWidget was null");
	LevelEndWidget->DisplayResults(InResults);
	SpyPlayerController->RequestInputMode(EPlayerInputMode::UIOnly);
}

void ASpyHUD::UpdateResults(const TArray<FGameResult>& InResults) const
{
	checkfSlow(LevelEndWidget, "PlayerHUD attempted to update results but LevelEndWidget was null");
	LevelEndWidget->UpdateResults(InResults);
}

void ASpyHUD::UpdateServerLobby(TArray<FServerLobbyEntry>& LobbyListings) const
{
	if (!IsValid(LevelMenuWidget) || !IsValid(LevelEndWidget))
	{ return; }

	UE_LOG(SVSLogDebug, Log, TEXT("SpyHUD Update Lobby Listings with %i entries"),
		LobbyListings.Num());
	
	if (LevelMenuWidget->IsVisible())
	{ LevelMenuWidget->UpdatePlayerLobby(LobbyListings); }
	else if (LevelEndWidget->IsVisible())
	{ LevelEndWidget->UpdatePlayerLobby(LobbyListings); }
}

void ASpyHUD::RemoveResults()
{
	if (LevelEndWidget)
	{ LevelEndWidget->RemoveResults(); }

	check(BaseUIWidget)
	LevelMenuWidget->DisplayGameMenu();
}

void ASpyHUD::DisplayLevelMenu()
{
	checkfSlow(LevelMenuWidget, "PlayerHUD attempted to display Level Menu but LevelMenuWidget was null");
	LevelMenuWidget->DisplayGameMenu();
	SpyPlayerController->RequestInputMode(EPlayerInputMode::UIOnly);
}

void ASpyHUD::HideLevelMenu()
{
	checkfSlow(LevelMenuWidget, "PlayerHUD attempted to hide Level Menu but LevelMenuWidget was null");
	LevelMenuWidget->HideGameMenu();
	SpyPlayerController->RequestInputMode(EPlayerInputMode::GameOnly);
}

void ASpyHUD::UpdateDisplayedPlayerStatus(const EPlayerGameStatus InPlayerStatus) const
{
	if (IsValid(GameLevelWidget))
	{ GameLevelWidget->UpdateDisplayedPlayerStatus(InPlayerStatus); }
}

UUIElementWidget* ASpyHUD::AddSlotUI_Implementation(const TSubclassOf<UUIElementWidget> InWidgetClass, FName InSlotName)
{
	check(InWidgetClass);
	
	/** Base UI is the parent for all Widgets */
	if (BaseUIWidget == nullptr)
	{
		BaseUIWidget = AddWidget(BaseUIWidgetClass);
		check(BaseUIWidget);
	}

	if (UUIElementWidget* OutWidgetRef = CreateWidget<UUIElementWidget>(
		GetOwningPlayerController(),
		InWidgetClass))
	{
		BaseUIWidget->SetContentForSlot(InSlotName, OutWidgetRef);
		if(BaseUIWidget->GetContentForSlot(InSlotName) != nullptr)
		{ return OutWidgetRef; }
	}
	return nullptr;
}

UUIElementWidget* ASpyHUD::AddWidget(const TSubclassOf<UUIElementWidget> InWidgetClass) const
{
	check(InWidgetClass);
	
	if(UUIElementWidget* ReturnWidget = CreateWidget<UUIElementWidget>(
		GetOwningPlayerController(),
		InWidgetClass))
	{
		ReturnWidget->AddToPlayerScreen();
		return ReturnWidget;
	}
	return nullptr;
}
