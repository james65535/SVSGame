// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UIElementWidget.h"
#include "UIElementAsset.generated.h"

/**
 * 
 */
UCLASS()
class SPYVSSPY_API UUIElementAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "SVS|UI")
	FName Name;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "SVS|UI")
	FText Description;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "SVS|UI")
	FGameUI GameWidgetClasses;
};
