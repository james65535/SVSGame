// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryBaseHeldAsset.h"
#include "InventoryTrapDisarmAsset.generated.h"

/**
 * 
 */
UCLASS()
class SPYVSSPY_API UInventoryTrapDisarmAsset : public UInventoryBaseHeldAsset
{
	GENERATED_BODY()

public:

	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId("InventoryTrapDisarmAsset", GetFName()); }
};
