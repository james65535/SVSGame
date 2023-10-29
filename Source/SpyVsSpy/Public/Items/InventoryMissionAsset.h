// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/InventoryBaseAsset.h"
#include "InventoryMissionAsset.generated.h"

/**
 * 
 */
UCLASS()
class SPYVSSPY_API UInventoryMissionAsset : public UInventoryBaseAsset
{
	GENERATED_BODY()

public:

	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId("InventoryMissionAsset", GetFName()); }
};
