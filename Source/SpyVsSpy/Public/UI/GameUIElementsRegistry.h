// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameModes/SpyVsSpyGameState.h"
#include "UI/UIElementAsset.h"
#include "Engine/DataAsset.h"
#include "GameUIElementsRegistry.generated.h"

/**
 * 
 */
UCLASS()
class SPYVSSPY_API UGameUIElementsRegistry : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SVS|UI")
	TMap<ESVSGameType, TSoftObjectPtr<UUIElementAsset>> GameTypeUIMapping;
};
