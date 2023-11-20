// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryComponent.h"
#include "Items/InventoryWeaponAsset.h"
#include "InventoryTrapAsset.generated.h"

enum class EInventoryOwnerType : uint8;

/**
 * 
 */
UCLASS()
class SPYVSSPY_API UInventoryTrapAsset : public UInventoryWeaponAsset
{
	GENERATED_BODY()

public:

	/** Type of inventory owner this trap may be used on */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "SVS|Inventory|Trap")
	EInventoryOwnerType InventoryOwnerType = EInventoryOwnerType::None;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId("InventoryTrapAsset", GetFName()); }
};
