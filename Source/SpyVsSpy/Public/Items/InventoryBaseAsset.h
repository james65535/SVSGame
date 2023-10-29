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

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory")
	FName InventoryItemName;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory")
	FText InventoryItemDescription;

	/** Image to be used for visual depiction in Inventory */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory")
	UTexture* ItemInventoryImage;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory")
	TSubclassOf<UInventoryItemComponent> ItemClass;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory")
	UStaticMesh* Mesh;

	/** -1 Signifies unlimited */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory")
	int Quantity = -1;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId("InventoryBaseAsset", GetFName()); }
	
};
