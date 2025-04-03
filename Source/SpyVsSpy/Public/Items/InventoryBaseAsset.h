// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InventoryBaseAsset.generated.h"

class UInventoryItemComponent;

/**
 * 
 */
UCLASS()
class SPYVSSPY_API UInventoryBaseAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	/** ID assigned by the Item Subsystem */
	UPROPERTY(BlueprintReadOnly, Category = "SVS|Inventory")
	uint8 ItemID;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (DisplayPriority="1"), Category = "SVS|Inventory")
	FName InventoryItemName;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (DisplayPriority="2"),Category = "SVS|Inventory")
	FText InventoryItemDescription;

	/** Image to be used for visual depiction in Inventory */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (DisplayPriority="3"),Category = "SVS|Inventory")
	UTexture2D* ItemInventoryImage;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (DisplayPriority="4"),Category = "SVS|Inventory")
	TSubclassOf<UInventoryItemComponent> ItemClass;

	/** -1 Signifies unlimited */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (DisplayPriority="5"),Category = "SVS|Inventory")
	int Quantity = -1;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (DisplayPriority="6"),Category = "SVS|Inventory")
	bool bNotForDistribution = false;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId("InventoryBaseAsset", GetFName()); }
	
};
