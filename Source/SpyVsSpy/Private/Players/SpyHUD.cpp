// Fill out your copyright notice in the Description page of Project Settings.


#include "Players/SpyHUD.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/UIElementWidget.h"
#include "UI/UIElementAsset.h"
#include "Players/SpyPlayerController.h"

void ASpyHUD::BeginPlay()
{
	Super::BeginPlay();
}

void ASpyHUD::CreateScreenResOpts(TMap<FString, FIntPoint>& ScreenResOpts)
{
}

void ASpyHUD::SetScreenRes(FIntPoint InScreenRes, bool bOverrideCommandLine)
{
}

void ASpyHUD::ConfirmGameUserSettings(bool bOverrideCommandLine)
{
}

void ASpyHUD::SetGameUIAssets(const TSoftObjectPtr<UUIElementAsset> InGameUIElementsAssets)
{
}

void ASpyHUD::ToggleDisplayGameTime(const bool bIsDisplayed) const
{
}

void ASpyHUD::SetMatchTimerSeconds(const float InMatchTimerSeconds) const
{
}

void ASpyHUD::DisplayMatchStartCountDownTime(const float InMatchStartCountDownTime) const
{
}

void ASpyHUD::UpdateUIOnFinish() const
{
}

void ASpyHUD::DisplayResults(const TArray<FGameResult>& InResults) const
{
}

void ASpyHUD::RemoveResults()
{
}

void ASpyHUD::DisplayLevelMenu()
{
}

void ASpyHUD::HideLevelMenu()
{
}

void ASpyHUD::UpdateDisplayedPlayerState(const EPlayerGameStatus InPlayerState) const
{
}

UUIElementWidget* ASpyHUD::AddSlotUI_Implementation(TSubclassOf<UUIElementWidget> InWidgetClass, FName InSlotName)
{
	return nullptr;
}

UUIElementWidget* ASpyHUD::AddWidget(const TSubclassOf<UUIElementWidget> InWidgetClass) const
{
	return nullptr;
}
