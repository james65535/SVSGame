// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PlayerSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class SPYVSSPY_API UPlayerSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	UPlayerSaveGame();

	/** Player Details to Save */
	UPROPERTY(VisibleAnywhere, Category = "SVS|Save")
	FString SpyPlayerName;
	
	/** Pre-reqs for Saving */
	UPROPERTY(VisibleAnywhere, Category = "SVS|Save")
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, Category = "SVS|Save")
	uint32 UserIndex;
	
};
