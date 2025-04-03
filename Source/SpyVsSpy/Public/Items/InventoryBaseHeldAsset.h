// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryComponent.h"
#include "Items/InventoryBaseAsset.h"
#include "InventoryBaseHeldAsset.generated.h"

/**
 * 
 */
UCLASS()
class SPYVSSPY_API UInventoryBaseHeldAsset : public UInventoryBaseAsset
{
	GENERATED_BODY()

public:

	/** Type of inventory owner this Asset may be used on */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory")
	EObjectTypeAssociation ObjectTypeAssociation = EObjectTypeAssociation::None;

	/** Local Space Transform for Effect */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory")
	FTransform ItemUseEffectTransformOffset;

	/** Static Mesh for the Asset When Held */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory")
	UStaticMesh* HeldItemMesh;
	
	/** Attach Transform for Held Item */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory")
	FTransform HeldTrapAttachTransform;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId("InventoryBaseHeldAsset", GetFName()); }
	
};
